/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FileservePlugin/FileservePluginPCH.h>

#include <FileservePlugin/Client/FileserveClient.h>
#include <FileservePlugin/Discovery/FileserveDiscovery.h>
#include <FileservePlugin/Fileserver/Fileserver.h>
#include <FileservePlugin/Implementation/FileserveTransfer.h>
#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Strings/UnicodeUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <cstring>

using xiiFileserveTransfer::Operation;
using xiiFileserveTransfer::Request;

XII_IMPLEMENT_SINGLETON(xiiFileserver);

struct xiiFileserver::Download
{
  xiiNetworkChannel                     m_Channel;
  xiiFileserveClientContext::FileStatus m_Status;
  xiiFileserveFileState                 m_State = xiiFileserveFileState::None;
  xiiOSFile                             m_File;
  xiiString                             m_sPath;
  xiiUuid                               m_Id;
  xiiTime                               m_Deadline;
  xiiUInt32                             m_uiClientId  = 0;
  xiiUInt32                             m_uiSent      = 0;
  xiiUInt16                             m_uiDataDir   = 0;
  xiiUInt16                             m_uiChunkSize = 0;
  bool                                  m_bChunkReady = false;
  bool                                  m_bSentChunk  = false;
  xiiUInt8                              m_Chunk[xiiFileserveTransfer::ChunkBytes];
};

namespace
{
  bool HasRootPermission(xiiStringView sAlias, bool bWrite)
  {
    const auto&         command = *xiiCommandLineUtils::GetGlobalInstance();
    const xiiStringView sOption = bWrite ? "-fs_write_roots" : "-fs_read_roots";
    const auto          uiCount = command.GetStringOptionArguments(sOption);
    if (!bWrite && !command.HasOption(sOption))
      return sAlias.IsEqual_NoCase("sdk") || sAlias.IsEqual_NoCase("project");

    for (xiiUInt32 i = 0; i < uiCount; ++i)
    {
      if (sAlias.IsEqual_NoCase(command.GetStringOption(sOption, i)))
        return true;
    }
    return false;
  }

  bool IsShaderIdentifier(xiiStringView sText)
  {
    if (sText.IsEmpty() || sText.GetElementCount() > 96)
      return false;

    for (const auto c : sText)
    {
      if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'))
        return false;
    }
    return true;
  }

  xiiResult ReadRequestText(xiiStreamReader& ref_reader, xiiStringBuilder& out_sText)
  {
    xiiUInt32 uiLength = 0;
    XII_SUCCEED_OR_RETURN(ref_reader.ReadDWordValue(&uiLength));
    char szText[4096];
    if (uiLength > sizeof(szText) || ref_reader.ReadBytes(szText, uiLength) != uiLength ||
        !xiiUnicodeUtils::IsValidUtf8(szText, szText + uiLength) || std::memchr(szText, '\0', uiLength) != nullptr)
      return XII_FAILURE;
    out_sText = xiiStringView(szText, szText + uiLength);
    return XII_SUCCESS;
  }
} // namespace

xiiFileserver::xiiFileserver() :
  m_SingletonRegistrar(this)
{
  // once a server exists, the client should stay inactive
  xiiFileserveClient::DisabledFileserveClient();

  // check whether the fileserve port was reconfigured through the command line
  m_uiPort = static_cast<xiiUInt16>(xiiCommandLineUtils::GetGlobalInstance()->GetIntOption("-fs_port", m_uiPort));
}

xiiFileserver::~xiiFileserver()
{
  StopServer();
}

void xiiFileserver::StartServer()
{
  if (m_pNetwork)
    return;

  xiiUniquePtr<xiiNetworkHost> host = XII_DEFAULT_NEW(xiiNetworkHost);
  xiiNetworkListener           listener;
  if (xiiFileserveTransfer::Start(*host, true).Failed() || xiiFileserveTransfer::Listen(*host, m_uiPort, listener).Failed())
  {
    xiiLog::Error("Could not start FileServe listener {}.", m_uiPort);
    return;
  }
  m_pNetwork = std::move(host);

  xiiFileserverEvent e;
  e.m_Type = xiiFileserverEvent::Type::ServerStarted;
  m_Events.Broadcast(e);
}

