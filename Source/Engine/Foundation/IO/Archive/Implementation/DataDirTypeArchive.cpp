/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/Archive/ArchiveUtils.h>
#include <Foundation/IO/Archive/DataDirTypeArchive.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Foundation, ArchiveDataDirectory)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "FileSystem", "FolderDataDirectory"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
  xiiFileSystem::RegisterDataDirectoryFactory(xiiDataDirectory::ArchiveType::Factory);
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiDataDirectory::ArchiveType::ArchiveType()  = default;
xiiDataDirectory::ArchiveType::~ArchiveType() = default;

xiiDataDirectoryType* xiiDataDirectory::ArchiveType::Factory(xiiStringView sDataDirectory, xiiStringView sGroup, xiiStringView sRootName, xiiDataDirUsage usage)
{
  XII_IGNORE_UNUSED(sGroup);
  XII_IGNORE_UNUSED(sRootName);
  XII_IGNORE_UNUSED(usage);

  ArchiveType* pDataDir = XII_DEFAULT_NEW(ArchiveType);

  if (pDataDir->InitializeDataDirectory(sDataDirectory) == XII_SUCCESS)
    return pDataDir;

  XII_DEFAULT_DELETE(pDataDir);
  return nullptr;
}

xiiDataDirectoryReader* xiiDataDirectory::ArchiveType::OpenFileToRead(xiiStringView sFile, xiiFileShareMode::Enum FileShareMode, bool bSpecificallyThisDataDir)
{
  XII_IGNORE_UNUSED(bSpecificallyThisDataDir);

  const xiiArchiveTOC& toc          = m_ArchiveReader.GetArchiveTOC();
  xiiStringBuilder     sArchivePath = m_sArchiveSubFolder;
  sArchivePath.AppendPath(sFile);
  sArchivePath.MakeCleanPath();

  const xiiUInt32 uiEntryIndex = toc.FindEntry(sArchivePath);

  if (uiEntryIndex == xiiInvalidIndex)
    return nullptr;

  const xiiArchiveEntry* pEntry = &toc.m_Entries[uiEntryIndex];

  ArchiveReaderCommon* pReader = nullptr;

  {
    XII_LOCK(m_ReaderMutex);

    switch (pEntry->m_CompressionMode)
    {
      case xiiArchiveCompressionMode::Uncompressed:
      {
        if (!m_FreeReadersUncompressed.IsEmpty())
        {
          pReader = m_FreeReadersUncompressed.PeekBack();
          m_FreeReadersUncompressed.PopBack();
        }
        else
        {
          m_ReadersUncompressed.PushBack(XII_DEFAULT_NEW(ArchiveReaderUncompressed, 0));
          pReader = m_ReadersUncompressed.PeekBack().Borrow();
        }
        break;
      }

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
      case xiiArchiveCompressionMode::Compressed_zstd:
      {
        if (!m_FreeReadersZstd.IsEmpty())
        {
          pReader = m_FreeReadersZstd.PeekBack();
          m_FreeReadersZstd.PopBack();
        }
        else
        {
          m_ReadersZstd.PushBack(XII_DEFAULT_NEW(ArchiveReaderZstd, 1));
          pReader = m_ReadersZstd.PeekBack().Borrow();
        }
        break;
      }
#endif
#ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT
      case xiiArchiveCompressionMode::Compressed_zip:
      {
        if (!m_FreeReadersZip.IsEmpty())
        {
          pReader = m_FreeReadersZip.PeekBack();
          m_FreeReadersZip.PopBack();
        }
        else
        {
          m_ReadersZip.PushBack(XII_DEFAULT_NEW(ArchiveReaderZip, 2));
          pReader = m_ReadersZip.PeekBack().Borrow();
        }
        break;
      }
#endif

      default:
        XII_REPORT_FAILURE("Compression mode {} is unknown (or not compiled in)", (xiiUInt8)pEntry->m_CompressionMode);
        return nullptr;
    }
  }

  pReader->m_uiUncompressedSize = pEntry->m_uiUncompressedDataSize;
  pReader->m_uiCompressedSize   = pEntry->m_uiStoredDataSize;

  m_ArchiveReader.ConfigureRawMemoryStreamReader(uiEntryIndex, pReader->m_MemStreamReader);

  if (pReader->Open(sArchivePath, this, FileShareMode).Failed())
  {
    XII_DEFAULT_DELETE(pReader);
    return nullptr;
  }

  return pReader;
}

void xiiDataDirectory::ArchiveType::RemoveDataDirectory()
{
  ArchiveType* pThis = this;
  XII_DEFAULT_DELETE(pThis);
}

