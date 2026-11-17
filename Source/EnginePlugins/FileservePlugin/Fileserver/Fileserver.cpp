/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FileservePlugin/FileservePluginPCH.h>

#include <FileservePlugin/Client/FileserveClient.h>
#include <FileservePlugin/Discovery/FileserveDiscovery.h>
#include <FileservePlugin/Fileserver/Fileserver.h>
#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Communication/RemoteInterfaceEnet.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Strings/UnicodeUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <cstring>

XII_IMPLEMENT_SINGLETON(xiiFileserver);

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

void xiiFileserver::StartServer()
{
  if (m_pNetwork)
    return;

  xiiStringBuilder tmp;

  m_pNetwork = xiiRemoteInterfaceEnet::Make();
  if (m_pNetwork->StartServer('XIFS', xiiConversionUtils::ToString(m_uiPort, tmp), false).Failed())
  {
    m_pNetwork.Clear();
    xiiLog::Error("Could not listen on FileServe port {}.", m_uiPort);
    return;
  }
  m_pNetwork->SetMessageHandler('FSRV', xiiMakeDelegate(&xiiFileserver::NetworkMsgHandler, this));
  m_pNetwork->SetMessageHandler('SHDR', xiiMakeDelegate(&xiiFileserver::HandleShaderCompileRequest, this));
  m_pNetwork->m_RemoteEvents.AddEventHandler(xiiMakeDelegate(&xiiFileserver::NetworkEventHandler, this));

  xiiFileserverEvent e;
  e.m_Type = xiiFileserverEvent::Type::ServerStarted;
  m_Events.Broadcast(e);
}

void xiiFileserver::StopServer()
{
  if (!m_pNetwork)
    return;

  m_pNetwork->ShutdownConnection();
  m_pNetwork.Clear();
  m_Uploads.Clear();
  m_Clients.Clear();

  xiiFileserverEvent e;
  e.m_Type = xiiFileserverEvent::Type::ServerStopped;
  m_Events.Broadcast(e);
}

bool xiiFileserver::UpdateServer()
{
  if (!m_pNetwork)
    return false;

  const auto now = xiiTime::Now();
  for (auto it = m_Uploads.GetIterator(); it.IsValid();)
  {
    if (it.Value()->m_Deadline <= now)
      it = m_Uploads.Remove(it);
    else
      ++it;
  }
  m_pNetwork->UpdateRemoteInterface();
  return m_pNetwork->ExecuteAllMessageHandlers() > 0;
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

  m_pNetwork->Send('FSRV', 'RLDR');
}

void xiiFileserver::NetworkMsgHandler(xiiRemoteMessage& msg)
{
  auto& client = DetermineClient(msg);

  if (msg.GetMessageID() == 'HELO')
    return;

  if (msg.GetMessageID() == 'RUTR')
  {
    // 'are you there' is used to check whether a certain address is a proper Fileserver
    m_pNetwork->Send('FSRV', ' YES');

    xiiFileserverEvent e;
    e.m_Type = xiiFileserverEvent::Type::AreYouThereRequest;
    m_Events.Broadcast(e);
    return;
  }

  if (msg.GetMessageID() == 'READ')
  {
    HandleFileRequest(client, msg);
    return;
  }

  if (msg.GetMessageID() == 'UPLH')
  {
    HandleUploadFileHeader(client, msg);
    return;
  }

  if (msg.GetMessageID() == 'UPLD')
  {
    HandleUploadFileTransfer(client, msg);
    return;
  }

  if (msg.GetMessageID() == 'UPLF')
  {
    HandleUploadFileFinished(client, msg);
    return;
  }

  if (msg.GetMessageID() == 'DELF')
  {
    HandleDeleteFileRequest(client, msg);
    return;
  }

  if (msg.GetMessageID() == ' MNT')
  {
    HandleMountRequest(client, msg);
    return;
  }

  if (msg.GetMessageID() == 'UMNT')
  {
    HandleUnmountRequest(client, msg);
    return;
  }

  xiiLog::Error("Unknown FSRV message: '{0}' - {1} bytes", msg.GetMessageID(), msg.GetMessageData().GetCount());
}

void xiiFileserver::SetShaderCompileHandler(ShaderCompileHandler handler)
{
  m_ShaderCompileHandler = std::move(handler);
}

void xiiFileserver::NetworkEventHandler(const xiiRemoteEvent& e)
{
  switch (e.m_Type)
  {
    case xiiRemoteEvent::DisconnectedFromClient:
    {
      m_Uploads.Remove(e.m_uiOtherAppID);
      if (m_Clients.Contains(e.m_uiOtherAppID))
      {
        xiiFileserverEvent se;
        se.m_Type       = xiiFileserverEvent::Type::ClientDisconnected;
        se.m_uiClientID = e.m_uiOtherAppID;

        m_Events.Broadcast(se);

        m_Clients[e.m_uiOtherAppID].m_bLostConnection = true;
      }
    }
    break;

    default:
      break;
  }
}