void xiiFileserver::StopServer()
{
  if (!m_pNetwork)
    return;

  m_pNetwork->Shutdown();
  m_pNetwork.Clear();
  m_Uploads.Clear();
  m_Downloads.Clear();
  m_Clients.Clear();

  xiiFileserverEvent e;
  e.m_Type = xiiFileserverEvent::Type::ServerStopped;
  m_Events.Broadcast(e);
}

bool xiiFileserver::UpdateServer()
{
  if (!m_pNetwork)
    return false;

  bool            bProcessed = false;
  xiiNetworkEvent event;
  for (xiiUInt32 i = 0; i < 256 && m_pNetwork->TryGetEvent(event); ++i)
  {
    bProcessed        = true;
    const auto uiSlot = event.m_Connection.m_uiSlot;
    if (event.m_Type == xiiNetworkEventType::Connected && event.m_pSessionInfo)
    {
      auto& client             = m_Clients[uiSlot];
      client.m_Channel         = event.m_pSessionInfo->m_DefaultChannel;
      client.m_uiApplicationID = ++m_uiNextClientId;
      xiiFileserverEvent e;
      e.m_Type       = xiiFileserverEvent::Type::ClientConnected;
      e.m_uiClientID = client.m_uiApplicationID;
      m_Events.Broadcast(e);
      continue;
    }
    auto* client = m_Clients.GetValue(uiSlot);
    if (client == nullptr || client->m_Channel.m_Connection != event.m_Connection)
      continue;
    if (event.m_Type == xiiNetworkEventType::Closed)
    {
      xiiFileserverEvent e;
      e.m_Type       = xiiFileserverEvent::Type::ClientDisconnected;
      e.m_uiClientID = client->m_uiApplicationID;
      m_Uploads.Remove(e.m_uiClientID);
      m_Downloads.Remove(e.m_uiClientID);
      m_Clients.Remove(uiSlot);
      m_Events.Broadcast(e);
    }
    else if (event.m_Type == xiiNetworkEventType::Message && event.m_Channel == client->m_Channel)
    {
      NetworkMsgHandler(*client, event.m_Payload.GetBytes());
    }
  }

  const auto now = xiiTime::Now();
  for (auto it = m_Uploads.GetIterator(); it.IsValid();)
  {
    if (it.Value()->m_Deadline <= now)
      it = m_Uploads.Remove(it);
    else
      ++it;
  }
  for (auto it = m_Downloads.GetIterator(); it.IsValid();)
  {
    if (PumpDownload(*it.Value()))
      it = m_Downloads.Remove(it);
    else
      ++it;
  }
  return bProcessed;
}

bool xiiFileserver::IsServerRunning() const
{
  return m_pNetwork != nullptr;
}

void xiiFileserver::SetPort(xiiUInt16 uiPort)
{
  XII_ASSERT_DEV(m_pNetwork == nullptr, "The port cannot be changed after the server was started");
  m_uiPort = uiPort;
}


void xiiFileserver::BroadcastReloadResourcesCommand()
{
  if (!IsServerRunning())
    return;

  const Request request(Operation::Reload);
  for (auto it = m_Clients.GetIterator(); it.IsValid(); ++it)
    SendResponse(it.Value().m_Channel, request);
}