bool xiiDataDirectory::ArchiveType::ExistsFile(xiiStringView sFile, bool bOneSpecificDataDir)
{
  XII_IGNORE_UNUSED(bOneSpecificDataDir);

  xiiStringBuilder sArchivePath = m_sArchiveSubFolder;
  sArchivePath.AppendPath(sFile);
  sArchivePath.MakeCleanPath();
  return m_ArchiveReader.GetArchiveTOC().FindEntry(sArchivePath) != xiiInvalidIndex;
}

xiiResult xiiDataDirectory::ArchiveType::GetFileStats(xiiStringView sFileOrFolder, bool bOneSpecificDataDir, xiiFileStats& out_Stats)
{
  XII_IGNORE_UNUSED(bOneSpecificDataDir);

  const xiiArchiveTOC& toc          = m_ArchiveReader.GetArchiveTOC();
  xiiStringBuilder     sArchivePath = m_sArchiveSubFolder;
  sArchivePath.AppendPath(sFileOrFolder);
  // We might be called with paths like AAA/../BBB which we won't find in the toc unless we clean the path first.
  sArchivePath.MakeCleanPath();
  const xiiUInt32 uiEntryIndex = toc.FindEntry(sArchivePath);

  if (uiEntryIndex == xiiInvalidIndex)
    return XII_FAILURE;

  const xiiArchiveEntry* pEntry = &toc.m_Entries[uiEntryIndex];

  const xiiStringView sPath = toc.GetEntryPathString(uiEntryIndex);

  out_Stats.m_bIsDirectory         = false;
  out_Stats.m_LastModificationTime = m_LastModificationTime;
  out_Stats.m_uiFileSize           = pEntry->m_uiUncompressedDataSize;
  out_Stats.m_sParentPath          = sPath;
  out_Stats.m_sParentPath.PathParentDirectory();
  out_Stats.m_sName = xiiPathUtils::GetFileNameAndExtension(sPath);

  return XII_SUCCESS;
}

xiiResult xiiDataDirectory::ArchiveType::InternalInitializeDataDirectory(xiiStringView sDirectory)
{
  xiiStringBuilder sRedirected;
  XII_SUCCEED_OR_RETURN(xiiFileSystem::ResolveSpecialDirectory(sDirectory, sRedirected));

  sRedirected.MakeCleanPath();
  // remove trailing slashes
  sRedirected.Trim("", "/");
  m_sRedirectedDataDirPath = sRedirected;

  bool             bSupported = false;
  xiiStringBuilder sArchivePath;

  xiiHybridArray<xiiString, 4, xiiStaticAllocatorWrapper> extensions = xiiArchiveUtils::GetAcceptedArchiveFileExtensions();

#ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT
  extensions.PushBack("zip");
  extensions.PushBack("apk");
#endif

  for (const auto& ext : extensions)
  {
    const xiiUInt32 uiLength = ext.GetElementCount();
    if (sRedirected.HasExtension(ext))
    {
      sArchivePath        = sRedirected;
      m_sArchiveSubFolder = "";
      bSupported          = true;
      goto EndLoop;
    }
    const char* szFound = nullptr;
    do
    {
      szFound = sRedirected.FindLastSubString_NoCase(ext, szFound);
      if (szFound != nullptr && szFound[uiLength] == '/')
      {
        sArchivePath        = xiiStringView(sRedirected.GetData(), szFound + uiLength);
        m_sArchiveSubFolder = szFound + uiLength + 1;
        bSupported          = true;
        goto EndLoop;
      }

    } while (szFound != nullptr);
  }
EndLoop:
  if (!bSupported)
    return XII_FAILURE;

#if XII_ENABLED(XII_SUPPORTS_FILE_STATS)
  xiiFileStats stats;
  if (xiiOSFile::GetFileStats(sArchivePath, stats).Failed())
    return XII_FAILURE;

  m_LastModificationTime = stats.m_LastModificationTime;
#endif

  XII_LOG_BLOCK("xiiArchiveDataDir", sDirectory);

  XII_SUCCEED_OR_RETURN(m_ArchiveReader.OpenArchive(sArchivePath));

  ReloadExternalConfigs();

  return XII_SUCCESS;
}

