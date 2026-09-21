/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <FileservePlugin/Client/FileserveClient.h>
#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/UniquePtr.h>

namespace xiiDataDirectory
{
  class FileserveDataDirectoryReader : public FolderReader
  {
  public:
    FileserveDataDirectoryReader(xiiInt32 iDataDirUserData);

  protected:
    virtual xiiResult InternalOpen(xiiFileShareMode::Enum FileShareMode) override;
  };

  class FileserveDataDirectoryWriter : public FolderWriter
  {
  protected:
    virtual void InternalClose() override;
  };

  /// A data directory type to handle access to files that are served from a network host.
  class XII_FILESERVEPLUGIN_DLL FileserveType : public FolderType
  {
  public:
    /// The factory that can be registered at xiiFileSystem to create data directories of this type.
    static xiiDataDirectoryType* Factory(xiiStringView sDataDirectory, xiiStringView sGroup, xiiStringView sRootName, xiiDataDirUsage usage);

    /// [internal] Makes sure the redirection config files are up to date and then reloads them.
    virtual void ReloadExternalConfigs() override;

    /// [internal] Called by FileserveDataDirectoryWriter when it is finished to upload the written file to the server
    void FinishedWriting(FolderWriter* pWriter);

  protected:
    virtual xiiDataDirectoryReader* OpenFileToRead(xiiStringView sFile, xiiFileShareMode::Enum FileShareMode, bool bSpecificallyThisDataDir) override;
    virtual xiiDataDirectoryWriter* OpenFileToWrite(xiiStringView sFile, xiiFileShareMode::Enum FileShareMode) override;
    virtual xiiResult               InternalInitializeDataDirectory(xiiStringView sDirectory) override;
    virtual void                    RemoveDataDirectory() override;
    virtual void                    DeleteFile(xiiStringView sFile) override;
    virtual bool                    ExistsFile(xiiStringView sFile, bool bOneSpecificDataDir) override;
    /// Limitation: Fileserve does not handle folders, only files. If someone stats a folder, this will fail.
    virtual xiiResult     GetFileStats(xiiStringView sFileOrFolder, bool bOneSpecificDataDir, xiiFileStats& out_Stats) override;
    virtual FolderReader* CreateFolderReader() const override;
    virtual FolderWriter* CreateFolderWriter() const override;

    xiiUInt16    m_uiDataDirID = 0xffff;
    xiiString128 m_sFileserveCacheMetaFolder;
  };
} // namespace xiiDataDirectory