void xiiFileserver::NetworkMsgHandler(xiiFileserveClientContext& client, xiiArrayPtr<const xiiUInt8> data)
{
  xiiRawMemoryStreamReader reader(data.GetPtr(), data.GetCount());
  Operation                operation;
  if (xiiFileserveTransfer::ReadOperation(reader, operation).Failed())
  {
    m_pNetwork->Disconnect(client.m_Channel.m_Connection, false).IgnoreResult();
    return;
  }

  data = data.GetSubArray(1);
  switch (operation)
  {
    case Operation::Mount:
      HandleMountRequest(client, data);
      break;
    case Operation::Unmount:
      HandleUnmountRequest(client, data);
      break;
    case Operation::Read:
      HandleFileRequest(client, data);
      break;
    case Operation::Delete:
      HandleDeleteFileRequest(client, data);
      break;
    case Operation::UploadBegin:
      HandleUploadFileHeader(client, data);
      break;
    case Operation::UploadChunk:
      HandleUploadFileTransfer(client, data);
      break;
    case Operation::UploadEnd:
      HandleUploadFileFinished(client, data);
      break;
    case Operation::CompileShader:
      HandleShaderCompileRequest(client, data);
      break;
    default:
      m_pNetwork->Disconnect(client.m_Channel.m_Connection, false).IgnoreResult();
      break;
  }
}

void xiiFileserver::SetShaderCompileHandler(ShaderCompileHandler handler)
{
  m_ShaderCompileHandler = std::move(handler);
}

void xiiFileserver::SendResponse(xiiNetworkChannel channel, const Request& request)
{
  if (request.Send(*m_pNetwork, channel).Failed())
    m_pNetwork->Disconnect(channel.m_Connection, false).IgnoreResult();
}

void xiiFileserver::HandleMountRequest(xiiFileserveClientContext& client, xiiArrayPtr<const xiiUInt8> data)
{
  xiiStringBuilder sDataDir, sRootName, sMountPoint, sRedir;
  xiiUInt16        uiDataDirID = 0xffff;

  xiiRawMemoryStreamReader reader(data.GetPtr(), data.GetCount());
  if (ReadRequestText(reader, sDataDir).Failed() || ReadRequestText(reader, sRootName).Failed() ||
      ReadRequestText(reader, sMountPoint).Failed() || reader.ReadWordValue(&uiDataDirID).Failed() || uiDataDirID >= 4096 ||
      (!sRootName.IsEmpty() && (!xiiFileserveClientContext::IsValidRelativePath(sRootName) || sRootName.FindSubString("/") != nullptr)) ||
      (uiDataDirID < client.m_MountedDataDirs.GetCount() && client.m_MountedDataDirs[uiDataDirID].m_bMounted))
    return;

  client.m_MountedDataDirs.SetCount(xiiMath::Max<xiiUInt32>(uiDataDirID + 1, client.m_MountedDataDirs.GetCount()));
  auto& dir           = client.m_MountedDataDirs[uiDataDirID];
  dir.m_sPathOnClient = sDataDir;
  dir.m_sRootName     = sRootName;
  dir.m_sMountPoint   = sMountPoint;

  xiiFileserverEvent e;

  xiiStringBuilder sMountName = sDataDir;
  sMountName.TrimWordEnd("/");
  if (sMountName.TrimWordStart(">") && xiiFileserveClientContext::IsValidRelativePath(sMountName))
  {
    const xiiStringView mountName = sMountName;
    const auto*         pSlash    = sMountName.FindSubString("/");
    const xiiStringView alias(mountName.GetStartPointer(), pSlash != nullptr ? pSlash : mountName.GetEndPointer());
    xiiStringBuilder    sSpecial(">", alias);
    if (HasRootPermission(alias, false) && xiiFileSystem::ResolveSpecialDirectory(sSpecial, sRedir).Succeeded())
    {
      dir.m_bMounted = pSlash == nullptr ||
        xiiFileserveClientContext::ResolveFilePath(sRedir, xiiStringView(pSlash + 1, mountName.GetEndPointer()), sRedir, true).Succeeded();
      dir.m_bWritable = dir.m_bMounted && HasRootPermission(alias, true);
    }
  }

  if (dir.m_bMounted)
  {
    dir.m_bMounted      = true;
    dir.m_sPathOnServer = sRedir;
    e.m_Type            = xiiFileserverEvent::Type::MountDataDir;
  }
  else
  {
    dir.m_bMounted = false;
    e.m_Type       = xiiFileserverEvent::Type::MountDataDirFailed;
  }

  e.m_uiClientID       = client.m_uiApplicationID;
  e.m_szName           = sRootName;
  e.m_szPath           = sDataDir;
  e.m_szRedirectedPath = sRedir;
  m_Events.Broadcast(e);
}


