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

xiiDataDirectoryReader* xiiDataDirectory::FileserveType::OpenFileToRead(xiiStringView sFile, xiiFileShareMode::Enum FileShareMode, bool bSpecificallyThisDataDir)
{
  // fileserve cannot handle absolute paths, which is actually already ruled out at creation time, so this is just an optimization
  if (xiiPathUtils::IsAbsolutePath(sFile))
    return nullptr;

  xiiStringBuilder sRedirected;
  if (ResolveAssetRedirection(sFile, sRedirected))
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

xiiDataDirectoryWriter* xiiDataDirectory::FileserveType::OpenFileToWrite(xiiStringView sFile, xiiFileShareMode::Enum FileShareMode)
{
  // fileserve cannot handle absolute paths, which is actually already ruled out at creation time, so this is just an optimization
  if (xiiPathUtils::IsAbsolutePath(sFile))
    return nullptr;

  return FolderType::OpenFileToWrite(sFile, FileShareMode);
}

xiiResult xiiDataDirectory::FileserveType::InternalInitializeDataDirectory(xiiStringView sDirectory)
{
  xiiStringBuilder sDataDir = sDirectory;
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

void xiiDataDirectory::FileserveType::DeleteFile(xiiStringView sFile)
{
  if (xiiFileserveClient::GetSingleton())
  {
    xiiFileserveClient::GetSingleton()->DeleteFile(m_uiDataDirID, sFile);
  }

  FolderType::DeleteFile(sFile);
}

xiiDataDirectory::FolderReader* xiiDataDirectory::FileserveType::CreateFolderReader() const
{
  return XII_DEFAULT_NEW(FileserveDataDirectoryReader, 0);
}

xiiDataDirectory::FolderWriter* xiiDataDirectory::FileserveType::CreateFolderWriter() const
{
  return XII_DEFAULT_NEW(FileserveDataDirectoryWriter);
}

xiiResult xiiDataDirectory::FileserveType::GetFileStats(xiiStringView sFileOrFolder, bool bOneSpecificDataDir, xiiFileStats& out_Stats)
{
  xiiStringBuilder sRedirected;
  if (ResolveAssetRedirection(sFileOrFolder, sRedirected))
    bOneSpecificDataDir = true; // If this data dir can resolve the guid, only this should load it as well.

  // we know that the server cannot resolve asset GUIDs, so don't even ask
  if (xiiConversionUtils::IsStringUuid(sRedirected))
    return XII_FAILURE;

  xiiStringBuilder sFullPath;
  XII_SUCCEED_OR_RETURN(xiiFileserveClient::GetSingleton()->DownloadFile(m_uiDataDirID, sRedirected, bOneSpecificDataDir, &sFullPath));
  return xiiOSFile::GetFileStats(sFullPath, out_Stats);
}

bool xiiDataDirectory::FileserveType::ExistsFile(xiiStringView sFile, bool bOneSpecificDataDir)
{
  xiiStringBuilder sRedirected;
  if (ResolveAssetRedirection(sFile, sRedirected))
    bOneSpecificDataDir = true; // If this data dir can resolve the guid, only this should load it as well.

  // we know that the server cannot resolve asset GUIDs, so don't even ask
  if (xiiConversionUtils::IsStringUuid(sRedirected))
    return false;

  return xiiFileserveClient::GetSingleton()->DownloadFile(m_uiDataDirID, sRedirected, bOneSpecificDataDir, nullptr).Succeeded();
}

xiiDataDirectoryType* xiiDataDirectory::FileserveType::Factory(xiiStringView sDataDirectory, xiiStringView sGroup, xiiStringView sRootName, xiiFileSystem::DataDirUsage Usage)
{
  if (!xiiFileserveClient::s_bEnableFileserve || xiiFileserveClient::GetSingleton() == nullptr)
    return nullptr; // this would only happen if the functionality is switched off, but not before the factory was added

  // ignore the empty data dir, which handles absolute paths, as we cannot translate these paths to the fileserve host OS
  if (sDataDirectory.IsEmpty())
    return nullptr;

  // Fileserve can only translate paths on the server that start with a 'Special Directory' (e.g. ">sdk/" or ">project/")
  // ignore everything else
  if (!sDataDirectory.StartsWith(">"))
    return nullptr;

  if (xiiFileserveClient::GetSingleton()->EnsureConnected().Failed())
    return nullptr;

  xiiDataDirectory::FileserveType* pDataDir = XII_DEFAULT_NEW(xiiDataDirectory::FileserveType);
  pDataDir->m_uiDataDirID                   = xiiFileserveClient::GetSingleton()->MountDataDirectory(sDataDirectory, sRootName);

  if (pDataDir->m_uiDataDirID < 0xffff && pDataDir->InitializeDataDirectory(sDataDirectory) == XII_SUCCESS)
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
