/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FileservePlugin/FileservePluginPCH.h>

#include <FileservePlugin/Client/FileserveClient.h>
#include <FileservePlugin/Discovery/FileserveDiscovery.h>
#include <FileservePlugin/Fileserver/ClientContext.h>
#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/RemoteInterfaceEnet.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/UnicodeUtils.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <cstring>

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

    m_pNetwork->ShutdownConnection();
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
    m_pNetwork = xiiRemoteInterfaceEnet::Make();
  }

  if (!m_pNetwork->IsConnectedToServer())
  {
    ClearState();
    m_bFailedToConnect = true;

    if (m_pNetwork->ConnectToServer(XII_FILESERVE_TRANSFER_PROTOCOL, m_sServerConnectionAddress).Failed())
      return XII_FAILURE;

    if (timeout.GetSeconds() < 0)
    {
      timeout = xiiTime::MakeFromSeconds(xiiCommandLineUtils::GetGlobalInstance()->GetFloatOption("-fs_timeout", -timeout.GetSeconds()));
    }
    if (!timeout.IsPositive() || timeout > xiiTime::MakeFromSeconds(120))
      timeout = xiiTime::MakeFromSeconds(120);

    if (m_pNetwork->WaitForConnectionToServer(timeout).Failed())
    {
      m_pNetwork->ShutdownConnection();
      xiiLog::Error("Connection to xiiFileserver timed out");
      return XII_FAILURE;
    }
    else
    {
      xiiLog::Success("Connected to xiiFileserver '{0}", m_sServerConnectionAddress);
      m_pNetwork->SetMessageHandler('FSRV', xiiMakeDelegate(&xiiFileserveClient::NetworkMsgHandler, this));

      m_pNetwork->Send('FSRV', 'HELO'); // be friendly
    }

    m_bFailedToConnect = false;
  }

  return XII_SUCCESS;
}

