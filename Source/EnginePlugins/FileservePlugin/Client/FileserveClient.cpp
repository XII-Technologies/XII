/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FileservePlugin/FileservePluginPCH.h>

#include <FileservePlugin/Client/FileserveClient.h>
#include <FileservePlugin/Discovery/FileserveDiscovery.h>
#include <FileservePlugin/Fileserver/ClientContext.h>
#include <FileservePlugin/Implementation/FileserveTransfer.h>
#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Events/GlobalEvent.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/UnicodeUtils.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <cstring>

using xiiFileserveTransfer::Operation;
using xiiFileserveTransfer::Request;

XII_IMPLEMENT_SINGLETON(xiiFileserveClient);

bool xiiFileserveClient::s_bEnableFileserve = true;

xiiFileserveClient::xiiFileserveClient() :
  m_SingletonRegistrar(this)
{
  AddServerAddressToTry("localhost:1042");

  xiiStringBuilder sAddress, sSearch;

  // the app directory
  {
    sSearch = xiiOSFile::GetApplicationDirectory();
    sSearch.AppendPath("xiiFileserve.txt");

    if (TryReadFileserveConfig(sSearch, sAddress).Succeeded())
    {
      AddServerAddressToTry(sAddress);
    }
  }

  // command line argument
  AddServerAddressToTry(xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-fs_server", 0, ""));

  // last successful IP is stored in the user directory
  {
    sSearch = xiiOSFile::GetUserDataFolder("xiiFileserve.txt");

    if (TryReadFileserveConfig(sSearch, sAddress).Succeeded())
    {
      AddServerAddressToTry(sAddress);
    }
  }

  if (xiiCommandLineUtils::GetGlobalInstance()->GetBoolOption("-fs_off"))
    s_bEnableFileserve = false;
}

xiiFileserveClient::~xiiFileserveClient()
{
  ShutdownConnection();
}

void xiiFileserveClient::ShutdownConnection()
{
  ClearState();
  if (m_pNetwork)
  {
    xiiLog::Dev("Shutting down fileserve client");

    m_pNetwork->Shutdown();
    m_Channel  = {};
    m_pNetwork = nullptr;
  }
}

void xiiFileserveClient::ClearState()
{
  m_bDownloading              = false;
  m_bWaitingForUploadFinished = false;
  m_UploadRequest             = {};
  m_bReloadResources          = false;
  m_CurFileRequestGuid        = xiiUuid();
  m_sCurFileRequest.Clear();
  DiscardDownload();
}

xiiResult xiiFileserveClient::EnsureConnected(xiiTime timeout)
{
  XII_LOCK(m_Mutex);
  if (!s_bEnableFileserve || m_bFailedToConnect)
    return XII_FAILURE;

  if (m_pNetwork == nullptr)
  {
    xiiStringBuilder sServerCache;
    sServerCache.SetFormat("{0}", xiiArgU(xiiHashingUtils::xxHash64String(m_sServerConnectionAddress), 16, true, 16));
    xiiStringBuilder sCacheRoot = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-fs_cache_dir");
    if (sCacheRoot.IsEmpty())
      sCacheRoot = xiiOSFile::GetUserDataFolder("xiiFileserve");
    if (!sCacheRoot.IsAbsolutePath())
      return XII_FAILURE;
    m_sFileserveCacheFolder     = xiiStringBuilder(sCacheRoot, "/Cache/", sServerCache);
    m_sFileserveCacheMetaFolder = xiiStringBuilder(sCacheRoot, "/Meta/", sServerCache);

    if (xiiOSFile::CreateDirectoryStructure(m_sFileserveCacheFolder).Failed())
    {
      xiiLog::Error("Could not create fileserve cache folder '{0}'", m_sFileserveCacheFolder);
      return XII_FAILURE;
    }

    if (xiiOSFile::CreateDirectoryStructure(m_sFileserveCacheMetaFolder).Failed())
    {
      xiiLog::Error("Could not create fileserve cache folder '{0}'", m_sFileserveCacheMetaFolder);
      return XII_FAILURE;
    }
  }

  if (m_Channel.IsValid())
    return XII_SUCCESS;

  ClearState();
  m_bFailedToConnect = true;
  if (timeout.GetSeconds() < 0)
    timeout = xiiTime::MakeFromSeconds(xiiCommandLineUtils::GetGlobalInstance()->GetFloatOption("-fs_timeout", -timeout.GetSeconds()));
  if (!timeout.IsPositive() || timeout > xiiTime::MakeFromSeconds(120))
    timeout = xiiTime::MakeFromSeconds(120);

  xiiUniquePtr<xiiNetworkHost> host = XII_DEFAULT_NEW(xiiNetworkHost);
  XII_SUCCEED_OR_RETURN(xiiFileserveTransfer::Start(*host, false));
  xiiNetworkConnection connection;
  XII_SUCCEED_OR_RETURN(xiiFileserveTransfer::Connect(*host, m_sServerConnectionAddress, connection).GetResult());
  const auto      deadline = xiiTime::Now() + timeout;
  xiiNetworkEvent event;
  while (xiiTime::Now() < deadline && host->WaitForEvent(event, deadline - xiiTime::Now()).Succeeded())
  {
    if (event.m_Connection != connection)
      continue;
    if (event.m_Type == xiiNetworkEventType::Closed)
      break;
    if (event.m_Type == xiiNetworkEventType::Connected && event.m_pSessionInfo)
    {
      m_Channel  = event.m_pSessionInfo->m_DefaultChannel;
      m_pNetwork = std::move(host);
      for (xiiUInt16 i = 0; i < m_MountedDataDirs.GetCount(); ++i)
      {
        if (m_MountedDataDirs[i].m_bMounted && SendMount(i, m_MountedDataDirs[i]).Failed())
        {
          ShutdownConnection();
          return XII_FAILURE;
        }
      }
      InvalidateAllCache();
      m_bFailedToConnect = false;
      xiiLog::Success("Connected to FileServe '{}'.", m_sServerConnectionAddress);
      return XII_SUCCESS;
    }
  }
  xiiLog::Error("FileServe connection to '{}' failed. {}", m_sServerConnectionAddress, event.m_Error.GetCodeName());
  return XII_FAILURE;
}

void xiiFileserveClient::ProcessNetworkEvents(xiiTime wait)
{
  if (!m_pNetwork)
    return;
  xiiNetworkEvent event;
  bool            bEvent = wait.IsPositive() ? m_pNetwork->WaitForEvent(event, wait).Succeeded() : m_pNetwork->TryGetEvent(event);
  for (xiiUInt32 i = 0; bEvent && i < 128; ++i)
  {
    if (event.m_Connection == m_Channel.m_Connection)
    {
      if (event.m_Type == xiiNetworkEventType::Closed)
      {
        m_Channel = {};
        ClearState();
        return;
      }
      if (event.m_Type == xiiNetworkEventType::Message && event.m_Channel == m_Channel)
      {
        const auto               bytes = event.m_Payload.GetBytes();
        xiiRawMemoryStreamReader reader(bytes.GetPtr(), bytes.GetCount());
        Operation                operation;
        if (xiiFileserveTransfer::ReadOperation(reader, operation).Succeeded())
          NetworkMsgHandler(operation, bytes.GetSubArray(1));
        else
          m_pNetwork->Disconnect(m_Channel.m_Connection, false).IgnoreResult();
      }
    }
    if (i + 1 < 128)
      bEvent = m_pNetwork->TryGetEvent(event);
  }
}

xiiResult xiiFileserveClient::SendRequest(const Request& request, xiiTime deadline)
{
  while (m_Channel.IsValid() && xiiTime::Now() < deadline)
  {
    const auto error = request.Send(*m_pNetwork, m_Channel);
    if (error.Succeeded())
      return XII_SUCCESS;
    if (error.m_Code != xiiNetworkErrorCode::Backpressure && error.m_Code != xiiNetworkErrorCode::WouldBlock &&
        error.m_Code != xiiNetworkErrorCode::ResourceExhausted)
      return XII_FAILURE;
    ProcessNetworkEvents(deadline - xiiTime::Now());
  }
  return XII_FAILURE;
}

void xiiFileserveClient::UpdateClient()
{
  bool bReloadResources = false;
  {
    XII_LOCK(m_Mutex);
    if (m_pNetwork == nullptr || m_bFailedToConnect || !s_bEnableFileserve)
      return;

    ProcessNetworkEvents();
    if (!m_Channel.IsValid())
    {
      if (EnsureConnected().Failed())
      {
        xiiLog::Error("Fileserve connection was lost and could not be re-established.");
        ShutdownConnection();
      }
      return;
    }

    if (m_bReloadResources && !m_bDownloading && !m_bWaitingForUploadFinished)
    {
      m_bReloadResources = false;
      bReloadResources   = true;
    }
  }

  if (bReloadResources)
    XII_BROADCAST_EVENT(xiiResourceManager_ReloadAllResources);
}

void xiiFileserveClient::AddServerAddressToTry(xiiStringView sAddress)
{
  XII_LOCK(m_Mutex);
  if (sAddress.IsEmpty())
    return;

  if (m_TryServerAddresses.Contains(sAddress))
    return;

  m_TryServerAddresses.PushBack(sAddress);

  // always set the most recent address as the default one
  m_sServerConnectionAddress = sAddress;
}

bool xiiFileserveClient::IsShaderCompilationAvailable() const
{
  XII_LOCK(m_Mutex);
  return m_pNetwork && m_Channel.IsValid();
}

xiiResult xiiFileserveClient::CompileShader(const xiiRemoteShaderCompileRequest& request, xiiStringBuilder& out_sLog)
{
  XII_LOCK(m_Mutex);
  out_sLog = "Invalid or unavailable FileServe shader compilation request.";
  if (!m_pNetwork || !m_Channel.IsValid() || m_bShaderCompiling || m_bDownloading ||
      m_bWaitingForUploadFinished || request.m_Permutations.GetCount() > 64 ||
      !xiiFileserveClientContext::IsValidRelativePath(request.m_sShader) || request.m_sPlatform.GetElementCount() > 96)
    return XII_FAILURE;

  for (const auto& permutation : request.m_Permutations)
  {
    if (permutation.m_sName.GetElementCount() > 96 || permutation.m_sValue.GetElementCount() > 96)
      return XII_FAILURE;
  }

  const auto id       = xiiUuid::MakeUuid();
  m_bShaderCompiling  = true;
  bool      bComplete = false;
  xiiResult result    = XII_FAILURE;
  m_ShaderResponse    = [&](xiiArrayPtr<const xiiUInt8> bytes) {
    constexpr xiiUInt32 uiHeaderSize = 16 + sizeof(xiiUInt8) + sizeof(xiiUInt32);
    if (bComplete || bytes.GetCount() < uiHeaderSize)
      return;

    xiiRawMemoryStreamReader reader(bytes.GetPtr(), bytes.GetCount());
    xiiUuid                  responseId;
    reader >> responseId;
    if (responseId != id)
      return;

    bComplete             = true;
    xiiUInt8  uiSucceeded = 0;
    xiiUInt32 uiLogBytes  = 0;
    if (reader.ReadBytes(&uiSucceeded, sizeof(uiSucceeded)) != sizeof(uiSucceeded) ||
        reader.ReadDWordValue(&uiLogBytes).Failed() || uiSucceeded > 1 || uiLogBytes > 64 * 1024 ||
        uiLogBytes != bytes.GetCount() - uiHeaderSize)
      return;

    const auto* pLog = reinterpret_cast<const char*>(bytes.GetPtr() + uiHeaderSize);
    if (!xiiUnicodeUtils::IsValidUtf8(pLog, pLog + uiLogBytes) || std::memchr(pLog, '\0', uiLogBytes) != nullptr)
      return;
    out_sLog = xiiStringView(pLog, pLog + uiLogBytes);
    result   = uiSucceeded == 1 ? XII_SUCCESS : XII_FAILURE;
  };
  XII_SCOPE_EXIT(m_ShaderResponse = {}; m_bShaderCompiling = false);

  Request msg(Operation::CompileShader);
  msg.GetWriter() << id;
  msg.GetWriter() << request.m_sShader;
  msg.GetWriter() << request.m_sPlatform;
  msg.GetWriter() << request.m_Permutations.GetCount();
  for (const auto& permutation : request.m_Permutations)
  {
    msg.GetWriter() << permutation.m_sName;
    msg.GetWriter() << permutation.m_sValue;
  }
  const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(120);
  XII_SUCCEED_OR_RETURN(SendRequest(msg, deadline));
  while (!bComplete && m_Channel.IsValid() && xiiTime::Now() < deadline)
  {
    ProcessNetworkEvents(deadline - xiiTime::Now());
  }
  if (!bComplete)
    out_sLog = "FileServe disconnected or the shader compilation deadline expired.";
  if (result.Succeeded())
    InvalidateAllCache();
  return result;
}

xiiResult xiiFileserveClient::UploadFile(xiiUInt16 uiDataDirID, xiiStringView sFile, xiiOSFile& ref_file)
{
  XII_LOCK(m_Mutex);
  if (!m_pNetwork || !m_Channel.IsValid() || m_bDownloading || m_bWaitingForUploadFinished ||
      uiDataDirID >= m_MountedDataDirs.GetCount() || !m_MountedDataDirs[uiDataDirID].m_bMounted ||
      !xiiFileserveClientContext::IsValidRelativePath(sFile))
    return XII_FAILURE;

  const auto uiFileSize = ref_file.GetFileSize();
  if (uiFileSize > xiiMath::MaxValue<xiiUInt32>())
    return XII_FAILURE;

  // The local writer has already changed the cached payload. Until the server confirms the
  // upload, neither the in-memory cache nor its metadata can describe the new contents.
  const xiiStringBuilder sFilePath(sFile);
  xiiStringBuilder       sCachedMetaFile;
  BuildPathInCache(sFilePath, m_MountedDataDirs[uiDataDirID].m_sMountPoint, nullptr, &sCachedMetaFile);
  InvalidateFileCache(uiDataDirID, sFile, 0);
  if (xiiOSFile::DeleteFile(sCachedMetaFile).Failed())
    return XII_FAILURE;

  m_UploadRequest             = xiiUuid::MakeUuid();
  m_uiUploadAcknowledged      = 0;
  m_bUploadSucceeded          = false;
  m_bWaitingForUploadFinished = true;
  XII_SCOPE_EXIT(m_bWaitingForUploadFinished = false; m_UploadRequest = {});
  const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(120);

  {
    Request msg(Operation::UploadBegin);
    msg.GetWriter() << m_UploadRequest;
    msg.GetWriter() << static_cast<xiiUInt32>(uiFileSize);
    msg.GetWriter() << uiDataDirID;
    msg.GetWriter() << sFile;
    XII_SUCCEED_OR_RETURN(SendRequest(msg, deadline));
  }

  xiiHashStreamWriter64 hash(1);
  xiiUInt8              bytes[16 * 1024];
  xiiUInt64             uiRemaining = uiFileSize;
  while (uiRemaining != 0 && m_bWaitingForUploadFinished && m_Channel.IsValid() && xiiTime::Now() < deadline)
  {
    const auto uiChunkSize = static_cast<xiiUInt16>(xiiMath::Min<xiiUInt64>(sizeof(bytes), uiRemaining));
    if (ref_file.Read(bytes, uiChunkSize) != uiChunkSize || hash.WriteBytes(bytes, uiChunkSize).Failed())
      return XII_FAILURE;

    Request msg(Operation::UploadChunk);
    msg.GetWriter() << m_UploadRequest;
    msg.GetWriter() << uiChunkSize;
    XII_SUCCEED_OR_RETURN(msg.GetWriter().WriteBytes(bytes, uiChunkSize));
    XII_SUCCEED_OR_RETURN(SendRequest(msg, deadline));
    ProcessNetworkEvents();
    uiRemaining -= uiChunkSize;
    const auto uiSent = uiFileSize - uiRemaining;
    if (uiRemaining != 0 && uiSent % xiiFileserveTransfer::TransferWindowBytes == 0)
    {
      while (m_uiUploadAcknowledged < uiSent && m_bWaitingForUploadFinished && m_Channel.IsValid() && xiiTime::Now() < deadline)
        ProcessNetworkEvents(deadline - xiiTime::Now());
      if (m_uiUploadAcknowledged != uiSent)
        return XII_FAILURE;
    }
  }

  if (uiRemaining != 0 || !m_bWaitingForUploadFinished || !m_Channel.IsValid())
    return XII_FAILURE;

  const xiiUInt64 uiHash = uiFileSize == 0 ? 1 : hash.GetHashValue();
  {
    Request msg(Operation::UploadEnd);
    msg.GetWriter() << m_UploadRequest;
    msg.GetWriter() << uiDataDirID;
    msg.GetWriter() << sFile;
    msg.GetWriter() << uiHash;

    XII_SUCCEED_OR_RETURN(SendRequest(msg, deadline));
  }

  while (m_bWaitingForUploadFinished && m_Channel.IsValid() && xiiTime::Now() < deadline)
  {
    ProcessNetworkEvents(deadline - xiiTime::Now());
  }

  if (!m_bUploadSucceeded)
    return XII_FAILURE;

  if (WriteMetaFile(sCachedMetaFile, 0, uiHash).Failed())
  {
    xiiOSFile::DeleteFile(sCachedMetaFile).IgnoreResult();
    xiiLog::Warning("Could not write FileServe cache metadata for '{}'.", sFile);
  }
  InvalidateFileCache(uiDataDirID, sFile, uiHash);
  return XII_SUCCESS;
}

void xiiFileserveClient::InvalidateFileCache(xiiUInt16 uiDataDirID, xiiStringView sFile, xiiUInt64 uiHash)
{
  XII_LOCK(m_Mutex);
  auto& cache       = m_MountedDataDirs[uiDataDirID].m_CacheStatus[sFile];
  cache.m_FileHash  = uiHash;
  cache.m_TimeStamp = 0;
  cache.m_LastCheck = xiiTime::MakeZero(); // will trigger a server request and that in turn will update the file timestamp

  // redirect the next access to this cache entry
  // together with the zero LastCheck that will make sure the best match gets updated as well
  m_FileDataDir[sFile] = uiDataDirID;
}

void xiiFileserveClient::FillFileStatusCache(const char* szFile)
{
  XII_LOCK(m_Mutex);
  auto it    = m_FileDataDir.FindOrAdd(szFile);
  it.Value() = 0xffff; // does not exist

  for (xiiUInt16 i = static_cast<xiiUInt16>(m_MountedDataDirs.GetCount()); i > 0; --i)
  {
    const xiiUInt16 dd = i - 1;

    if (!m_MountedDataDirs[dd].m_bMounted)
      continue;

    auto& cache = m_MountedDataDirs[dd].m_CacheStatus[szFile];

    DetermineCacheStatus(dd, szFile, cache);
    cache.m_LastCheck = xiiTime::MakeZero();

    if (cache.m_TimeStamp != 0 && cache.m_FileHash != 0) // file exists
    {
      // best possible candidate
      if (it.Value() == 0xffff)
        it.Value() = dd;
    }
  }

  if (it.Value() == 0xffff)
    it.Value() = 0; // fallback
}

void xiiFileserveClient::BuildPathInCache(const char* szFile, const char* szMountPoint, xiiStringBuilder* out_pAbsPath, xiiStringBuilder* out_pFullPathMeta) const
{
  XII_ASSERT_DEV(!xiiPathUtils::IsAbsolutePath(szFile), "Invalid path");
  XII_LOCK(m_Mutex);
  if (out_pAbsPath)
  {
    *out_pAbsPath = m_sFileserveCacheFolder;
    out_pAbsPath->AppendPath(szMountPoint, szFile);
    out_pAbsPath->MakeCleanPath();
  }
  if (out_pFullPathMeta)
  {
    *out_pFullPathMeta = m_sFileserveCacheMetaFolder;
    out_pFullPathMeta->AppendPath(szMountPoint, szFile);
    out_pFullPathMeta->MakeCleanPath();
  }
}

void xiiFileserveClient::ComputeDataDirMountPoint(xiiStringView sDataDir, xiiStringBuilder& out_sMountPoint)
{
  XII_ASSERT_DEV(sDataDir.IsEmpty() || sDataDir.EndsWith("/"), "Invalid path");

  const xiiUInt32 uiMountPoint = xiiHashingUtils::xxHash32String(sDataDir);
  out_sMountPoint.SetFormat("{0}", xiiArgU(uiMountPoint, 8, true, 16));
}

void xiiFileserveClient::GetFullDataDirCachePath(const char* szDataDir, xiiStringBuilder& out_sFullPath, xiiStringBuilder& out_sFullPathMeta) const
{
  XII_LOCK(m_Mutex);
  xiiStringBuilder sMountPoint;
  ComputeDataDirMountPoint(szDataDir, sMountPoint);

  out_sFullPath = m_sFileserveCacheFolder;
  out_sFullPath.AppendPath(sMountPoint);

  out_sFullPathMeta = m_sFileserveCacheMetaFolder;
  out_sFullPathMeta.AppendPath(sMountPoint);
}

void xiiFileserveClient::NetworkMsgHandler(Operation operation, xiiArrayPtr<const xiiUInt8> data)
{
  switch (operation)
  {
    case Operation::DownloadChunk:
      HandleFileTransferMsg(data);
      break;
    case Operation::DownloadResult:
      HandleFileTransferFinishedMsg(data);
      break;
    case Operation::Reload:
      if (data.IsEmpty())
      {
        InvalidateAllCache();
        m_bReloadResources = true;
      }
      break;
    case Operation::UploadResult:
      if (m_bWaitingForUploadFinished && data.GetCount() == 16 + sizeof(xiiUInt8))
      {
        xiiRawMemoryStreamReader reader(data.GetPtr(), data.GetCount());
        xiiUuid                  request;
        reader >> request;
        xiiUInt8 uiSuccess = 0;
        if (request == m_UploadRequest && reader.ReadBytes(&uiSuccess, sizeof(uiSuccess)) == sizeof(uiSuccess))
        {
          m_bWaitingForUploadFinished = false;
          m_bUploadSucceeded          = uiSuccess == 1;
        }
      }
      break;
    case Operation::UploadProgress:
      if (m_bWaitingForUploadFinished && data.GetCount() == 16 + sizeof(xiiUInt32))
      {
        xiiRawMemoryStreamReader reader(data.GetPtr(), data.GetCount());
        xiiUuid                  request;
        xiiUInt32                uiReceived = 0;
        reader >> request;
        if (request == m_UploadRequest && reader.ReadDWordValue(&uiReceived).Succeeded() &&
            uiReceived > m_uiUploadAcknowledged && uiReceived - m_uiUploadAcknowledged == xiiFileserveTransfer::TransferWindowBytes)
          m_uiUploadAcknowledged = uiReceived;
      }
      break;
    case Operation::ShaderResult:
      if (m_ShaderResponse.IsValid())
        m_ShaderResponse(data);
      break;
    default:
      m_pNetwork->Disconnect(m_Channel.m_Connection, false).IgnoreResult();
      break;
  }
}

void xiiFileserveClient::InvalidateAllCache()
{
  for (auto& dir : m_MountedDataDirs)
  {
    for (auto& entry : dir.m_CacheStatus)
      entry.Value().m_LastCheck = xiiTime::MakeZero();
  }
}

xiiUInt16 xiiFileserveClient::MountDataDirectory(xiiStringView sDataDirectory, xiiStringView sRootName)
{
  XII_LOCK(m_Mutex);
  if (!m_pNetwork || !m_Channel.IsValid() || m_MountedDataDirs.GetCount() >= 4096)
    return 0xffff;

  xiiStringBuilder sRoot = sRootName;
  sRoot.Trim(":/");

  xiiStringBuilder sMountPoint;
  ComputeDataDirMountPoint(sDataDirectory, sMountPoint);

  const xiiUInt16 uiDataDirID = static_cast<xiiUInt16>(m_MountedDataDirs.GetCount());

  DataDir dir;
  dir.m_sPathOnClient = sDataDirectory;
  dir.m_sRootName     = sRoot;
  dir.m_sMountPoint   = sMountPoint;
  dir.m_bMounted      = true;
  if (SendMount(uiDataDirID, dir).Failed())
    return 0xffff;
  m_MountedDataDirs.PushBack(std::move(dir));
  return uiDataDirID;
}

xiiResult xiiFileserveClient::SendMount(xiiUInt16 uiDataDirID, const DataDir& dir)
{
  Request request(Operation::Mount);
  request.GetWriter() << dir.m_sPathOnClient;
  request.GetWriter() << dir.m_sRootName;
  request.GetWriter() << dir.m_sMountPoint;
  request.GetWriter() << uiDataDirID;
  return SendRequest(request, xiiTime::Now() + xiiTime::MakeFromSeconds(3));
}

void xiiFileserveClient::UnmountDataDirectory(xiiUInt16 uiDataDir)
{
  XII_LOCK(m_Mutex);
  if (uiDataDir >= m_MountedDataDirs.GetCount() || !m_MountedDataDirs[uiDataDir].m_bMounted)
    return;

  if (m_pNetwork && m_Channel.IsValid())
  {
    Request msg(Operation::Unmount);
    msg.GetWriter() << uiDataDir;
    SendRequest(msg, xiiTime::Now() + xiiTime::MakeFromSeconds(3)).IgnoreResult();
  }

  auto& dd      = m_MountedDataDirs[uiDataDir];
  dd.m_bMounted = false;
}

void xiiFileserveClient::DeleteFile(xiiUInt16 uiDataDir, xiiStringView sFile)
{
  XII_LOCK(m_Mutex);
  if (!m_pNetwork || !m_Channel.IsValid() || uiDataDir >= m_MountedDataDirs.GetCount() ||
      !m_MountedDataDirs[uiDataDir].m_bMounted || !xiiFileserveClientContext::IsValidRelativePath(sFile))
    return;

  InvalidateFileCache(uiDataDir, sFile, 0);

  Request msg(Operation::Delete);
  msg.GetWriter() << uiDataDir;
  msg.GetWriter() << sFile;

  SendRequest(msg, xiiTime::Now() + xiiTime::MakeFromSeconds(3)).IgnoreResult();
}

void xiiFileserveClient::HandleFileTransferMsg(xiiArrayPtr<const xiiUInt8> data)
{
  if (!m_bDownloading || data.GetCount() < sizeof(xiiUuid))
    return;

  xiiRawMemoryStreamReader reader(data.GetPtr(), data.GetCount());
  xiiUuid                  request;
  reader >> request;
  if (request != m_CurFileRequestGuid)
    return;

  xiiUInt16           uiChunkSize  = 0;
  xiiUInt32           uiFileSize   = 0;
  constexpr xiiUInt32 uiHeaderSize = 16 + sizeof(uiChunkSize) + sizeof(uiFileSize);
  if (reader.ReadWordValue(&uiChunkSize).Failed() || reader.ReadDWordValue(&uiFileSize).Failed() ||
      data.GetCount() != uiHeaderSize + uiChunkSize ||
      (m_bDownloadStaged && uiFileSize != m_uiDownloadSize) || m_uiDownloadReceived > uiFileSize ||
      uiChunkSize > uiFileSize - m_uiDownloadReceived || (uiChunkSize == 0 && (uiFileSize != 0 || m_bDownloadStaged)))
  {
    m_bDownloading = false;
    return;
  }

  if (!m_bDownloadStaged)
  {
    xiiStringBuilder sRequestId;
    xiiConversionUtils::ToString(request, sRequestId);
    xiiStringBuilder sStagingPath(m_sFileserveCacheFolder, "/.transfers/", sRequestId);
    if (m_DownloadFile.Open(sStagingPath, xiiFileOpenMode::Write).Failed())
    {
      m_bDownloading = false;
      return;
    }
    m_sDownloadStagingPath = sStagingPath;
    m_uiDownloadSize       = uiFileSize;
    m_bDownloadStaged      = true;
  }

  if (m_DownloadFile.Write(data.GetPtr() + uiHeaderSize, uiChunkSize).Failed())
  {
    m_bDownloading = false;
    return;
  }

  m_uiDownloadReceived += uiChunkSize;
  if (m_uiDownloadReceived < uiFileSize && m_uiDownloadReceived % xiiFileserveTransfer::TransferWindowBytes == 0)
  {
    Request progress(Operation::DownloadProgress);
    progress.GetWriter() << request;
    progress.GetWriter() << m_uiDownloadReceived;
    if (progress.Send(*m_pNetwork, m_Channel).Failed())
    {
      m_pNetwork->Disconnect(m_Channel.m_Connection, false).IgnoreResult();
      m_bDownloading = false;
    }
  }
}

void xiiFileserveClient::HandleFileTransferFinishedMsg(xiiArrayPtr<const xiiUInt8> data)
{
  if (!m_bDownloading || data.GetCount() < sizeof(xiiUuid))
    return;

  xiiRawMemoryStreamReader reader(data.GetPtr(), data.GetCount());
  xiiUuid                  request;
  reader >> request;
  if (request != m_CurFileRequestGuid)
    return;

  XII_SCOPE_EXIT(m_bDownloading = false);
  constexpr xiiUInt32 uiResponseSize = 16 + sizeof(xiiInt8) + sizeof(xiiInt64) + sizeof(xiiUInt64) + sizeof(xiiUInt16);
  if (data.GetCount() != uiResponseSize)
    return;

  xiiInt8   iFileState = 0;
  xiiInt64  iTimestamp = 0;
  xiiUInt64 uiHash     = 0;
  xiiUInt16 uiDataDir  = 0;
  reader >> iFileState;
  reader >> iTimestamp;
  reader >> uiHash;
  reader >> uiDataDir;

  const auto state = static_cast<xiiFileserveFileState>(iFileState);
  if (state < xiiFileserveFileState::NonExistant || state > xiiFileserveFileState::Different)
    return;

  if (uiDataDir == 0xffff)
  {
    if (!m_bDownloadStaged && !m_bForceDownloadDataDir &&
        (state == xiiFileserveFileState::NonExistant || state == xiiFileserveFileState::NonExistantEither))
    {
      for (auto& dir : m_MountedDataDirs)
      {
        if (dir.m_bMounted)
          dir.m_CacheStatus[m_sCurFileRequest] = {0, 0, xiiTime::Now()};
      }
      m_FileDataDir[m_sCurFileRequest] = m_uiDownloadDataDir;
      m_bDownloadSucceeded             = true;
    }
    return;
  }

  if (uiDataDir >= m_MountedDataDirs.GetCount() || !m_MountedDataDirs[uiDataDir].m_bMounted ||
      (m_bForceDownloadDataDir && uiDataDir != m_uiDownloadDataDir) || (m_bDownloadStaged != (state == xiiFileserveFileState::Different)))
    return;

  auto&            dir    = m_MountedDataDirs[uiDataDir];
  auto&            status = dir.m_CacheStatus[m_sCurFileRequest];
  xiiStringBuilder sCachedFile, sCachedMetaFile;
  BuildPathInCache(m_sCurFileRequest, dir.m_sMountPoint, &sCachedFile, &sCachedMetaFile);

  switch (state)
  {
    case xiiFileserveFileState::Different:
      if (uiHash == 0 || CommitDownload(sCachedFile, uiHash).Failed())
        return;
      if (WriteMetaFile(sCachedMetaFile, iTimestamp, uiHash).Failed())
      {
        xiiOSFile::DeleteFile(sCachedMetaFile).IgnoreResult();
        return;
      }
      break;

    case xiiFileserveFileState::SameTimestamp:
    case xiiFileserveFileState::SameHash:
      if (uiHash == 0 || status.m_FileHash != uiHash || !xiiOSFile::ExistsFile(sCachedFile))
        return;
      if (state == xiiFileserveFileState::SameHash && WriteMetaFile(sCachedMetaFile, iTimestamp, uiHash).Failed())
        return;
      break;

    case xiiFileserveFileState::NonExistant:
    case xiiFileserveFileState::NonExistantEither:
      if (uiHash != 0 || !xiiOSFile::DeleteFile(sCachedFile).Succeeded() || !xiiOSFile::DeleteFile(sCachedMetaFile).Succeeded())
        return;
      break;

    default:
      return;
  }

  status = {iTimestamp, uiHash, xiiTime::Now()};
  if (!m_bForceDownloadDataDir)
    m_FileDataDir[m_sCurFileRequest] = uiDataDir;
  m_bDownloadSucceeded = true;
}

xiiResult xiiFileserveClient::WriteMetaFile(xiiStringView sCachedMetaFile, xiiInt64 iFileTimeStamp, xiiUInt64 uiFileHash)
{
  xiiStringBuilder sId;
  xiiConversionUtils::ToString(xiiUuid::MakeUuid(), sId);
  const xiiStringBuilder sStagingPath(sCachedMetaFile, ".", sId, ".tmp");
  xiiOSFile              file;
  XII_SUCCEED_OR_RETURN(file.Open(sStagingPath, xiiFileOpenMode::Write));
  XII_SCOPE_EXIT(file.Close(); xiiOSFile::DeleteFile(sStagingPath).IgnoreResult());

  XII_SUCCEED_OR_RETURN(file.Write(&iFileTimeStamp, sizeof(iFileTimeStamp)));
  XII_SUCCEED_OR_RETURN(file.Write(&uiFileHash, sizeof(uiFileHash)));
  file.Close();
  xiiFileStats stat;
  if (xiiOSFile::GetFileStats(sStagingPath, stat).Failed() || stat.m_uiFileSize != sizeof(iFileTimeStamp) + sizeof(uiFileHash))
    return XII_FAILURE;

  return xiiOSFile::AtomicReplaceFile(sStagingPath, sCachedMetaFile);
}

xiiResult xiiFileserveClient::CommitDownload(xiiStringView sCachedFile, xiiUInt64 uiExpectedHash)
{
  if (!m_bDownloadStaged || m_uiDownloadReceived != m_uiDownloadSize)
    return XII_FAILURE;

  m_DownloadFile.Close();
  xiiOSFile verifiedFile;
  XII_SUCCEED_OR_RETURN(verifiedFile.Open(m_sDownloadStagingPath, xiiFileOpenMode::Read));
  xiiUInt64 uiHash = 0;
  if (xiiFileserveClientContext::HashFile(verifiedFile, m_uiDownloadSize, uiHash).Failed() || uiHash != uiExpectedHash)
    return XII_FAILURE;

  verifiedFile.Close();
  const xiiStringBuilder sDestination(sCachedFile);
  XII_SUCCEED_OR_RETURN(xiiOSFile::CreateDirectoryStructure(sDestination.GetFileDirectory()));
  return xiiOSFile::AtomicReplaceFile(m_sDownloadStagingPath, sCachedFile);
}

void xiiFileserveClient::DiscardDownload()
{
  m_DownloadFile.Close();
  if (!m_sDownloadStagingPath.IsEmpty())
  {
    xiiOSFile::DeleteFile(m_sDownloadStagingPath).IgnoreResult();
    m_sDownloadStagingPath.Clear();
  }
  m_bDownloadStaged    = false;
  m_uiDownloadSize     = 0;
  m_uiDownloadReceived = 0;
}

xiiResult xiiFileserveClient::DownloadFile(xiiUInt16 uiDataDirID, const char* szFile, bool bForceThisDataDir, xiiStringBuilder* out_pFullPath)
{
  XII_LOCK(m_Mutex);
  if (m_bDownloading)
  {
    xiiLog::Warning("Trying to download a file over fileserve while another file is already downloading. Recursive download is ignored.");
    return XII_FAILURE;
  }

  if (!m_pNetwork || !m_Channel.IsValid() || uiDataDirID >= m_MountedDataDirs.GetCount() ||
      !m_MountedDataDirs[uiDataDirID].m_bMounted || !xiiFileserveClientContext::IsValidRelativePath(szFile))
    return XII_FAILURE;

  bool bCachedYet    = false;
  auto itFileDataDir = m_FileDataDir.FindOrAdd(szFile, &bCachedYet);
  if (!bCachedYet)
  {
    FillFileStatusCache(szFile);
  }

  const auto             uiCachedDir       = itFileDataDir.Value();
  const auto             uiUseDataDirCache = bForceThisDataDir || !m_MountedDataDirs[uiCachedDir].m_bMounted ? uiDataDirID : uiCachedDir;
  const FileCacheStatus& CacheStatus       = m_MountedDataDirs[uiUseDataDirCache].m_CacheStatus[szFile];

  if (xiiTime::Now() - CacheStatus.m_LastCheck < xiiTime::MakeFromSeconds(5.0f))
  {
    if (CacheStatus.m_FileHash == 0 || uiUseDataDirCache != uiDataDirID)
      return XII_FAILURE;

    if (out_pFullPath)
      BuildPathInCache(szFile, m_MountedDataDirs[uiUseDataDirCache].m_sMountPoint, out_pFullPath, nullptr);

    return XII_SUCCESS;
  }

  m_sCurFileRequest       = szFile;
  m_CurFileRequestGuid    = xiiUuid::MakeUuid();
  m_uiDownloadDataDir     = uiUseDataDirCache;
  m_bForceDownloadDataDir = bForceThisDataDir;
  m_bDownloadSucceeded    = false;
  m_bDownloading          = true;
  XII_SCOPE_EXIT(m_bDownloading = false; DiscardDownload());

  Request msg(Operation::Read);
  msg.GetWriter() << uiUseDataDirCache;
  msg.GetWriter() << bForceThisDataDir;
  msg.GetWriter() << szFile;
  msg.GetWriter() << m_CurFileRequestGuid;
  msg.GetWriter() << CacheStatus.m_TimeStamp;
  msg.GetWriter() << CacheStatus.m_FileHash;

  const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(120);
  XII_SUCCEED_OR_RETURN(SendRequest(msg, deadline));
  while (m_bDownloading && m_Channel.IsValid() && xiiTime::Now() < deadline)
  {
    ProcessNetworkEvents(deadline - xiiTime::Now());
  }

  if (!m_bDownloadSucceeded)
    return XII_FAILURE;

  if (bForceThisDataDir)
  {
    if (m_MountedDataDirs[uiDataDirID].m_CacheStatus[m_sCurFileRequest].m_FileHash == 0)
      return XII_FAILURE;

    if (out_pFullPath)
      BuildPathInCache(szFile, m_MountedDataDirs[uiDataDirID].m_sMountPoint, out_pFullPath, nullptr);

    return XII_SUCCESS;
  }
  else
  {
    const xiiUInt16 uiBestDir = itFileDataDir.Value();
    if (uiBestDir == uiDataDirID) // best match is still this? -> success
    {
      // file does not exist
      if (m_MountedDataDirs[uiBestDir].m_CacheStatus[m_sCurFileRequest].m_FileHash == 0)
        return XII_FAILURE;

      if (out_pFullPath)
        BuildPathInCache(szFile, m_MountedDataDirs[uiBestDir].m_sMountPoint, out_pFullPath, nullptr);

      return XII_SUCCESS;
    }

    return XII_FAILURE;
  }
}

void xiiFileserveClient::DetermineCacheStatus(xiiUInt16 uiDataDirID, const char* szFile, FileCacheStatus& out_Status) const
{
  XII_LOCK(m_Mutex);
  xiiStringBuilder sAbsPathFile, sAbsPathMeta;
  const auto&      dd = m_MountedDataDirs[uiDataDirID];

  XII_ASSERT_DEV(dd.m_bMounted, "Data directory {0} is not mounted", uiDataDirID);

  BuildPathInCache(szFile, dd.m_sMountPoint, &sAbsPathFile, &sAbsPathMeta);

  if (xiiOSFile::ExistsFile(sAbsPathFile))
  {
    xiiOSFile meta;
    if (meta.Open(sAbsPathMeta, xiiFileOpenMode::Read).Failed())
    {
      // cleanup, when the meta file does not exist, the data file is useless
      xiiOSFile::DeleteFile(sAbsPathFile).IgnoreResult();
      return;
    }

    xiiOSFile file;
    xiiUInt64 uiActualHash = 0;
    if (meta.GetFileSize() != sizeof(xiiInt64) + sizeof(xiiUInt64) ||
        meta.Read(&out_Status.m_TimeStamp, sizeof(xiiInt64)) != sizeof(xiiInt64) ||
        meta.Read(&out_Status.m_FileHash, sizeof(xiiUInt64)) != sizeof(xiiUInt64) ||
        file.Open(sAbsPathFile, xiiFileOpenMode::Read).Failed() ||
        xiiFileserveClientContext::HashFile(file, file.GetFileSize(), uiActualHash).Failed() ||
        out_Status.m_FileHash != uiActualHash)
    {
      out_Status = {};
    }
  }
}

xiiResult xiiFileserveClient::TryReadFileserveConfig(const char* szFile, xiiStringBuilder& out_Result)
{
  xiiOSFile file;
  if (file.Open(szFile, xiiFileOpenMode::Read).Succeeded())
  {
    xiiUInt8 data[64]; // an IP + port should not be longer than 22 characters

    xiiStringBuilder res;

    data[file.Read(data, 63)] = 0;
    res                       = (const char*)data;
    res.Trim(" \t\n\r");

    if (res.IsEmpty())
      return XII_FAILURE;

    // has to contain a port number
    if (res.FindSubString(":") == nullptr)
      return XII_FAILURE;

    // otherwise could be an arbitrary string
    out_Result = res;
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiFileserveClient::SearchForServerAddress(xiiTime timeout /*= xiiTime::MakeFromSeconds(5)*/)
{
  XII_LOCK(m_Mutex);
  if (!s_bEnableFileserve)
    return XII_FAILURE;

  xiiStringBuilder sAddress;

  // add the command line argument again, in case this was modified since the constructor ran
  // will not change anything, if this is a duplicate
  AddServerAddressToTry(xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-fs_server", 0, ""));

  // go through the available options
  for (xiiInt32 idx = m_TryServerAddresses.GetCount() - 1; idx >= 0; --idx)
  {
    if (TryConnectWithFileserver(m_TryServerAddresses[idx], timeout).Succeeded())
      return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiFileserveClient::TryConnectWithFileserver(const char* szAddress, xiiTime timeout) const
{
  XII_LOCK(m_Mutex);
  if (xiiStringUtils::IsNullOrEmpty(szAddress))
    return XII_FAILURE;

  xiiLog::Info("File server address: '{0}' ({1} sec)", szAddress, timeout.GetSeconds());

  xiiNetworkHost host;
  XII_SUCCEED_OR_RETURN(xiiFileserveTransfer::Start(host, false));
  xiiNetworkConnection connection;
  XII_SUCCEED_OR_RETURN(xiiFileserveTransfer::Connect(host, szAddress, connection).GetResult());

  const auto      deadline = xiiTime::Now() + timeout;
  xiiNetworkEvent event;
  bool            bConnected = false;
  while (xiiTime::Now() < deadline && host.WaitForEvent(event, deadline - xiiTime::Now()).Succeeded())
  {
    if (event.m_Connection != connection)
      continue;
    if (event.m_Type == xiiNetworkEventType::Closed)
      break;
    if (event.m_Type == xiiNetworkEventType::Connected)
    {
      bConnected = true;
      break;
    }
  }
  if (!bConnected)
    return XII_FAILURE;

  m_sServerConnectionAddress = szAddress;

  // always store the IP that was successful in the user directory
  if (!xiiCommandLineUtils::GetGlobalInstance()->GetBoolOption("-fs_no_save"))
    SaveCurrentConnectionInfoToDisk().IgnoreResult();
  return XII_SUCCESS;
}

xiiResult xiiFileserveClient::WaitForServerInfo(xiiTime timeout)
{
  if (!s_bEnableFileserve || !timeout.IsPositive())
    return XII_FAILURE;

  const auto                 deadline = xiiTime::Now() + timeout;
  xiiUInt16                  uiPort   = 0;
  xiiDynamicArray<xiiString> addresses;
  XII_SUCCEED_OR_RETURN(xiiFileserveDiscovery::Receive(uiPort, addresses, timeout));

  for (const auto& ip : addresses)
  {
    const auto remaining = deadline - xiiTime::Now();
    if (!remaining.IsPositive())
      break;

    xiiStringBuilder sAddress;
    if (ip.FindSubString(":") != nullptr)
      sAddress.SetFormat("[{}]:{}", ip, uiPort);
    else
      sAddress.SetFormat("{}:{}", ip, uiPort);
    if (TryConnectWithFileserver(sAddress, xiiMath::Min(remaining, xiiTime::MakeFromSeconds(3))).Succeeded())
      return XII_SUCCESS;
  }
  return XII_FAILURE;
}
xiiResult xiiFileserveClient::SaveCurrentConnectionInfoToDisk() const
{
  XII_LOCK(m_Mutex);
  xiiStringBuilder sFile = xiiOSFile::GetUserDataFolder("xiiFileserve.txt");
  xiiOSFile        file;
  XII_SUCCEED_OR_RETURN(file.Open(sFile, xiiFileOpenMode::Write));

  XII_SUCCEED_OR_RETURN(file.Write(m_sServerConnectionAddress.GetData(), m_sServerConnectionAddress.GetElementCount()));
  file.Close();

  return XII_SUCCESS;
}

XII_ON_GLOBAL_EVENT(GameApp_UpdatePlugins)
{
  if (xiiFileserveClient::GetSingleton())
  {
    xiiFileserveClient::GetSingleton()->UpdateClient();
  }
}



XII_STATICLINK_FILE(FileservePlugin, FileservePlugin_Client_FileserveClient);
