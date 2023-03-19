#include <FileservePlugin/FileservePluginPCH.h>

#include <FileservePlugin/Client/FileserveClient.h>
#include <FileservePlugin/Fileserver/ClientContext.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/RemoteInterfaceEnet.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineUtils.h>

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

  m_CurrentTime = xiiTime::Now();
}

xiiFileserveClient::~xiiFileserveClient()
{
  ShutdownConnection();
}

void xiiFileserveClient::ShutdownConnection()
{
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
  m_CurFileRequestGuid        = xiiUuid();
  m_sCurFileRequest.Clear();
  m_Download.Clear();
}

xiiResult xiiFileserveClient::EnsureConnected(xiiTime timeout)
{
  XII_LOCK(m_Mutex);
  if (!s_bEnableFileserve || m_bFailedToConnect)
    return XII_FAILURE;

  if (m_pNetwork == nullptr)
  {
    m_pNetwork = xiiRemoteInterfaceEnet::Make(); /// \todo Somehow abstract this away ?

    m_sFileserveCacheFolder     = xiiOSFile::GetUserDataFolder("xiiFileserve/Cache");
    m_sFileserveCacheMetaFolder = xiiOSFile::GetUserDataFolder("xiiFileserve/Meta");

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

  if (!m_pNetwork->IsConnectedToServer())
  {
    ClearState();
    m_bFailedToConnect = true;

    if (m_pNetwork->ConnectToServer('XIIFS', m_sServerConnectionAddress).Failed())
      return XII_FAILURE;

    if (timeout.GetSeconds() < 0)
    {
      timeout = xiiTime::Seconds(xiiCommandLineUtils::GetGlobalInstance()->GetFloatOption("-fs_timeout", -timeout.GetSeconds()));
    }

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

  m_CurrentTime = xiiTime::Now();

  m_pNetwork->ExecuteAllMessageHandlers();
}

void xiiFileserveClient::AddServerAddressToTry(const char* szAddress)
{
  XII_LOCK(m_Mutex);
  if (xiiStringUtils::IsNullOrEmpty(szAddress))
    return;

  if (m_TryServerAddresses.Contains(szAddress))
    return;

  m_TryServerAddresses.PushBack(szAddress);

  // always set the most recent address as the default one
  m_sServerConnectionAddress = szAddress;
}

void xiiFileserveClient::UploadFile(xiiUInt16 uiDataDirID, const char* szFile, const xiiDynamicArray<xiiUInt8>& fileContent)
{
  XII_LOCK(m_Mutex);

  if (m_pNetwork == nullptr)
    return;

  // update meta state and cache
  {
    const xiiString& sMountPoint = m_MountedDataDirs[uiDataDirID].m_sMountPoint;
    xiiStringBuilder sCachedMetaFile;
    BuildPathInCache(szFile, sMountPoint, nullptr, &sCachedMetaFile);

    xiiUInt64 uiHash = 1;

    if (!fileContent.IsEmpty())
    {
      uiHash = xiiHashingUtils::xxHash64(fileContent.GetData(), fileContent.GetCount(), uiHash);
    }

    WriteMetaFile(sCachedMetaFile, 0, uiHash);

    InvalidateFileCache(uiDataDirID, szFile, uiHash);
  }

  const xiiUInt32 uiFileSize = fileContent.GetCount();

  xiiUuid uploadGuid;
  uploadGuid.CreateNewUuid();

  {
    xiiRemoteMessage msg;
    msg.SetMessageID('FSRV', 'UPLH');
    msg.GetWriter() << uploadGuid;
    msg.GetWriter() << uiFileSize;
    msg.GetWriter() << uiDataDirID;
    msg.GetWriter() << szFile;
    m_pNetwork->Send(xiiRemoteTransmitMode::Reliable, msg);
  }

  xiiUInt32 uiNextByte = 0;

  // send the file over in multiple packages of 1KB each
  // send at least one package, even for empty files

  while (uiNextByte < fileContent.GetCount())
  {
    const xiiUInt16 uiChunkSize = (xiiUInt16)xiiMath::Min<xiiUInt32>(1024, fileContent.GetCount() - uiNextByte);

    xiiRemoteMessage msg;
    msg.GetWriter() << uploadGuid;
    msg.GetWriter() << uiChunkSize;
    msg.GetWriter().WriteBytes(&fileContent[uiNextByte], uiChunkSize).IgnoreResult();

    msg.SetMessageID('FSRV', 'UPLD');
    m_pNetwork->Send(xiiRemoteTransmitMode::Reliable, msg);

    uiNextByte += uiChunkSize;
  }

  // continuously update the network until we know the server has received the big chunk of data
  m_bWaitingForUploadFinished = true;

  // final message to server
  {
    xiiRemoteMessage msg('FSRV', 'UPLF');
    msg.GetWriter() << uploadGuid;
    msg.GetWriter() << uiDataDirID;
    msg.GetWriter() << szFile;

    m_pNetwork->Send(xiiRemoteTransmitMode::Reliable, msg);
  }

  while (m_bWaitingForUploadFinished)
  {
    UpdateClient();
  }
}


void xiiFileserveClient::InvalidateFileCache(xiiUInt16 uiDataDirID, xiiStringView sFile, xiiUInt64 uiHash)
{
  XII_LOCK(m_Mutex);
  auto& cache       = m_MountedDataDirs[uiDataDirID].m_CacheStatus[sFile];
  cache.m_FileHash  = uiHash;
  cache.m_TimeStamp = 0;
  cache.m_LastCheck.SetZero(); // will trigger a server request and that in turn will update the file timestamp

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
    cache.m_LastCheck.SetZero();

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
  out_sMountPoint.Format("{0}", xiiArgU(uiMountPoint, 8, true, 16));
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

  static bool s_bReloadResources = false;

  if (msg.GetMessageID() == 'RLDR')
  {
    s_bReloadResources = true;
  }

  if (!m_bDownloading && s_bReloadResources)
  {
    XII_BROADCAST_EVENT(xiiResourceManager_ReloadAllResources);
    s_bReloadResources = false;
    return;
  }

  if (msg.GetMessageID() == 'RLDR')
    return;

  if (msg.GetMessageID() == 'UACK')
  {
    m_bWaitingForUploadFinished = false;
    return;
  }

  xiiLog::Error("Unknown FSRV message: '{0}' - {1} bytes", msg.GetMessageID(), msg.GetMessageData().GetCount());
}

xiiUInt16 xiiFileserveClient::MountDataDirectory(xiiStringView sDataDirectory, xiiStringView sRootName)
{
  XII_LOCK(m_Mutex);
  if (!m_pNetwork->IsConnectedToServer())
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
  if (!m_pNetwork->IsConnectedToServer())
    return;

  xiiRemoteMessage msg('FSRV', 'UMNT');
  msg.GetWriter() << uiDataDir;

  m_pNetwork->Send(xiiRemoteTransmitMode::Reliable, msg);

  auto& dd      = m_MountedDataDirs[uiDataDir];
  dd.m_bMounted = false;
}

void xiiFileserveClient::DeleteFile(xiiUInt16 uiDataDir, xiiStringView sFile)
{
  XII_LOCK(m_Mutex);
  if (!m_pNetwork->IsConnectedToServer())
    return;

  InvalidateFileCache(uiDataDir, sFile, 0);

  xiiRemoteMessage msg('FSRV', 'DELF');
  msg.GetWriter() << uiDataDir;
  msg.GetWriter() << sFile;

  m_pNetwork->Send(xiiRemoteTransmitMode::Reliable, msg);
}

void xiiFileserveClient::HandleFileTransferMsg(xiiRemoteMessage& msg)
{
  XII_LOCK(m_Mutex);
  {
    xiiUuid fileRequestGuid;
    msg.GetReader() >> fileRequestGuid;

    if (fileRequestGuid != m_CurFileRequestGuid)
    {
      // xiiLog::Debug("Fileserver is answering someone else");
      return;
    }
  }

  xiiUInt16 uiChunkSize = 0;
  msg.GetReader() >> uiChunkSize;

  xiiUInt32 uiFileSize = 0;
  msg.GetReader() >> uiFileSize;

  // make sure we don't need to reallocate
  m_Download.Reserve(uiFileSize);

  if (uiChunkSize > 0)
  {
    const xiiUInt32 uiStartPos = m_Download.GetCount();
    m_Download.SetCountUninitialized(uiStartPos + uiChunkSize);
    msg.GetReader().ReadBytes(&m_Download[uiStartPos], uiChunkSize);
  }
}


void xiiFileserveClient::HandleFileTransferFinishedMsg(xiiRemoteMessage& msg)
{
  XII_LOCK(m_Mutex);
  XII_SCOPE_EXIT(m_bDownloading = false);

  {
    xiiUuid fileRequestGuid;
    msg.GetReader() >> fileRequestGuid;

    if (fileRequestGuid != m_CurFileRequestGuid)
    {
      // xiiLog::Debug("Fileserver is answering someone else");
      return;
    }
  }

  xiiFileserveFileState fileState;
  {
    xiiInt8 iFileStatus = 0;
    msg.GetReader() >> iFileStatus;
    fileState = (xiiFileserveFileState)iFileStatus;
  }

  xiiInt64 iFileTimeStamp = 0;
  msg.GetReader() >> iFileTimeStamp;

  xiiUInt64 uiFileHash = 0;
  msg.GetReader() >> uiFileHash;

  xiiUInt16 uiFoundInDataDir = 0;
  msg.GetReader() >> uiFoundInDataDir;

  if (uiFoundInDataDir == 0xffff) // file does not exist on server in any data dir
  {
    m_FileDataDir[m_sCurFileRequest] = 0; // placeholder

    for (xiiUInt32 i = 0; i < m_MountedDataDirs.GetCount(); ++i)
    {
      auto& ref       = m_MountedDataDirs[i].m_CacheStatus[m_sCurFileRequest];
      ref.m_FileHash  = 0;
      ref.m_TimeStamp = 0;
      ref.m_LastCheck = m_CurrentTime;
    }

    return;
  }
  else
  {
    m_FileDataDir[m_sCurFileRequest] = uiFoundInDataDir;

    auto& ref       = m_MountedDataDirs[uiFoundInDataDir].m_CacheStatus[m_sCurFileRequest];
    ref.m_FileHash  = uiFileHash;
    ref.m_TimeStamp = iFileTimeStamp;
    ref.m_LastCheck = m_CurrentTime;
  }

  // nothing changed
  if (fileState == xiiFileserveFileState::SameTimestamp || fileState == xiiFileserveFileState::NonExistantEither)
    return;

  const xiiString& sMountPoint = m_MountedDataDirs[uiFoundInDataDir].m_sMountPoint;
  xiiStringBuilder sCachedFile, sCachedMetaFile;
  BuildPathInCache(m_sCurFileRequest, sMountPoint, &sCachedFile, &sCachedMetaFile);

  if (fileState == xiiFileserveFileState::NonExistant)
  {
    // remove them from the cache as well, if they still exist there
    xiiOSFile::DeleteFile(sCachedFile).IgnoreResult();
    xiiOSFile::DeleteFile(sCachedMetaFile).IgnoreResult();
    return;
  }

  // timestamp changed, but hash is still the same -> update timestamp
  if (fileState == xiiFileserveFileState::SameHash)
  {
    WriteMetaFile(sCachedMetaFile, iFileTimeStamp, uiFileHash);
  }

  if (fileState == xiiFileserveFileState::Different)
  {
    WriteDownloadToDisk(sCachedFile);
    WriteMetaFile(sCachedMetaFile, iFileTimeStamp, uiFileHash);
  }
}


void xiiFileserveClient::WriteMetaFile(xiiStringBuilder sCachedMetaFile, xiiInt64 iFileTimeStamp, xiiUInt64 uiFileHash)
{
  xiiOSFile file;
  if (file.Open(sCachedMetaFile, xiiFileOpenMode::Write).Succeeded())
  {
    file.Write(&iFileTimeStamp, sizeof(xiiInt64)).IgnoreResult();
    file.Write(&uiFileHash, sizeof(xiiUInt64)).IgnoreResult();

    file.Close();
  }
  else
  {
    xiiLog::Error("Failed to write meta file to '{0}'", sCachedMetaFile);
  }
}

void xiiFileserveClient::WriteDownloadToDisk(xiiStringBuilder sCachedFile)
{
  XII_LOCK(m_Mutex);
  xiiOSFile file;
  if (file.Open(sCachedFile, xiiFileOpenMode::Write).Succeeded())
  {
    if (!m_Download.IsEmpty())
      file.Write(m_Download.GetData(), m_Download.GetCount()).IgnoreResult();

    file.Close();
  }
  else
  {
    xiiLog::Error("Failed to write download to '{0}'", sCachedFile);
  }
}

xiiResult xiiFileserveClient::DownloadFile(xiiUInt16 uiDataDirID, const char* szFile, bool bForceThisDataDir, xiiStringBuilder* out_pFullPath)
{
  // bForceThisDataDir = true;
  XII_LOCK(m_Mutex);
  if (m_bDownloading)
  {
    xiiLog::Warning("Trying to download a file over fileserve while another file is already downloading. Recursive download is ignored.");
    return XII_FAILURE;
  }

  XII_ASSERT_DEV(uiDataDirID < m_MountedDataDirs.GetCount(), "Invalid data dir index {0}", uiDataDirID);
  XII_ASSERT_DEV(m_MountedDataDirs[uiDataDirID].m_bMounted, "Data directory {0} is not mounted", uiDataDirID);
  XII_ASSERT_DEV(!m_bDownloading, "Cannot start a download, while one is still running");

  if (!m_pNetwork->IsConnectedToServer())
    return XII_FAILURE;

  bool bCachedYet    = false;
  auto itFileDataDir = m_FileDataDir.FindOrAdd(szFile, &bCachedYet);
  if (!bCachedYet)
  {
    FillFileStatusCache(szFile);
  }

  const xiiUInt16        uiUseDataDirCache = bForceThisDataDir ? uiDataDirID : itFileDataDir.Value();
  const FileCacheStatus& CacheStatus       = m_MountedDataDirs[uiUseDataDirCache].m_CacheStatus[szFile];

  if (m_CurrentTime - CacheStatus.m_LastCheck < xiiTime::Seconds(5.0f))
  {
    if (CacheStatus.m_FileHash == 0) // file does not exist
      return XII_FAILURE;

    if (out_pFullPath)
      BuildPathInCache(szFile, m_MountedDataDirs[uiUseDataDirCache].m_sMountPoint, out_pFullPath, nullptr);

    return XII_SUCCESS;
  }

  m_Download.Clear();
  m_sCurFileRequest = szFile;
  m_CurFileRequestGuid.CreateNewUuid();
  m_bDownloading = true;

  xiiRemoteMessage msg('FSRV', 'READ');
  msg.GetWriter() << uiUseDataDirCache;
  msg.GetWriter() << bForceThisDataDir;
  msg.GetWriter() << szFile;
  msg.GetWriter() << m_CurFileRequestGuid;
  msg.GetWriter() << CacheStatus.m_TimeStamp;
  msg.GetWriter() << CacheStatus.m_FileHash;

  m_pNetwork->Send(xiiRemoteTransmitMode::Reliable, msg);

  while (m_bDownloading)
  {
    m_pNetwork->UpdateRemoteInterface();
    m_pNetwork->ExecuteAllMessageHandlers();
  }

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

    meta.Read(&out_Status.m_TimeStamp, sizeof(xiiInt64));
    meta.Read(&out_Status.m_FileHash, sizeof(xiiUInt64));
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

xiiResult xiiFileserveClient::SearchForServerAddress(xiiTime timeout /*= xiiTime::Seconds(5)*/)
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
  if (network->ConnectToServer('XIIFS', szAddress, false).Failed())
    return XII_FAILURE;

  bool bServerFound = false;
  network->SetMessageHandler('FSRV', [&bServerFound](xiiRemoteMessage& msg) {
    switch (msg.GetMessageID())
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

      xiiThreadUtils::Sleep(xiiTime::Milliseconds(100));

      network->UpdateRemoteInterface();
      network->ExecuteAllMessageHandlers();
    }
  }

  network->ShutdownConnection();

  if (!bServerFound)
    return XII_FAILURE;

  m_sServerConnectionAddress = szAddress;

  // always store the IP that was successful in the user directory
  SaveCurrentConnectionInfoToDisk().IgnoreResult();
  return XII_SUCCESS;
}

xiiResult xiiFileserveClient::WaitForServerInfo(xiiTime timeout /*= xiiTime::Seconds(60.0 * 5)*/)
{
  XII_LOCK(m_Mutex);
  if (!s_bEnableFileserve)
    return XII_FAILURE;

  xiiUInt16                           uiPort = 1042;
  xiiHybridArray<xiiStringBuilder, 4> sServerIPs;

  {
    xiiUniquePtr<xiiRemoteInterfaceEnet> network = xiiRemoteInterfaceEnet::Make(); /// \todo Abstract this somehow ?
    network->SetMessageHandler('FSRV', [&sServerIPs, &uiPort](xiiRemoteMessage& msg) {
        switch (msg.GetMessageID())
        {
          case 'MYIP':
            msg.GetReader() >> uiPort;

            xiiUInt8 uiCount = 0;
            msg.GetReader() >> uiCount;

            sServerIPs.SetCount(uiCount);
            for (xiiUInt32 i = 0; i < uiCount; ++i)
            {
              msg.GetReader() >> sServerIPs[i];
            }

            break;
        } });

    XII_SUCCEED_OR_RETURN(network->StartServer('XIIP', "2042", false));

    xiiTime tStart = xiiTime::Now();
    while (xiiTime::Now() - tStart < timeout && sServerIPs.IsEmpty())
    {
      xiiThreadUtils::Sleep(xiiTime::Milliseconds(1));

      network->UpdateRemoteInterface();
      network->ExecuteAllMessageHandlers();
    }

    network->ShutdownConnection();
  }

  if (sServerIPs.IsEmpty())
    return XII_FAILURE;

  // network connections are unreliable and surprisingly slow sometimes
  // we just got an IP from a server, so we know it's there and we should be able to connect to it
  // still this often fails the first few times
  // so we try this several times and waste some time in between and hope that at some point the connection succeeds
  for (xiiUInt32 i = 0; i < 8; ++i)
  {
    xiiStringBuilder sAddress;
    for (auto& ip : sServerIPs)
    {
      sAddress.Format("{0}:{1}", ip, uiPort);

      xiiThreadUtils::Sleep(xiiTime::Milliseconds(500));

      if (TryConnectWithFileserver(sAddress, xiiTime::Seconds(3)).Succeeded())
        return XII_SUCCESS;
    }

    xiiThreadUtils::Sleep(xiiTime::Milliseconds(1000));
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