void xiiDataDirectory::ArchiveType::OnReaderWriterClose(xiiDataDirectoryReaderWriterBase* pClosed)
{
  XII_LOCK(m_ReaderMutex);

  if (pClosed->GetDataDirUserData() == 0)
  {
    m_FreeReadersUncompressed.PushBack(static_cast<ArchiveReaderUncompressed*>(pClosed));
    return;
  }

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  if (pClosed->GetDataDirUserData() == 1)
  {
    m_FreeReadersZstd.PushBack(static_cast<ArchiveReaderZstd*>(pClosed));
    return;
  }
#endif

#ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT
  if (pClosed->GetDataDirUserData() == 2)
  {
    m_FreeReadersZip.PushBack(static_cast<ArchiveReaderZip*>(pClosed));
    return;
  }
#endif

  XII_ASSERT_NOT_IMPLEMENTED;
}

//////////////////////////////////////////////////////////////////////////

xiiDataDirectory::ArchiveReaderCommon::ArchiveReaderCommon(xiiInt32 iDataDirUserData) :
  xiiDataDirectoryReader(iDataDirUserData)
{
}

xiiUInt64 xiiDataDirectory::ArchiveReaderCommon::GetFileSize() const
{
  return m_uiUncompressedSize;
}

//////////////////////////////////////////////////////////////////////////

xiiDataDirectory::ArchiveReaderUncompressed::ArchiveReaderUncompressed(xiiInt32 iDataDirUserData) :
  ArchiveReaderCommon(iDataDirUserData)
{
}

xiiUInt64 xiiDataDirectory::ArchiveReaderUncompressed::Skip(xiiUInt64 uiBytes)
{
  return m_MemStreamReader.SkipBytes(uiBytes);
}

xiiUInt64 xiiDataDirectory::ArchiveReaderUncompressed::Read(void* pBuffer, xiiUInt64 uiBytes)
{
  return m_MemStreamReader.ReadBytes(pBuffer, uiBytes);
}

xiiResult xiiDataDirectory::ArchiveReaderUncompressed::InternalOpen(xiiFileShareMode::Enum fileShareMode)
{
  XII_IGNORE_UNUSED(fileShareMode);

  XII_ASSERT_DEBUG(fileShareMode != xiiFileShareMode::Exclusive, "Archives only support shared reading of files. Exclusive access cannot be guaranteed.");

  // nothing to do
  return XII_SUCCESS;
}

void xiiDataDirectory::ArchiveReaderUncompressed::InternalClose()
{
  // nothing to do
}

//////////////////////////////////////////////////////////////////////////

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT

xiiDataDirectory::ArchiveReaderZstd::ArchiveReaderZstd(xiiInt32 iDataDirUserData) :
  ArchiveReaderCommon(iDataDirUserData)
{
}

xiiUInt64 xiiDataDirectory::ArchiveReaderZstd::Read(void* pBuffer, xiiUInt64 uiBytes)
{
  return m_CompressedStreamReader.ReadBytes(pBuffer, uiBytes);
}

xiiResult xiiDataDirectory::ArchiveReaderZstd::InternalOpen(xiiFileShareMode::Enum fileShareMode)
{
  XII_IGNORE_UNUSED(fileShareMode);

  XII_ASSERT_DEBUG(fileShareMode != xiiFileShareMode::Exclusive, "Archives only support shared reading of files. Exclusive access cannot be guaranteed.");

  m_CompressedStreamReader.SetInputStream(&m_MemStreamReader);
  return XII_SUCCESS;
}

void xiiDataDirectory::ArchiveReaderZstd::InternalClose()
{
  // nothing to do
}
#endif

//////////////////////////////////////////////////////////////////////////

#ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT

xiiDataDirectory::ArchiveReaderZip::ArchiveReaderZip(xiiInt32 iDataDirUserData) :
  ArchiveReaderUncompressed(iDataDirUserData)
{
}

xiiDataDirectory::ArchiveReaderZip::~ArchiveReaderZip() = default;

xiiUInt64 xiiDataDirectory::ArchiveReaderZip::Read(void* pBuffer, xiiUInt64 uiBytes)
{
  return m_CompressedStreamReader.ReadBytes(pBuffer, uiBytes);
}

xiiResult xiiDataDirectory::ArchiveReaderZip::InternalOpen(xiiFileShareMode::Enum fileShareMode)
{
  XII_IGNORE_UNUSED(fileShareMode);

  XII_ASSERT_DEBUG(fileShareMode != xiiFileShareMode::Exclusive, "Archives only support shared reading of files. Exclusive access cannot be guaranteed.");

  m_CompressedStreamReader.SetInputStream(&m_MemStreamReader, m_uiCompressedSize);
  return XII_SUCCESS;
}

#endif

XII_STATICLINK_FILE(Foundation, Foundation_IO_Archive_Implementation_DataDirTypeArchive);
