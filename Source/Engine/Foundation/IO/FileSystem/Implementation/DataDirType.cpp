/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>

xiiResult xiiDataDirectoryType::InitializeDataDirectory(xiiStringView sDataDirPath)
{
  xiiStringBuilder sPath = sDataDirPath;
  sPath.MakeCleanPath();

  XII_ASSERT_DEV(sPath.IsEmpty() || sPath.EndsWith("/"), "Data directory path must end with a slash.");

  m_sDataDirectoryPath = sPath;

  return InternalInitializeDataDirectory(m_sDataDirectoryPath.GetData());
}

bool xiiDataDirectoryType::ExistsFile(xiiStringView sFile, bool bOneSpecificDataDir)
{
  XII_IGNORE_UNUSED(bOneSpecificDataDir);

  xiiStringBuilder sRedirectedAsset;
  ResolveAssetRedirection(sFile, sRedirectedAsset);

  xiiStringBuilder sPath = GetRedirectedDataDirectoryPath();
  sPath.AppendPath(sRedirectedAsset);
  return xiiOSFile::ExistsFile(sPath);
}

void xiiDataDirectoryReaderWriterBase::Close()
{
  InternalClose();

  xiiFileSystem::FileEvent fe;
  fe.m_EventType        = xiiFileSystem::FileEventType::CloseFile;
  fe.m_sFileOrDirectory = GetFilePath();
  fe.m_pDataDir         = m_pDataDirType;
  xiiFileSystem::s_pData->m_Event.Broadcast(fe);

  m_pDataDirType->OnReaderWriterClose(this);
}

XII_STATICLINK_FILE(Foundation, Foundation_IO_FileSystem_Implementation_DataDirType);
