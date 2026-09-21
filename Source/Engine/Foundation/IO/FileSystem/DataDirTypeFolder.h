/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/IO/OSFile.h>

namespace xiiDataDirectory
{
  class FolderReader;
  class FolderWriter;

  /// A data directory type to handle access to ordinary files.
  ///
  /// Register the 'Factory' function at xiiFileSystem to allow it to mount local directories.
  class XII_FOUNDATION_DLL FolderType : public xiiDataDirectoryType
  {
  public:
    ~FolderType();

    /// The factory that can be registered at xiiFileSystem to create data directories of this type.
    static xiiDataDirectoryType* Factory(xiiStringView sDataDirectory, xiiStringView sGroup, xiiStringView sRootName, xiiDataDirUsage usage);

    /// A 'redirection file' is an optional file located inside a data directory that lists which file access is redirected to which other
    /// file lookup. Each redirection is one line in the file (terminated by a \n). Each line consists of the 'key' string, a semicolon and
    /// a 'value' string. No unnecessary whitespace is allowed. When a file that matches 'key' is accessed through a mounted data directory,
    /// the file access will be replaced by 'value' (plus s_sRedirectionPrefix) 'key' may be anything (e.g. a GUID string), 'value' should
    /// be a valid relative path into the SAME data directory. The redirection file can be used to implement an asset lookup, where assets
    /// are identified by GUIDs and need to be mapped to the actual asset file.
    static xiiString s_sRedirectionFile;

    /// If a redirection file is used AND the redirection lookup was successful, s_sRedirectionPrefix is prepended to the redirected file
    /// access.
    static xiiString s_sRedirectionPrefix;

    /// When s_sRedirectionFile and s_sRedirectionPrefix are used to enable file redirection, this will reload those config files.
    virtual void ReloadExternalConfigs() override;

    virtual const xiiString128& GetRedirectedDataDirectoryPath() const override { return m_sRedirectedDataDirPath; }

  protected:
    // The implementations of the abstract functions.

    virtual xiiDataDirectoryReader* OpenFileToRead(xiiStringView sFile, xiiFileShareMode::Enum FileShareMode, bool bSpecificallyThisDataDir) override;

    virtual bool                    ResolveAssetRedirection(xiiStringView sPathOrAssetGuid, xiiStringBuilder& out_sRedirection) override;
    virtual xiiDataDirectoryWriter* OpenFileToWrite(xiiStringView sFile, xiiFileShareMode::Enum FileShareMode) override;
    virtual void                    RemoveDataDirectory() override;
    virtual void                    DeleteFile(xiiStringView sFile) override;
    virtual bool                    ExistsFile(xiiStringView sFile, bool bOneSpecificDataDir) override;
    virtual xiiResult               GetFileStats(xiiStringView sFileOrFolder, bool bOneSpecificDataDir, xiiFileStats& out_Stats) override;
    virtual FolderReader*           CreateFolderReader() const;
    virtual FolderWriter*           CreateFolderWriter() const;

    /// Called by 'xiiDataDirectoryType_Folder::Factory'
    virtual xiiResult InternalInitializeDataDirectory(xiiStringView sDirectory) override;

    /// Marks the given reader/writer as reusable.
    virtual void OnReaderWriterClose(xiiDataDirectoryReaderWriterBase* pClosed) override;

    void LoadRedirectionFile();

    mutable xiiMutex                                   m_ReaderWriterMutex; ///< Locks m_Readers / m_Writers as well as the m_bIsInUse flag of each reader / writer.
    xiiHybridArray<xiiDataDirectory::FolderReader*, 4> m_Readers;
    xiiHybridArray<xiiDataDirectory::FolderWriter*, 4> m_Writers;

    mutable xiiMutex             m_RedirectionMutex;
    xiiMap<xiiString, xiiString> m_FileRedirection;
    xiiString128                 m_sRedirectedDataDirPath;
  };


  /// Handles reading from ordinary files.
  class XII_FOUNDATION_DLL FolderReader : public xiiDataDirectoryReader
  {
    XII_DISALLOW_COPY_AND_ASSIGN(FolderReader);

  public:
    FolderReader(xiiInt32 iDataDirUserData) :
      xiiDataDirectoryReader(iDataDirUserData)
    {
      m_bIsInUse = false;
    }

    virtual xiiUInt64 Skip(xiiUInt64 uiBytes) override;
    virtual xiiUInt64 Read(void* pBuffer, xiiUInt64 uiBytes) override;
    virtual xiiUInt64 GetFileSize() const override;

  protected:
    virtual xiiResult InternalOpen(xiiFileShareMode::Enum FileShareMode) override;
    virtual void      InternalClose() override;

    friend class FolderType;

    bool      m_bIsInUse;
    xiiOSFile m_File;
  };

  /// Handles writing to ordinary files.
  class XII_FOUNDATION_DLL FolderWriter : public xiiDataDirectoryWriter
  {
    XII_DISALLOW_COPY_AND_ASSIGN(FolderWriter);

  public:
    FolderWriter(xiiInt32 iDataDirUserData = 0) :
      xiiDataDirectoryWriter(iDataDirUserData)
    {
      m_bIsInUse = false;
    }

    virtual xiiResult Write(const void* pBuffer, xiiUInt64 uiBytes) override;
    virtual xiiUInt64 GetFileSize() const override;

  protected:
    virtual xiiResult InternalOpen(xiiFileShareMode::Enum FileShareMode) override;
    virtual void      InternalClose() override;

    friend class FolderType;

    bool      m_bIsInUse;
    xiiOSFile m_File;
  };
} // namespace xiiDataDirectory
