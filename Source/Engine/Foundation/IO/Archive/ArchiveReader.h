/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/IO/Archive/Archive.h>
#include <Foundation/IO/MemoryMappedFile.h>
#include <Foundation/Types/UniquePtr.h>

class xiiRawMemoryStreamReader;
class xiiStreamReader;

/// A utility class for reading from xiiArchive files
class XII_FOUNDATION_DLL xiiArchiveReader
{
public:
  /// Opens the given file and validates that it is a valid archive file.
  xiiResult OpenArchive(xiiStringView sPath);

  /// Returns the table-of-contents for the previously opened archive.
  const xiiArchiveTOC& GetArchiveTOC();

  /// Extracts the given entry to the target folder.
  ///
  /// Calls ExtractFileProgressCallback() to report progress.
  xiiResult ExtractFile(xiiUInt32 uiEntryIdx, xiiStringView sTargetFolder) const;

  /// Extracts all files to the target folder.
  ///
  /// Calls ExtractNextFileCallback() for every file that is being extracted.
  xiiResult ExtractAllFiles(xiiStringView sTargetFolder) const;

  /// Sets up \a memReader for reading the raw (potentially compressed) data that is stored for the given entry in the archive.
  void ConfigureRawMemoryStreamReader(xiiUInt32 uiEntryIdx, xiiRawMemoryStreamReader& ref_memReader) const;

  /// Creates a reader that will decompress the given file entry.
  xiiUniquePtr<xiiStreamReader> CreateEntryReader(xiiUInt32 uiEntryIdx) const;

protected:
  /// Called by ExtractAllFiles() for progress reporting. Return false to abort.
  virtual bool ExtractNextFileCallback(xiiUInt32 uiCurEntry, xiiUInt32 uiMaxEntries, xiiStringView sSourceFile) const;

  /// Called by ExtractFile() for progress reporting. Return false to abort.
  virtual bool ExtractFileProgressCallback(xiiUInt64 uiBytesWritten, xiiUInt64 uiTotalBytes) const;

  xiiMemoryMappedFile m_MemFile;
  xiiArchiveTOC       m_ArchiveTOC;
  xiiUInt8            m_uiArchiveVersion = 0;
  const void*         m_pDataStart       = nullptr;
  xiiUInt64           m_uiMemFileSize    = 0;
};