void xiiFileserver::HandleUnmountRequest(xiiFileserveClientContext& client, xiiArrayPtr<const xiiUInt8> data)
{
  xiiUInt16                uiDataDirID = 0xffff;
  xiiRawMemoryStreamReader reader(data.GetPtr(), data.GetCount());
  if (reader.ReadWordValue(&uiDataDirID).Failed() || uiDataDirID >= client.m_MountedDataDirs.GetCount())
    return;

  auto& dir      = client.m_MountedDataDirs[uiDataDirID];
  dir.m_bMounted = false;
  if (const auto* upload = m_Uploads.GetValue(client.m_uiApplicationID); upload != nullptr && (*upload)->m_uiDataDir == uiDataDirID)
    m_Uploads.Remove(client.m_uiApplicationID);

  xiiFileserverEvent e;
  e.m_Type       = xiiFileserverEvent::Type::UnmountDataDir;
  e.m_uiClientID = client.m_uiApplicationID;
  e.m_szPath     = dir.m_sPathOnClient;
  e.m_szName     = dir.m_sRootName;
  m_Events.Broadcast(e);
}

void xiiFileserver::HandleFileRequest(xiiFileserveClientContext& client, xiiArrayPtr<const xiiUInt8> data)
{
  if (m_Downloads.Contains(client.m_uiApplicationID))
  {
    m_pNetwork->Disconnect(client.m_Channel.m_Connection, false).IgnoreResult();
    return;
  }

  xiiRawMemoryStreamReader reader(data.GetPtr(), data.GetCount());
  xiiUInt8                 uiForce = 0;
  xiiStringBuilder         sRequestedFile;
  xiiUInt64                uiIdLow  = 0;
  xiiUInt64                uiIdHigh = 0;
  xiiUniquePtr<Download>   download = XII_DEFAULT_NEW(Download);
  if (reader.ReadWordValue(&download->m_uiDataDir).Failed() || reader.ReadBytes(&uiForce, sizeof(uiForce)) != sizeof(uiForce) ||
      uiForce > 1 || ReadRequestText(reader, sRequestedFile).Failed() ||
      reader.ReadQWordValue(&uiIdHigh).Failed() || reader.ReadQWordValue(&uiIdLow).Failed() ||
      reader.ReadQWordValue(&download->m_Status.m_iTimestamp).Failed() || reader.ReadQWordValue(&download->m_Status.m_uiHash).Failed())
    return;

  download->m_Id = xiiUuid(uiIdLow, uiIdHigh);
  if (!download->m_Id.IsValid())
    return;
  download->m_sPath      = sRequestedFile;
  download->m_Channel    = client.m_Channel;
  download->m_uiClientId = client.m_uiApplicationID;
  download->m_Deadline   = xiiTime::Now() + xiiTime::MakeFromSeconds(120);
  download->m_State      = client.GetFileStatus(download->m_uiDataDir, sRequestedFile, download->m_Status, download->m_File, uiForce != 0);

  xiiFileserverEvent e;
  e.m_Type        = xiiFileserverEvent::Type::FileDownloadRequest;
  e.m_uiClientID  = client.m_uiApplicationID;
  e.m_szPath      = sRequestedFile;
  e.m_uiSizeTotal = static_cast<xiiUInt32>(download->m_Status.m_uiFileSize);
  e.m_FileState   = download->m_State;
  m_Events.Broadcast(e);
  m_Downloads.Insert(client.m_uiApplicationID, std::move(download));
}