void xiiFileserveClient::UpdateClient()
{
  bool bReloadResources = false;
  {
    XII_LOCK(m_Mutex);
    if (m_pNetwork == nullptr || m_bFailedToConnect || !s_bEnableFileserve)
      return;

    if (!m_pNetwork->IsConnectedToServer())
    {
      if (EnsureConnected().Failed())
      {
        xiiLog::Error("Fileserve connection was lost and could not be re-established.");
        ShutdownConnection();
      }
      return;
    }

    m_pNetwork->ExecuteAllMessageHandlers();

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
  return m_pNetwork && m_pNetwork->IsConnectedToServer();
}

xiiResult xiiFileserveClient::CompileShader(const xiiRemoteShaderCompileRequest& request, xiiStringBuilder& out_sLog)
{
  XII_LOCK(m_Mutex);
  out_sLog = "Invalid or unavailable FileServe shader compilation request.";
  if (!m_pNetwork || !m_pNetwork->IsConnectedToServer() || m_bShaderCompiling || m_bDownloading ||
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
  m_pNetwork->SetMessageHandler('SHDR', [&](xiiRemoteMessage& message) {
    constexpr xiiUInt32 uiHeaderSize = 16 + sizeof(xiiUInt8) + sizeof(xiiUInt32);
    const auto          bytes        = message.GetMessageData();
    if (bComplete || message.GetMessageID() != 'CRES' || bytes.GetCount() < uiHeaderSize)
      return;

    auto&   reader = message.GetReader();
    xiiUuid responseId;
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
  });
  XII_SCOPE_EXIT(m_pNetwork->SetMessageHandler('SHDR', {}); m_bShaderCompiling = false);

  xiiRemoteMessage msg('SHDR', 'CMPL');
  msg.GetWriter() << id;
  msg.GetWriter() << request.m_sShader;
  msg.GetWriter() << request.m_sPlatform;
  msg.GetWriter() << request.m_Permutations.GetCount();
  for (const auto& permutation : request.m_Permutations)
  {
    msg.GetWriter() << permutation.m_sName;
    msg.GetWriter() << permutation.m_sValue;
  }
  m_pNetwork->Send(xiiRemoteTransmitMode::Reliable, msg);

  const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(120);
  while (!bComplete && m_pNetwork->IsConnectedToServer() && xiiTime::Now() < deadline)
  {
    m_pNetwork->UpdateRemoteInterface();
    if (m_pNetwork->ExecuteAllMessageHandlers() == 0)
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
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
  if (!m_pNetwork || !m_pNetwork->IsConnectedToServer() || m_bDownloading || m_bWaitingForUploadFinished ||
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
  m_bUploadSucceeded          = false;
  m_bWaitingForUploadFinished = true;
  XII_SCOPE_EXIT(m_bWaitingForUploadFinished = false; m_UploadRequest = {});

  {
    xiiRemoteMessage msg('FSRV', 'UPLH');
    msg.GetWriter() << m_UploadRequest;
    msg.GetWriter() << static_cast<xiiUInt32>(uiFileSize);
    msg.GetWriter() << uiDataDirID;
    msg.GetWriter() << sFile;
    m_pNetwork->Send(xiiRemoteTransmitMode::Reliable, msg);
  }

  xiiHashStreamWriter64 hash(1);
  xiiUInt8              bytes[16 * 1024];
  xiiUInt64             uiRemaining = uiFileSize;
  const auto            deadline    = xiiTime::Now() + xiiTime::MakeFromSeconds(120);
  while (uiRemaining != 0 && m_bWaitingForUploadFinished && m_pNetwork->IsConnectedToServer() && xiiTime::Now() < deadline)
  {
    const auto uiChunkSize = static_cast<xiiUInt16>(xiiMath::Min<xiiUInt64>(sizeof(bytes), uiRemaining));
    if (ref_file.Read(bytes, uiChunkSize) != uiChunkSize || hash.WriteBytes(bytes, uiChunkSize).Failed())
      return XII_FAILURE;

    xiiRemoteMessage msg('FSRV', 'UPLD');
    msg.GetWriter() << m_UploadRequest;
    msg.GetWriter() << uiChunkSize;
    XII_SUCCEED_OR_RETURN(msg.GetWriter().WriteBytes(bytes, uiChunkSize));
    m_pNetwork->Send(xiiRemoteTransmitMode::Reliable, msg);
    m_pNetwork->UpdateRemoteInterface();
    m_pNetwork->ExecuteAllMessageHandlers();

    uiRemaining -= uiChunkSize;
  }

  if (uiRemaining != 0 || !m_bWaitingForUploadFinished || !m_pNetwork->IsConnectedToServer())
    return XII_FAILURE;

  const xiiUInt64 uiHash = uiFileSize == 0 ? 1 : hash.GetHashValue();
  {
    xiiRemoteMessage msg('FSRV', 'UPLF');
    msg.GetWriter() << m_UploadRequest;
    msg.GetWriter() << uiDataDirID;
    msg.GetWriter() << sFile;
    msg.GetWriter() << uiHash;

    m_pNetwork->Send(xiiRemoteTransmitMode::Reliable, msg);
  }

  while (m_bWaitingForUploadFinished && m_pNetwork->IsConnectedToServer() && xiiTime::Now() < deadline)
  {
    m_pNetwork->UpdateRemoteInterface();
    if (m_pNetwork->ExecuteAllMessageHandlers() == 0)
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
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

void xiiFileserveClient::NetworkMsgHandler(xiiRemoteMessage& msg)
{
  XII_LOCK(m_Mutex);
  if (msg.GetMessageID() == 'DWNL')
  {
    HandleFileTransferMsg(msg);
    return;
  }

  if (msg.GetMessageID() == 'DWNF')
  {
    HandleFileTransferFinishedMsg(msg);
    return;
  }

  if (msg.GetMessageID() == 'RLDR')
  {
    InvalidateAllCache();
    m_bReloadResources = true;
    return;
  }

  if (msg.GetMessageID() == 'UACK')
  {
    if (m_bWaitingForUploadFinished && msg.GetMessageData().GetCount() == 16 + sizeof(xiiUInt8))
    {
      xiiUuid  request;
      xiiUInt8 uiSuccess = 0;
      msg.GetReader() >> request;
      if (request == m_UploadRequest && msg.GetReader().ReadBytes(&uiSuccess, sizeof(uiSuccess)) == sizeof(uiSuccess))
      {
        m_bWaitingForUploadFinished = false;
        m_bUploadSucceeded          = uiSuccess == 1;
      }
    }
    return;
  }

  if (msg.GetMessageID() == 'INVC')
  {
    InvalidateAllCache();
    return;
  }

  xiiLog::Error("Unknown FSRV message: '{0}' - {1} bytes", msg.GetMessageID(), msg.GetMessageData().GetCount());
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
  if (!m_pNetwork || !m_pNetwork->IsConnectedToServer() || m_MountedDataDirs.GetCount() >= 4096)
    return 0xffff;

  xiiStringBuilder sRoot = sRootName;
  sRoot.Trim(":/");

  xiiStringBuilder sMountPoint;
  ComputeDataDirMountPoint(sDataDirectory, sMountPoint);

  const xiiUInt16 uiDataDirID = static_cast<xiiUInt16>(m_MountedDataDirs.GetCount());

  xiiRemoteMessage msg('FSRV', ' MNT');
  msg.GetWriter() << sDataDirectory;
  msg.GetWriter() << sRoot;
  msg.GetWriter() << sMountPoint;
  msg.GetWriter() << uiDataDirID;

  m_pNetwork->Send(xiiRemoteTransmitMode::Reliable, msg);

  auto& dd = m_MountedDataDirs.ExpandAndGetRef();
  // dd.m_sPathOnClient = sDataDirectory;
  // dd.m_sRootName = sRoot;
  dd.m_sMountPoint = sMountPoint;
  dd.m_bMounted    = true;

  return uiDataDirID;
}


void xiiFileserveClient::UnmountDataDirectory(xiiUInt16 uiDataDir)
{
  XII_LOCK(m_Mutex);
  if (uiDataDir >= m_MountedDataDirs.GetCount() || !m_MountedDataDirs[uiDataDir].m_bMounted)
    return;

  if (m_pNetwork && m_pNetwork->IsConnectedToServer())
  {
    xiiRemoteMessage msg('FSRV', 'UMNT');
    msg.GetWriter() << uiDataDir;
    m_pNetwork->Send(xiiRemoteTransmitMode::Reliable, msg);
  }

  auto& dd      = m_MountedDataDirs[uiDataDir];
  dd.m_bMounted = false;
}

void xiiFileserveClient::DeleteFile(xiiUInt16 uiDataDir, xiiStringView sFile)
{
  XII_LOCK(m_Mutex);
  if (!m_pNetwork || !m_pNetwork->IsConnectedToServer() || uiDataDir >= m_MountedDataDirs.GetCount() ||
      !m_MountedDataDirs[uiDataDir].m_bMounted || !xiiFileserveClientContext::IsValidRelativePath(sFile))
    return;

  InvalidateFileCache(uiDataDir, sFile, 0);

  xiiRemoteMessage msg('FSRV', 'DELF');
  msg.GetWriter() << uiDataDir;
  msg.GetWriter() << sFile;

  m_pNetwork->Send(xiiRemoteTransmitMode::Reliable, msg);
}

void xiiFileserveClient::HandleFileTransferMsg(xiiRemoteMessage& msg)
{
  if (!m_bDownloading || msg.GetMessageData().GetCount() < sizeof(xiiUuid))
    return;

  auto&   reader = msg.GetReader();
  xiiUuid request;
  reader >> request;
  if (request != m_CurFileRequestGuid)
    return;

  xiiUInt16           uiChunkSize  = 0;
  xiiUInt32           uiFileSize   = 0;
  constexpr xiiUInt32 uiHeaderSize = 16 + sizeof(uiChunkSize) + sizeof(uiFileSize);
  if (reader.ReadWordValue(&uiChunkSize).Failed() || reader.ReadDWordValue(&uiFileSize).Failed() ||
      msg.GetMessageData().GetCount() != uiHeaderSize + uiChunkSize ||
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

  if (m_DownloadFile.Write(msg.GetMessageData().GetPtr() + uiHeaderSize, uiChunkSize).Failed())
  {
    m_bDownloading = false;
    return;
  }

  m_uiDownloadReceived += uiChunkSize;
}

void xiiFileserveClient::HandleFileTransferFinishedMsg(xiiRemoteMessage& msg)
{
  if (!m_bDownloading || msg.GetMessageData().GetCount() < sizeof(xiiUuid))
    return;

  auto&   reader = msg.GetReader();
  xiiUuid request;
  reader >> request;
  if (request != m_CurFileRequestGuid)
    return;

  XII_SCOPE_EXIT(m_bDownloading = false);
  constexpr xiiUInt32 uiResponseSize = 16 + sizeof(xiiInt8) + sizeof(xiiInt64) + sizeof(xiiUInt64) + sizeof(xiiUInt16);
  if (msg.GetMessageData().GetCount() != uiResponseSize)
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

  if (!m_pNetwork || !m_pNetwork->IsConnectedToServer() || uiDataDirID >= m_MountedDataDirs.GetCount() ||
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

  xiiRemoteMessage msg('FSRV', 'READ');
  msg.GetWriter() << uiUseDataDirCache;
  msg.GetWriter() << bForceThisDataDir;
  msg.GetWriter() << szFile;
  msg.GetWriter() << m_CurFileRequestGuid;
  msg.GetWriter() << CacheStatus.m_TimeStamp;
  msg.GetWriter() << CacheStatus.m_FileHash;

  m_pNetwork->Send(xiiRemoteTransmitMode::Reliable, msg);

  const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(120);
  while (m_bDownloading && m_pNetwork->IsConnectedToServer() && xiiTime::Now() < deadline)
  {
    m_pNetwork->UpdateRemoteInterface();
    if (m_pNetwork->ExecuteAllMessageHandlers() == 0)
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
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

  xiiUniquePtr<xiiRemoteInterfaceEnet> network = xiiRemoteInterfaceEnet::Make(); /// \todo Abstract this somehow ?
  if (network->ConnectToServer(XII_FILESERVE_TRANSFER_PROTOCOL, szAddress, false).Failed())
    return XII_FAILURE;

  bool bServerFound = false;
  network->SetMessageHandler('FSRV', [&bServerFound](xiiRemoteMessage& ref_msg) {
    switch (ref_msg.GetMessageID())
    {
      case ' YES':
        bServerFound = true;
        break;
    } });

  if (network->WaitForConnectionToServer(timeout).Succeeded())
  {
    // wait for a proper response
    xiiTime tStart = xiiTime::Now();
    while (xiiTime::Now() - tStart < timeout && !bServerFound)
    {
      network->Send('FSRV', 'RUTR');

      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(100));

      network->UpdateRemoteInterface();
      network->ExecuteAllMessageHandlers();
    }
  }

  network->ShutdownConnection();

  if (!bServerFound)
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
