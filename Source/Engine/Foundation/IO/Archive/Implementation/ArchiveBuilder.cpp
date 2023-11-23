#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Archive/ArchiveBuilder.h>
#include <Foundation/IO/Archive/ArchiveUtils.h>
#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Time/Stopwatch.h>

void xiiArchiveBuilder::AddFolder(xiiStringView sAbsFolderPath, xiiArchiveCompressionMode defaultMode /*= xiiArchiveCompressionMode::Uncompressed*/, InclusionCallback callback /*= InclusionCallback()*/)
{
#if XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS)
  xiiFileSystemIterator fileIt;

  xiiStringBuilder sBasePath = sAbsFolderPath;
  sBasePath.MakeCleanPath();

  xiiStringBuilder fullPath;
  xiiStringBuilder relPath;

  for (fileIt.StartSearch(sBasePath, xiiFileSystemIteratorFlags::ReportFilesRecursive); fileIt.IsValid(); fileIt.Next())
  {
    const auto& stat = fileIt.GetStats();

    stat.GetFullPath(fullPath);
    relPath = fullPath;

    if (relPath.MakeRelativeTo(sBasePath).Succeeded())
    {
      xiiArchiveCompressionMode compression       = defaultMode;
      xiiInt32                  iCompressionLevel = 0;

      if (callback.IsValid())
      {
        switch (callback(fullPath))
        {
          case InclusionMode::Exclude:
            continue;

          case InclusionMode::Uncompressed:
            compression = xiiArchiveCompressionMode::Uncompressed;
            break;

#  ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
          case InclusionMode::Compress_zstd_fastest:
            compression       = xiiArchiveCompressionMode::Compressed_zstd;
            iCompressionLevel = static_cast<xiiInt32>(xiiCompressedStreamWriterZstd::Compression::Fastest);
            break;
          case InclusionMode::Compress_zstd_fast:
            compression       = xiiArchiveCompressionMode::Compressed_zstd;
            iCompressionLevel = static_cast<xiiInt32>(xiiCompressedStreamWriterZstd::Compression::Fast);
            break;
          case InclusionMode::Compress_zstd_average:
            compression       = xiiArchiveCompressionMode::Compressed_zstd;
            iCompressionLevel = static_cast<xiiInt32>(xiiCompressedStreamWriterZstd::Compression::Average);
            break;
          case InclusionMode::Compress_zstd_high:
            compression       = xiiArchiveCompressionMode::Compressed_zstd;
            iCompressionLevel = static_cast<xiiInt32>(xiiCompressedStreamWriterZstd::Compression::High);
            break;
          case InclusionMode::Compress_zstd_highest:
            compression       = xiiArchiveCompressionMode::Compressed_zstd;
            iCompressionLevel = static_cast<xiiInt32>(xiiCompressedStreamWriterZstd::Compression::Highest);
            break;
#  endif
        }
      }

      auto& e               = m_Entries.ExpandAndGetRef();
      e.m_sAbsSourcePath    = fullPath;
      e.m_sRelTargetPath    = relPath;
      e.m_CompressionMode   = compression;
      e.m_iCompressionLevel = iCompressionLevel;
    }
  }

#else
  XII_ASSERT_NOT_IMPLEMENTED;
#endif
}

xiiResult xiiArchiveBuilder::WriteArchive(xiiStringView sFile) const
{
  XII_LOG_BLOCK("WriteArchive", sFile);

  xiiFileWriter file;
  if (file.Open(sFile, 1024 * 1024 * 16).Failed())
  {
    xiiLog::Error("Could not open file for writing archive to: '{}'", sFile);
    return XII_FAILURE;
  }

  return WriteArchive(file);
}

xiiResult xiiArchiveBuilder::WriteArchive(xiiStreamWriter& ref_stream) const
{
  XII_SUCCEED_OR_RETURN(xiiArchiveUtils::WriteHeader(ref_stream));

  xiiArchiveTOC toc;

  xiiStringBuilder sHashablePath;

  xiiUInt64       uiStreamSize = 0;
  const xiiUInt32 uiNumEntries = m_Entries.GetCount();

  xiiStopwatch sw;

  for (xiiUInt32 i = 0; i < uiNumEntries; ++i)
  {
    const SourceEntry& e = m_Entries[i];

    const xiiUInt32 uiPathStringOffset = toc.m_AllPathStrings.GetCount();
    toc.m_AllPathStrings.PushBackRange(xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(e.m_sRelTargetPath.GetData()), e.m_sRelTargetPath.GetElementCount() + 1));

    sHashablePath = e.m_sRelTargetPath;
    sHashablePath.ToLower();

    toc.m_PathToEntryIndex[xiiArchiveStoredString(xiiHashingUtils::StringHash(sHashablePath), uiPathStringOffset)] = toc.m_Entries.GetCount();

    if (!WriteNextFileCallback(i + 1, uiNumEntries, e.m_sAbsSourcePath))
      return XII_FAILURE;

    xiiArchiveEntry& tocEntry = toc.m_Entries.ExpandAndGetRef();

    XII_SUCCEED_OR_RETURN(xiiArchiveUtils::WriteEntryOptimal(ref_stream, e.m_sAbsSourcePath, uiPathStringOffset, e.m_CompressionMode, e.m_iCompressionLevel, tocEntry, uiStreamSize, xiiMakeDelegate(&xiiArchiveBuilder::WriteFileProgressCallback, this)));

    WriteFileResultCallback(i + 1, uiNumEntries, e.m_sAbsSourcePath, tocEntry.m_uiUncompressedDataSize, tocEntry.m_uiStoredDataSize, sw.Checkpoint());
  }

  XII_SUCCEED_OR_RETURN(xiiArchiveUtils::AppendTOC(ref_stream, toc));

  return XII_SUCCESS;
}

bool xiiArchiveBuilder::WriteNextFileCallback(xiiUInt32 uiCurEntry, xiiUInt32 uiMaxEntries, xiiStringView sSourceFile) const
{
  return true;
}

bool xiiArchiveBuilder::WriteFileProgressCallback(xiiUInt64 bytesWritten, xiiUInt64 bytesTotal) const
{
  return true;
}

XII_STATICLINK_FILE(Foundation, Foundation_IO_Archive_Implementation_ArchiveBuilder);