bool xiiFileserver::PumpDownload(Download& download)
{
  if (xiiTime::Now() >= download.m_Deadline)
  {
    m_pNetwork->Disconnect(download.m_Channel.m_Connection, false).IgnoreResult();
    return true;
  }

  xiiFileserverEvent e;
  e.m_uiClientID  = download.m_uiClientId;
  e.m_szPath      = download.m_sPath;
  e.m_uiSizeTotal = static_cast<xiiUInt32>(download.m_Status.m_uiFileSize);
  e.m_FileState   = download.m_State;
  // Bound each peer's disk and publication work per server update.
  for (xiiUInt32 i = 0; i < 8 && download.m_State == xiiFileserveFileState::Different &&
       (!download.m_bSentChunk || download.m_uiSent < e.m_uiSizeTotal);
       ++i)
  {
    if (!download.m_bChunkReady)
    {
      download.m_uiChunkSize = static_cast<xiiUInt16>(xiiMath::Min(xiiFileserveTransfer::ChunkBytes, e.m_uiSizeTotal - download.m_uiSent));
      if (download.m_uiChunkSize != 0 && download.m_File.Read(download.m_Chunk, download.m_uiChunkSize) != download.m_uiChunkSize)
      {
        download.m_State = xiiFileserveFileState::None;
        break;
      }
      download.m_bChunkReady = true;
    }
    Request chunk(Operation::DownloadChunk);
    chunk.GetWriter() << download.m_Id;
    chunk.GetWriter() << download.m_uiChunkSize;
    chunk.GetWriter() << e.m_uiSizeTotal;
    if (chunk.GetWriter().WriteBytes(download.m_Chunk, download.m_uiChunkSize).Failed())
    {
      download.m_State = xiiFileserveFileState::None;
      break;
    }
    const auto result = chunk.Send(*m_pNetwork, download.m_Channel);
    if (result.Failed())
    {
      if (result.m_Code == xiiNetworkErrorCode::Backpressure || result.m_Code == xiiNetworkErrorCode::ResourceExhausted)
        return false;
      m_pNetwork->Disconnect(download.m_Channel.m_Connection, false).IgnoreResult();
      return true;
    }
    download.m_bChunkReady = false;
    download.m_bSentChunk  = true;
    download.m_uiSent += download.m_uiChunkSize;
    e.m_Type        = xiiFileserverEvent::Type::FileDownloading;
    e.m_uiSentTotal = download.m_uiSent;
    m_Events.Broadcast(e);
  }
  if (download.m_State == xiiFileserveFileState::Different && download.m_uiSent < e.m_uiSizeTotal)
    return false;

  Request response(Operation::DownloadResult);
  response.GetWriter() << download.m_Id;
  response.GetWriter() << static_cast<xiiInt8>(download.m_State);
  response.GetWriter() << download.m_Status.m_iTimestamp;
  response.GetWriter() << download.m_Status.m_uiHash;
  response.GetWriter() << download.m_uiDataDir;
  const auto result = response.Send(*m_pNetwork, download.m_Channel);
  if (result.m_Code == xiiNetworkErrorCode::Backpressure || result.m_Code == xiiNetworkErrorCode::ResourceExhausted)
    return false;
  if (result.Failed())
    m_pNetwork->Disconnect(download.m_Channel.m_Connection, false).IgnoreResult();
  e.m_Type        = xiiFileserverEvent::Type::FileDownloadFinished;
  e.m_uiSentTotal = download.m_uiSent;
  e.m_FileState   = download.m_State;
  m_Events.Broadcast(e);
  return true;
}

