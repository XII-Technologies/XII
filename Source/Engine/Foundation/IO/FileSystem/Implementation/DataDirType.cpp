#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>

xiiResult xiiDataDirectoryType::InitializeDataDirectory(const char* szDataDirPath)
{
  xiiStringBuilder sPath = szDataDirPath;
  sPath.MakeCleanPath();

  XII_ASSERT_DEV(sPath.IsEmpty() || sPath.EndsWith("/"), "Data directory path must end with a slash.");

  m_sDataDirectoryPath = sPath;

  return InternalInitializeDataDirectory(m_sDataDirectoryPath.GetData());
}

bool xiiDataDirectoryType::ExistsFile(const char* szFile, bool bOneSpecificDataDir)
{
  xiiStringBuilder sRedirectedAsset;
  ResolveAssetRedirection(szFile, sRedirectedAsset);

  xiiStringBuilder sPath = GetRedirectedDataDirectoryPath();
  sPath.AppendPath(sRedirectedAsset);
  return xiiOSFile::ExistsFile(sPath);
}

void xiiDataDirectoryReaderWriterBase::Close()
{
  InternalClose();

  xiiFileSystem::FileEvent fe;
  fe.m_EventType         = xiiFileSystem::FileEventType::CloseFile;
  fe.m_szFileOrDirectory = GetFilePath().GetData();
  fe.m_pDataDir          = m_pDataDirectory;
  xiiFileSystem::s_pData->m_Event.Broadcast(fe);

  m_pDataDirectory->OnReaderWriterClose(this);
}



XII_STATICLINK_FILE(Foundation, Foundation_IO_FileSystem_Implementation_DataDirType);
