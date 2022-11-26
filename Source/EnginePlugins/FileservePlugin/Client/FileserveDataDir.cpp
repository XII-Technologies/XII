#include <FileservePlugin/FileservePluginPCH.h>

#include <FileservePlugin/Client/FileserveDataDir.h>
#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Communication/RemoteInterfaceEnet.h>
#include <Foundation/Logging/Log.h>

void xiiDataDirectory::FileserveType::ReloadExternalConfigs()
{
  XII_LOCK(m_RedirectionMutex);
  m_FileRedirection.Clear();

  if (!s_sRedirectionFile.IsEmpty())
  {
    xiiFileserveClient::GetSingleton()->DownloadFile(m_uiDataDirID, s_sRedirectionFile, true, nullptr).IgnoreResult();
  }

  FolderType::ReloadExternalConfigs();
}

xiiDataDirectoryReader* xiiDataDirectory::FileserveType::OpenFileToRead(const char* szFile, xiiFileShareMode::Enum FileShareMode, bool bSpecificallyThisDataDir)
{
  // fileserve cannot handle absolute paths, which is actually already ruled out at creation time, so this is just an optimization
  if (xiiPathUtils::IsAbsolutePath(szFile))
    return nullptr;

  xiiStringBuilder sRedirected;
  if (ResolveAssetRedirection(szFile, sRedirected))
    bSpecificallyThisDataDir = true; // If this data dir can resolve the guid, only this should load it as well.

  // we know that the server cannot resolve asset GUIDs, so don't even ask
  if (xiiConversionUtils::IsStringUuid(sRedirected))
    return nullptr;

  xiiStringBuilder sFullPath;
  if (xiiFileserveClient::GetSingleton()->DownloadFile(m_uiDataDirID, sRedirected, bSpecificallyThisDataDir, &sFullPath).Failed())
    return nullptr;

  // It's fine to use the base class here as it will resurface in CreateFolderReader which gives us control of the important part.
  return FolderType::OpenFileToRead(sFullPath, FileShareMode, bSpecificallyThisDataDir);
}

xiiDataDirectoryWriter* xiiDataDirectory::FileserveType::OpenFileToWrite(const char* szFile, xiiFileShareMode::Enum FileShareMode)
{
  // fileserve cannot handle absolute paths, which is actually already ruled out at creation time, so this is just an optimization
  if (xiiPathUtils::IsAbsolutePath(szFile))
    return nullptr;

  return FolderType::OpenFileToWrite(szFile, FileShareMode);
}

xiiResult xiiDataDirectory::FileserveType::InternalInitializeDataDirectory(const char* szDirectory)
{
  xiiStringBuilder sDataDir = szDirectory;
  sDataDir.MakeCleanPath();

  xiiStringBuilder sCacheFolder, sCacheMetaFolder;
  xiiFileserveClient::GetSingleton()->GetFullDataDirCachePath(sDataDir, sCacheFolder, sCacheMetaFolder);
  m_sRedirectedDataDirPath    = sCacheFolder;
  m_sFileserveCacheMetaFolder = sCacheMetaFolder;

  ReloadExternalConfigs();
  return XII_SUCCESS;
}

void xiiDataDirectory::FileserveType::RemoveDataDirectory()
{
  if (xiiFileserveClient::GetSingleton())
  {
    xiiFileserveClient::GetSingleton()->UnmountDataDirectory(m_uiDataDirID);
  }

  FolderType::RemoveDataDirectory();
}

void xiiDataDirectory::FileserveType::DeleteFile(const char* szFile)
{
  if (xiiFileserveClient::GetSingleton())
  {
    xiiFileserveClient::GetSingleton()->DeleteFile(m_uiDataDirID, szFile);
  }

  FolderType::DeleteFile(szFile);
}

xiiDataDirectory::FolderReader* xiiDataDirectory::FileserveType::CreateFolderReader() const
{
  return XII_DEFAULT_NEW(FileserveDataDirectoryReader, 0);
}

xiiDataDirectory::FolderWriter* xiiDataDirectory::FileserveType::CreateFolderWriter() const
{
  return XII_DEFAULT_NEW(FileserveDataDirectoryWriter);
}

