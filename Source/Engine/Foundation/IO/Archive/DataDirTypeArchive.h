/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/IO/Archive/ArchiveReader.h>
#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Time/Timestamp.h>

class xiiArchiveEntry;

namespace xiiDataDirectory
{
  class ArchiveReaderUncompressed;
  class ArchiveReaderZstd;
  class ArchiveReaderZip;

  class XII_FOUNDATION_DLL ArchiveType : public xiiDataDirectoryType
  {
  public:
    ArchiveType();
    ~ArchiveType();

    static xiiDataDirectoryType* Factory(xiiStringView sDataDirectory, xiiStringView sGroup, xiiStringView sRootName, xiiDataDirUsage usage);

    virtual const xiiString128& GetRedirectedDataDirectoryPath() const override { return m_sRedirectedDataDirPath; }

  protected:
    virtual xiiDataDirectoryReader* OpenFileToRead(xiiStringView sFile, xiiFileShareMode::Enum FileShareMode, bool bSpecificallyThisDataDir) override;

    virtual void RemoveDataDirectory() override;

    virtual bool ExistsFile(xiiStringView sFile, bool bOneSpecificDataDir) override;

    virtual xiiResult GetFileStats(xiiStringView sFileOrFolder, bool bOneSpecificDataDir, xiiFileStats& out_Stats) override;

    virtual xiiResult InternalInitializeDataDirectory(xiiStringView sDirectory) override;

    virtual void OnReaderWriterClose(xiiDataDirectoryReaderWriterBase* pClosed) override;

    xiiString128     m_sRedirectedDataDirPath;
    xiiString32      m_sArchiveSubFolder;
    xiiTimestamp     m_LastModificationTime;
    xiiArchiveReader m_ArchiveReader;

    xiiMutex                                                   m_ReaderMutex;
    xiiHybridArray<xiiUniquePtr<ArchiveReaderUncompressed>, 4> m_ReadersUncompressed;
    xiiHybridArray<ArchiveReaderUncompressed*, 4>              m_FreeReadersUncompressed;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    xiiHybridArray<xiiUniquePtr<ArchiveReaderZstd>, 4> m_ReadersZstd;
    xiiHybridArray<ArchiveReaderZstd*, 4>              m_FreeReadersZstd;
#endif
  };

  class XII_FOUNDATION_DLL ArchiveReaderCommon : public xiiDataDirectoryReader
  {
    XII_DISALLOW_COPY_AND_ASSIGN(ArchiveReaderCommon);

  public:
    ArchiveReaderCommon(xiiInt32 iDataDirUserData);

    virtual xiiUInt64 GetFileSize() const override;

  protected:
    friend class ArchiveType;

    xiiUInt64                m_uiUncompressedSize = 0;
    xiiUInt64                m_uiCompressedSize   = 0;
    xiiRawMemoryStreamReader m_MemStreamReader;
  };

  class XII_FOUNDATION_DLL ArchiveReaderUncompressed : public ArchiveReaderCommon
  {
    XII_DISALLOW_COPY_AND_ASSIGN(ArchiveReaderUncompressed);

  public:
    ArchiveReaderUncompressed(xiiInt32 iDataDirUserData);

    virtual xiiUInt64 Skip(xiiUInt64 uiBytes) override;
    virtual xiiUInt64 Read(void* pBuffer, xiiUInt64 uiBytes) override;

  protected:
    virtual xiiResult InternalOpen(xiiFileShareMode::Enum FileShareMode) override;
    virtual void      InternalClose() override;
  };

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  class XII_FOUNDATION_DLL ArchiveReaderZstd : public ArchiveReaderCommon
  {
    XII_DISALLOW_COPY_AND_ASSIGN(ArchiveReaderZstd);

  public:
    ArchiveReaderZstd(xiiInt32 iDataDirUserData);

    virtual xiiUInt64 Read(void* pBuffer, xiiUInt64 uiBytes) override;

  protected:
    virtual xiiResult InternalOpen(xiiFileShareMode::Enum FileShareMode) override;
    virtual void      InternalClose() override;

    xiiCompressedStreamReaderZstd m_CompressedStreamReader;
  };
#endif
} // namespace xiiDataDirectory