void xiiFileserver::HandleDeleteFileRequest(xiiFileserveClientContext& client, xiiArrayPtr<const xiiUInt8> data)
{
  xiiUInt16                uiDataDirID = 0xffff;
  xiiStringBuilder         sFile;
  xiiRawMemoryStreamReader reader(data.GetPtr(), data.GetCount());
  if (reader.ReadWordValue(&uiDataDirID).Failed() || uiDataDirID >= client.m_MountedDataDirs.GetCount() ||
      !client.m_MountedDataDirs[uiDataDirID].m_bWritable || ReadRequestText(reader, sFile).Failed())
    return;

  xiiStringBuilder sAbsPath;
  if (xiiFileserveClientContext::ResolveFilePath(client.m_MountedDataDirs[uiDataDirID].m_sPathOnServer, sFile, sAbsPath).Failed())
    return;

  xiiFileserverEvent e;
  e.m_Type       = xiiFileserverEvent::Type::FileDeleteRequest;
  e.m_uiClientID = client.m_uiApplicationID;
  e.m_szPath     = sFile;
  m_Events.Broadcast(e);

  xiiOSFile::DeleteFile(sAbsPath).IgnoreResult();
}

xiiFileserver::Upload::~Upload()
{
  m_File.Close();
  if (!m_sStagingPath.IsEmpty())
    xiiOSFile::DeleteFile(m_sStagingPath).IgnoreResult();
}

void xiiFileserver::HandleUploadFileHeader(xiiFileserveClientContext& client, xiiArrayPtr<const xiiUInt8> data)
{
  if (data.GetCount() < sizeof(xiiUuid))
    return;

  xiiUuid                  request;
  xiiRawMemoryStreamReader reader(data.GetPtr(), data.GetCount());
  reader >> request;
  xiiUInt32        uiSize = 0;
  xiiUInt16        uiDir  = 0;
  xiiStringBuilder sRelativePath, sDestination;
  if (!request.IsValid() || m_Uploads.Contains(client.m_uiApplicationID) || m_Uploads.GetCount() >= 64 ||
      reader.ReadDWordValue(&uiSize).Failed() || reader.ReadWordValue(&uiDir).Failed() ||
      uiDir >= client.m_MountedDataDirs.GetCount() || !client.m_MountedDataDirs[uiDir].m_bWritable ||
      ReadRequestText(reader, sRelativePath).Failed() ||
      xiiFileserveClientContext::ResolveFilePath(client.m_MountedDataDirs[uiDir].m_sPathOnServer, sRelativePath, sDestination).Failed())
  {
    SendUploadResult(client.m_Channel, request, false);
    return;
  }

  xiiUniquePtr<Upload> pUpload = XII_DEFAULT_NEW(Upload);
  pUpload->m_Id                = request;
  pUpload->m_uiSize            = uiSize;
  pUpload->m_uiDataDir         = uiDir;
  pUpload->m_sRelativePath     = sRelativePath;
  pUpload->m_sDestination      = sDestination;
  pUpload->m_Deadline          = xiiTime::Now() + xiiTime::MakeFromSeconds(120);

  xiiStringBuilder sStagingId;
  xiiConversionUtils::ToString(xiiUuid::MakeUuid(), sStagingId);
  const xiiStringBuilder sStagingPath(sDestination.GetFileDirectory(), "/.xii-upload-", sStagingId, ".tmp");
  if (pUpload->m_File.Open(sStagingPath, xiiFileOpenMode::Write).Failed())
  {
    SendUploadResult(client.m_Channel, request, false);
    return;
  }
  pUpload->m_sStagingPath = sStagingPath;
  m_Uploads.Insert(client.m_uiApplicationID, std::move(pUpload));

  xiiFileserverEvent e;
  e.m_Type        = xiiFileserverEvent::Type::FileUploadRequest;
  e.m_uiClientID  = client.m_uiApplicationID;
  e.m_szPath      = sRelativePath;
  e.m_uiSizeTotal = uiSize;
  m_Events.Broadcast(e);
}