xiiResult xiiDataDirectory::FileserveType::GetFileStats(const char* szFileOrFolder, bool bOneSpecificDataDir, xiiFileStats& out_Stats)
{
  xiiStringBuilder sRedirected;
  if (ResolveAssetRedirection(szFileOrFolder, sRedirected))
    bOneSpecificDataDir = true; // If this data dir can resolve the guid, only this should load it as well.

  // we know that the server cannot resolve asset GUIDs, so don't even ask
  if (xiiConversionUtils::IsStringUuid(sRedirected))
    return XII_FAILURE;

  xiiStringBuilder sFullPath;
  XII_SUCCEED_OR_RETURN(xiiFileserveClient::GetSingleton()->DownloadFile(m_uiDataDirID, sRedirected, bOneSpecificDataDir, &sFullPath));
  return xiiOSFile::GetFileStats(sFullPath, out_Stats);
}

bool xiiDataDirectory::FileserveType::ExistsFile(const char* szFile, bool bOneSpecificDataDir)
{
  xiiStringBuilder sRedirected;
  if (ResolveAssetRedirection(szFile, sRedirected))
    bOneSpecificDataDir = true; // If this data dir can resolve the guid, only this should load it as well.

  // we know that the server cannot resolve asset GUIDs, so don't even ask
  if (xiiConversionUtils::IsStringUuid(sRedirected))
    return false;

  return xiiFileserveClient::GetSingleton()->DownloadFile(m_uiDataDirID, sRedirected, bOneSpecificDataDir, nullptr).Succeeded();
}

xiiDataDirectoryType* xiiDataDirectory::FileserveType::Factory(const char* szDataDirectory, const char* szGroup, const char* szRootName, xiiFileSystem::DataDirUsage Usage)
{
  if (!xiiFileserveClient::s_bEnableFileserve || xiiFileserveClient::GetSingleton() == nullptr)
    return nullptr; // this would only happen if the functionality is switched off, but not before the factory was added

  // ignore the empty data dir, which handles absolute paths, as we cannot translate these paths to the fileserve host OS
  if (xiiStringUtils::IsNullOrEmpty(szDataDirectory))
    return nullptr;

  // Fileserve can only translate paths on the server that start with a 'Special Directory' (e.g. ">sdk/" or ">project/")
  // ignore everything else
  if (szDataDirectory[0] != '>')
    return nullptr;

  if (xiiFileserveClient::GetSingleton()->EnsureConnected().Failed())
    return nullptr;

  xiiDataDirectory::FileserveType* pDataDir = XII_DEFAULT_NEW(xiiDataDirectory::FileserveType);
  pDataDir->m_uiDataDirID                   = xiiFileserveClient::GetSingleton()->MountDataDirectory(szDataDirectory, szRootName);

  if (pDataDir->m_uiDataDirID < 0xffff && pDataDir->InitializeDataDirectory(szDataDirectory) == XII_SUCCESS)
    return pDataDir;

  XII_DEFAULT_DELETE(pDataDir);
  return nullptr;
}

xiiDataDirectory::FileserveDataDirectoryReader::FileserveDataDirectoryReader(xiiInt32 iDataDirUserData) :
  FolderReader(iDataDirUserData)
{
}

xiiResult xiiDataDirectory::FileserveDataDirectoryReader::InternalOpen(xiiFileShareMode::Enum FileShareMode)
{
  return m_File.Open(GetFilePath().GetData(), xiiFileOpenMode::Read, FileShareMode);
}

void xiiDataDirectory::FileserveDataDirectoryWriter::InternalClose()
{
  FolderWriter::InternalClose();

  static_cast<FileserveType*>(GetDataDirectory())->FinishedWriting(this);
}

void xiiDataDirectory::FileserveType::FinishedWriting(FolderWriter* pWriter)
{
  if (xiiFileserveClient::GetSingleton() == nullptr)
    return;

  xiiStringBuilder sAbsPath = pWriter->GetDataDirectory()->GetRedirectedDataDirectoryPath();
  sAbsPath.AppendPath(pWriter->GetFilePath());

  xiiOSFile file;
  if (file.Open(sAbsPath, xiiFileOpenMode::Read).Failed())
  {
    xiiLog::Error("Could not read file for upload: '{0}'", sAbsPath);
    return;
  }

  xiiDynamicArray<xiiUInt8> content;
  file.ReadAll(content);
  file.Close();

  xiiFileserveClient::GetSingleton()->UploadFile(m_uiDataDirID, pWriter->GetFilePath(), content);
}



XII_STATICLINK_FILE(FileservePlugin, FileservePlugin_Client_FileserveDataDir);