xiiFileserveClientContext& xiiFileserver::DetermineClient(xiiRemoteMessage& msg)
{
  xiiFileserveClientContext& client = m_Clients[msg.GetApplicationID()];

  if (client.m_uiApplicationID != msg.GetApplicationID())
  {
    client.m_uiApplicationID = msg.GetApplicationID();

    xiiFileserverEvent e;
    e.m_Type       = xiiFileserverEvent::Type::ClientConnected;
    e.m_uiClientID = client.m_uiApplicationID;
    m_Events.Broadcast(e);
  }
  else if (client.m_bLostConnection)
  {
    client.m_bLostConnection = false;

    xiiFileserverEvent e;
    e.m_Type       = xiiFileserverEvent::Type::ClientReconnected;
    e.m_uiClientID = client.m_uiApplicationID;
    m_Events.Broadcast(e);
  }

  return client;
}

void xiiFileserver::HandleMountRequest(xiiFileserveClientContext& client, xiiRemoteMessage& msg)
{
  xiiStringBuilder sDataDir, sRootName, sMountPoint, sRedir;
  xiiUInt16        uiDataDirID = 0xffff;

  auto& reader = msg.GetReader();
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


void xiiFileserver::HandleUnmountRequest(xiiFileserveClientContext& client, xiiRemoteMessage& msg)
{
  xiiUInt16 uiDataDirID = 0xffff;
  if (msg.GetReader().ReadWordValue(&uiDataDirID).Failed() || uiDataDirID >= client.m_MountedDataDirs.GetCount())
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

void xiiFileserver::HandleFileRequest(xiiFileserveClientContext& client, xiiRemoteMessage& msg)
{
  xiiUInt16 uiDataDirID       = 0;
  bool      bForceThisDataDir = false;

  auto&                                 reader  = msg.GetReader();
  xiiUInt8                              uiForce = 0;
  xiiStringBuilder sRequestedFile;
  xiiUInt64                             uiIdLow  = 0;
  xiiUInt64                             uiIdHigh = 0;
  xiiFileserveClientContext::FileStatus status;
  if (reader.ReadWordValue(&uiDataDirID).Failed() || reader.ReadBytes(&uiForce, sizeof(uiForce)) != sizeof(uiForce) ||
      uiForce > 1 || ReadRequestText(reader, sRequestedFile).Failed() ||
      reader.ReadQWordValue(&uiIdHigh).Failed() || reader.ReadQWordValue(&uiIdLow).Failed() ||
      reader.ReadQWordValue(&status.m_iTimestamp).Failed() || reader.ReadQWordValue(&status.m_uiHash).Failed())
    return;

  const xiiUuid downloadGuid(uiIdLow, uiIdHigh);
  bForceThisDataDir = uiForce != 0;

  xiiFileserverEvent e;
  e.m_uiClientID  = client.m_uiApplicationID;
  e.m_szPath      = sRequestedFile;
  e.m_uiSentTotal = 0;

  xiiOSFile file;
  auto      filestate = client.GetFileStatus(uiDataDirID, sRequestedFile, status, file, bForceThisDataDir);

  {
    e.m_Type        = xiiFileserverEvent::Type::FileDownloadRequest;
    e.m_uiSizeTotal = static_cast<xiiUInt32>(status.m_uiFileSize);
    e.m_FileState   = filestate;
    m_Events.Broadcast(e);
  }

  if (filestate == xiiFileserveFileState::Different)
  {
    xiiUInt32       uiNextByte = 0;
    const xiiUInt32 uiFileSize = static_cast<xiiUInt32>(status.m_uiFileSize);
    xiiUInt8        bytes[16 * 1024];

    // Even empty files announce their size so the client stages an empty file.
    do
    {
      const auto uiChunkSize = static_cast<xiiUInt16>(xiiMath::Min<xiiUInt32>(sizeof(bytes), uiFileSize - uiNextByte));
      if (uiChunkSize != 0 && file.Read(bytes, uiChunkSize) != uiChunkSize)
      {
        filestate = xiiFileserveFileState::None;
        break;
      }

      xiiRemoteMessage ret;
      ret.GetWriter() << downloadGuid;
      ret.GetWriter() << uiChunkSize;
      ret.GetWriter() << uiFileSize;

      if (uiChunkSize != 0 && ret.GetWriter().WriteBytes(bytes, uiChunkSize).Failed())
      {
        filestate = xiiFileserveFileState::None;
        break;
      }

      ret.SetMessageID('FSRV', 'DWNL');
      m_pNetwork->Send(xiiRemoteTransmitMode::Reliable, ret);

      uiNextByte += uiChunkSize;

      // reuse previous values
      {
        e.m_Type        = xiiFileserverEvent::Type::FileDownloading;
        e.m_uiSentTotal = uiNextByte;
        m_Events.Broadcast(e);
      }
    } while (uiNextByte < uiFileSize);
  }

  // final answer to client
  {
    xiiRemoteMessage ret('FSRV', 'DWNF');
    ret.GetWriter() << downloadGuid;
    ret.GetWriter() << (xiiInt8)filestate;
    ret.GetWriter() << status.m_iTimestamp;
    ret.GetWriter() << status.m_uiHash;
    ret.GetWriter() << uiDataDirID;

    m_pNetwork->Send(xiiRemoteTransmitMode::Reliable, ret);
  }

  // reuse previous values
  {
    e.m_Type = xiiFileserverEvent::Type::FileDownloadFinished;
    m_Events.Broadcast(e);
  }
}

void xiiFileserver::HandleDeleteFileRequest(xiiFileserveClientContext& client, xiiRemoteMessage& msg)
{
  xiiUInt16 uiDataDirID = 0xffff;
  xiiStringBuilder sFile;
  auto&            reader = msg.GetReader();
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

void xiiFileserver::HandleUploadFileHeader(xiiFileserveClientContext& client, xiiRemoteMessage& msg)
{
  if (msg.GetMessageData().GetCount() < sizeof(xiiUuid))
    return;

  xiiUuid request;
  auto&   reader = msg.GetReader();
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
    SendUploadResult(request, false);
    return;
  }

  auto pUpload             = XII_DEFAULT_NEW(Upload);
  pUpload->m_Id            = request;
  pUpload->m_uiSize        = uiSize;
  pUpload->m_uiDataDir     = uiDir;
  pUpload->m_sRelativePath = sRelativePath;
  pUpload->m_sDestination  = sDestination;
  pUpload->m_Deadline      = xiiTime::Now() + xiiTime::MakeFromSeconds(120);

  xiiStringBuilder sStagingId;
  xiiConversionUtils::ToString(xiiUuid::MakeUuid(), sStagingId);
  const xiiStringBuilder sStagingPath(sDestination.GetFileDirectory(), "/.xii-upload-", sStagingId, ".tmp");
  if (pUpload->m_File.Open(sStagingPath, xiiFileOpenMode::Write).Failed())
  {
    SendUploadResult(request, false);
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

void xiiFileserver::HandleUploadFileTransfer(xiiFileserveClientContext& client, xiiRemoteMessage& msg)
{
  auto*      pStored = m_Uploads.GetValue(client.m_uiApplicationID);
  const auto bytes   = msg.GetMessageData();
  if (pStored == nullptr || bytes.GetCount() < sizeof(xiiUuid))
    return;

  auto&   upload = **pStored;
  auto&   reader = msg.GetReader();
  xiiUuid request;
  reader >> request;
  if (request != upload.m_Id)
    return;

  xiiUInt16 uiChunkSize = 0;
  constexpr xiiUInt32 uiHeader    = 16 + sizeof(uiChunkSize);
  if (reader.ReadWordValue(&uiChunkSize).Failed() || uiChunkSize == 0 ||
      bytes.GetCount() != uiHeader + uiChunkSize || uiChunkSize > upload.m_uiSize - upload.m_uiReceived ||
      upload.m_File.Write(bytes.GetPtr() + uiHeader, uiChunkSize).Failed())
  {
    m_Uploads.Remove(client.m_uiApplicationID);
    SendUploadResult(request, false);
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

void xiiFileserver::HandleUploadFileFinished(xiiFileserveClientContext& client, xiiRemoteMessage& msg)
{
  if (msg.GetMessageData().GetCount() < sizeof(xiiUuid))
    return;

  auto&   reader = msg.GetReader();
  xiiUuid request;
  reader >> request;

  auto* pStored = m_Uploads.GetValue(client.m_uiApplicationID);
  if (pStored == nullptr || (*pStored)->m_Id != request)
  {
    SendUploadResult(request, false);
    return;
  }

  xiiUniquePtr<Upload> pUpload;
  m_Uploads.Remove(client.m_uiApplicationID, &pUpload);
  bool bCommitted = false;
  XII_SCOPE_EXIT(SendUploadResult(request, bCommitted));

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

void xiiFileserver::SendUploadResult(xiiUuid id, bool bSucceeded)
{
  xiiRemoteMessage response('FSRV', 'UACK');
  response.GetWriter() << id;
  response.GetWriter() << bSucceeded;
  m_pNetwork->Send(xiiRemoteTransmitMode::Reliable, response);
}
void xiiFileserver::HandleShaderCompileRequest(xiiRemoteMessage& msg)
{
  if (msg.GetMessageID() != 'CMPL' || msg.GetMessageData().GetCount() < sizeof(xiiUuid))
    return;

  auto&   reader = msg.GetReader();
  xiiUuid id;
  reader >> id;
  if (!id.IsValid())
    return;

  xiiStringBuilder sLog("Shader compiler is unavailable or the request is outside the allowed paths and platforms.");
  bool             bSucceeded = false;
  XII_SCOPE_EXIT(
    xiiRemoteMessage reply('SHDR', 'CRES');
    reply.GetWriter() << id;
    reply.GetWriter() << bSucceeded;
    if (sLog.GetElementCount() > 64 * 1024)
      sLog = "The compiler log exceeded the FileServe response limit.";
    reply.GetWriter() << sLog;
    m_pNetwork->Send(xiiRemoteTransmitMode::Reliable, reply));

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

  auto&            client = DetermineClient(msg);
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