void xiiFileserver::HandleUploadFileTransfer(xiiFileserveClientContext& client, xiiArrayPtr<const xiiUInt8> data)
{
  auto* pStored = m_Uploads.GetValue(client.m_uiApplicationID);
  if (pStored == nullptr || data.GetCount() < sizeof(xiiUuid))
    return;

  auto&                    upload = **pStored;
  xiiRawMemoryStreamReader reader(data.GetPtr(), data.GetCount());
  xiiUuid                  request;
  reader >> request;
  if (request != upload.m_Id)
    return;

  xiiUInt16           uiChunkSize = 0;
  constexpr xiiUInt32 uiHeader    = 16 + sizeof(uiChunkSize);
  if (reader.ReadWordValue(&uiChunkSize).Failed() || uiChunkSize == 0 ||
      data.GetCount() != uiHeader + uiChunkSize || uiChunkSize > upload.m_uiSize - upload.m_uiReceived ||
      upload.m_File.Write(data.GetPtr() + uiHeader, uiChunkSize).Failed())
  {
    m_Uploads.Remove(client.m_uiApplicationID);
    SendUploadResult(client.m_Channel, request, false);
    return;
  }
  upload.m_uiReceived += uiChunkSize;

  xiiFileserverEvent e;
  e.m_Type        = xiiFileserverEvent::Type::FileUploading;
  e.m_uiClientID  = client.m_uiApplicationID;
  e.m_szPath      = upload.m_sRelativePath;
  e.m_uiSentTotal = upload.m_uiReceived;
  e.m_uiSizeTotal = upload.m_uiSize;
  m_Events.Broadcast(e);
}

void xiiFileserver::HandleUploadFileFinished(xiiFileserveClientContext& client, xiiArrayPtr<const xiiUInt8> data)
{
  if (data.GetCount() < sizeof(xiiUuid))
    return;

  xiiRawMemoryStreamReader reader(data.GetPtr(), data.GetCount());
  xiiUuid                  request;
  reader >> request;

  auto* pStored = m_Uploads.GetValue(client.m_uiApplicationID);
  if (pStored == nullptr || (*pStored)->m_Id != request)
  {
    SendUploadResult(client.m_Channel, request, false);
    return;
  }

  xiiUniquePtr<Upload> pUpload;
  m_Uploads.Remove(client.m_uiApplicationID, &pUpload);
  bool bCommitted = false;
  XII_SCOPE_EXIT(SendUploadResult(client.m_Channel, request, bCommitted));

  xiiUInt16        uiDir = 0;
  xiiStringBuilder sRelativePath, sDestination;
  xiiUInt64        uiExpectedHash = 0;
  if (reader.ReadWordValue(&uiDir).Failed() || ReadRequestText(reader, sRelativePath).Failed() ||
      reader.ReadQWordValue(&uiExpectedHash).Failed() || uiDir != pUpload->m_uiDataDir ||
      sRelativePath != pUpload->m_sRelativePath.GetView() || pUpload->m_uiReceived != pUpload->m_uiSize ||
      !client.m_MountedDataDirs[uiDir].m_bWritable ||
      xiiFileserveClientContext::ResolveFilePath(client.m_MountedDataDirs[uiDir].m_sPathOnServer, sRelativePath, sDestination).Failed() ||
      sDestination != pUpload->m_sDestination.GetView())
    return;

  pUpload->m_File.Close();
  xiiOSFile verifiedFile;
  xiiUInt64 uiActualHash = 0;
  if (verifiedFile.Open(pUpload->m_sStagingPath, xiiFileOpenMode::Read).Failed() ||
      xiiFileserveClientContext::HashFile(verifiedFile, pUpload->m_uiSize, uiActualHash).Failed() ||
      uiActualHash != uiExpectedHash)
    return;
  verifiedFile.Close();

  if (xiiOSFile::AtomicReplaceFile(pUpload->m_sStagingPath, pUpload->m_sDestination).Failed())
    return;

  pUpload->m_sStagingPath.Clear();
  bCommitted = true;
  xiiFileserverEvent e;
  e.m_Type        = xiiFileserverEvent::Type::FileUploadFinished;
  e.m_uiClientID  = client.m_uiApplicationID;
  e.m_szPath      = sRelativePath;
  e.m_uiSentTotal = pUpload->m_uiSize;
  e.m_uiSizeTotal = pUpload->m_uiSize;
  m_Events.Broadcast(e);
}

