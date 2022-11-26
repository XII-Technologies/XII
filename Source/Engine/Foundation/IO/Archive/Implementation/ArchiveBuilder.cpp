#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Archive/ArchiveBuilder.h>
#include <Foundation/IO/Archive/ArchiveUtils.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>

void xiiArchiveBuilder::AddFolder(const char* szAbsFolderPath, xiiArchiveCompressionMode defaultMode /*= xiiArchiveCompressionMode::Uncompressed*/, InclusionCallback callback /*= InclusionCallback()*/)
{
#if XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS)
  xiiFileSystemIterator fileIt;

  xiiStringBuilder sBasePath = szAbsFolderPath;
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
      xiiArchiveCompressionMode compression = defaultMode;

      if (callback.IsValid())
      {
        switch (callback(fullPath))
        {
          case InclusionMode::Exclude:
            continue;

          case InclusionMode::Uncompressed:
            compression = xiiArchiveCompressionMode::Uncompressed;
            break;

          case InclusionMode::Compress_zstd:
            compression = xiiArchiveCompressionMode::Compressed_zstd;
            break;
        }
      }

      auto& e             = m_Entries.ExpandAndGetRef();
      e.m_sAbsSourcePath  = fullPath;
      e.m_sRelTargetPath  = relPath;
      e.m_CompressionMode = compression;
    }
  }

#else
  XII_ASSERT_NOT_IMPLEMENTED;
#endif
}

xiiResult xiiArchiveBuilder::WriteArchive(const char* szFile) const
{
  XII_LOG_BLOCK("WriteArchive", szFile);

  xiiFileWriter file;
  if (file.Open(szFile, 1024 * 1024 * 16).Failed())
  {
    xiiLog::Error("Could not open file for writing archive to: '{}'", szFile);
    return XII_FAILURE;
  }

  return WriteArchive(file);
}

xiiResult xiiArchiveBuilder::WriteArchive(xiiStreamWriter& stream) const
{
  XII_SUCCEED_OR_RETURN(xiiArchiveUtils::WriteHeader(stream));

  xiiArchiveTOC toc;

  xiiStringBuilder sHashablePath;

  xiiUInt64       uiStreamSize = 0;
  const xiiUInt32 uiNumEntries = m_Entries.GetCount();

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

    XII_SUCCEED_OR_RETURN(xiiArchiveUtils::WriteEntryOptimal(stream, e.m_sAbsSourcePath, uiPathStringOffset, e.m_CompressionMode, toc.m_Entries.ExpandAndGetRef(), uiStreamSize, xiiMakeDelegate(&xiiArchiveBuilder::WriteFileProgressCallback, this)));
  }

  XII_SUCCEED_OR_RETURN(xiiArchiveUtils::AppendTOC(stream, toc));

  return XII_SUCCESS;
}

bool xiiArchiveBuilder::WriteNextFileCallback(xiiUInt32 uiCurEntry, xiiUInt32 uiMaxEntries, const char* szSourceFile) const
{
  return true;
}

bool xiiArchiveBuilder::WriteFileProgressCallback(xiiUInt64 bytesWritten, xiiUInt64 bytesTotal) const
{
  return true;
}

XII_STATICLINK_FILE(Foundation, Foundation_IO_Archive_Implementation_ArchiveBuilder);