void xiiFileserver::SendUploadResult(xiiNetworkChannel channel, xiiUuid id, bool bSucceeded)
{
  Request response(Operation::UploadResult);
  response.GetWriter() << id;
  response.GetWriter() << bSucceeded;
  SendResponse(channel, response);
}
void xiiFileserver::HandleShaderCompileRequest(xiiFileserveClientContext& client, xiiArrayPtr<const xiiUInt8> data)
{
  if (data.GetCount() < sizeof(xiiUuid))
    return;

  xiiRawMemoryStreamReader reader(data.GetPtr(), data.GetCount());
  xiiUuid                  id;
  reader >> id;
  if (!id.IsValid())
    return;

  xiiStringBuilder sLog("Shader compiler is unavailable or the request is outside the allowed paths and platforms.");
  bool             bSucceeded = false;
  XII_SCOPE_EXIT(
    Request reply(Operation::ShaderResult);
    reply.GetWriter() << id;
    reply.GetWriter() << bSucceeded;
    if (sLog.GetElementCount() > 64 * 1024)
      sLog = "The compiler log exceeded the FileServe response limit.";
    reply.GetWriter() << sLog;
    SendResponse(client.m_Channel, reply));

  xiiStringBuilder sShader, sPlatform;
  xiiUInt32        uiPermutationCount = 0;
  if (!m_ShaderCompileHandler.IsValid() || ReadRequestText(reader, sShader).Failed() || ReadRequestText(reader, sPlatform).Failed() ||
      reader.ReadDWordValue(&uiPermutationCount).Failed() || uiPermutationCount > 64 ||
      !xiiFileserveClientContext::IsValidRelativePath(sShader) || !sShader.HasExtension("xiiShader") || !IsShaderIdentifier(sPlatform))
    return;

  const auto& command          = *xiiCommandLineUtils::GetGlobalInstance();
  const auto  uiPlatforms      = command.GetStringOptionArguments("-fs_shader_platforms");
  bool        bPlatformAllowed = false;
  for (xiiUInt32 i = 0; i < uiPlatforms; ++i)
    bPlatformAllowed |= sPlatform == command.GetStringOption("-fs_shader_platforms", i);
  if (!bPlatformAllowed)
    return;

  xiiStringBuilder sSource;
  bool             bSourceAllowed = false;
  for (const auto& dir : client.m_MountedDataDirs)
  {
    if (dir.m_bMounted && xiiFileserveClientContext::ResolveFilePath(dir.m_sPathOnServer, sShader, sSource).Succeeded() &&
        xiiOSFile::ExistsFile(sSource))
    {
      bSourceAllowed = true;
      break;
    }
  }
  if (!bSourceAllowed)
    return;

  xiiRemoteShaderCompileRequest request;
  request.m_sShader   = sShader;
  request.m_sPlatform = sPlatform;
  xiiStringBuilder sName, sValue;
  for (xiiUInt32 i = 0; i < uiPermutationCount; ++i)
  {
    if (ReadRequestText(reader, sName).Failed() || ReadRequestText(reader, sValue).Failed() ||
        !IsShaderIdentifier(sName) || !IsShaderIdentifier(sValue))
      return;
    request.m_Permutations.PushBack({sName.GetView(), sValue.GetView()});
  }

  xiiFileserverEvent e;
  e.m_Type       = xiiFileserverEvent::Type::LogCustomActivity;
  e.m_uiClientID = client.m_uiApplicationID;
  e.m_szName     = sShader;
  m_Events.Broadcast(e);
  sLog.Clear();
  bSucceeded = m_ShaderCompileHandler(client, request, sLog).Succeeded();
}

xiiResult xiiFileserver::SendConnectionInfo(const char* szClientAddress, xiiUInt16 uiMyPort, const xiiArrayPtr<xiiStringBuilder>& myIPs, xiiTime timeout)
{
  return xiiFileserveDiscovery::Send(szClientAddress, uiMyPort, myIPs, timeout);
}
