/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/IO/Archive/Archive.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/Types/Delegate.h>

/// Utility class to build a xiiArchive file from files/folders on disk
///
/// All functionality for writing a xiiArchive file is available through xiiArchiveUtils.
class XII_FOUNDATION_DLL xiiArchiveBuilder
{
public:
  struct SourceEntry
  {
    xiiString                 m_sAbsSourcePath; ///< The source file to read
    xiiString                 m_sRelTargetPath; ///< Under which relative path to store it in the xiiArchive
    xiiArchiveCompressionMode m_CompressionMode   = xiiArchiveCompressionMode::Uncompressed;
    xiiInt32                  m_iCompressionLevel = 0;
  };

  // all the source files from disk that should be put into the xiiArchive
  xiiDeque<SourceEntry> m_Entries;

  enum class InclusionMode
  {
    Exclude,               ///< Do not add this file to the archive
    Uncompressed,          ///< Add the file to the archive, but do not even try to compress it
    Compress_zstd_fastest, ///< Add the file and try out compression. If compression does not help, the file will end up uncompressed in the archive.
    Compress_zstd_fast,    ///< Add the file and try out compression. If compression does not help, the file will end up uncompressed in the archive.
    Compress_zstd_average, ///< Add the file and try out compression. If compression does not help, the file will end up uncompressed in the archive.
    Compress_zstd_high,    ///< Add the file and try out compression. If compression does not help, the file will end up uncompressed in the archive.
    Compress_zstd_highest, ///< Add the file and try out compression. If compression does not help, the file will end up uncompressed in the archive.
  };

  /// Custom decider whether to include a file into the archive
  using InclusionCallback = xiiDelegate<InclusionMode(xiiStringView)>;

  /// Iterates over all files in a folder and adds them to m_Entries for later.
  ///
  /// The callback can be used to exclude certain files or to deactivate compression on them.
  /// \note If no callback is given, the default is to store all files uncompressed!
  void AddFolder(xiiStringView sAbsFolderPath, xiiArchiveCompressionMode defaultMode = xiiArchiveCompressionMode::Uncompressed, InclusionCallback callback = InclusionCallback());

  /// Overwrites the given file with the archive
  xiiResult WriteArchive(xiiStringView sFile) const;

  /// Writes the previously gathered files to the file stream
  xiiResult WriteArchive(xiiStreamWriter& ref_stream) const;

protected:
  /// Override this to get a callback when the next file is being written to the output. Return 'true' to continue, 'false' to cancel the entire archive generation.
  virtual bool WriteNextFileCallback(xiiUInt32 uiCurEntry, xiiUInt32 uiMaxEntries, xiiStringView sSourceFile) const;
  /// Override this to get a progress report for writing a single file to the output
  virtual bool WriteFileProgressCallback(xiiUInt64 bytesWritten, xiiUInt64 bytesTotal) const;
  /// Override this to get a callback after a file has been processed. Gets additional information about the compression result and duration.
  virtual void WriteFileResultCallback(xiiUInt32 uiCurEntry, xiiUInt32 uiMaxEntries, xiiStringView sSourceFile, xiiUInt64 uiSourceSize, xiiUInt64 uiStoredSize, xiiTime duration) const
  {
    XII_IGNORE_UNUSED(uiCurEntry);
    XII_IGNORE_UNUSED(uiMaxEntries);
    XII_IGNORE_UNUSED(sSourceFile);
    XII_IGNORE_UNUSED(uiSourceSize);
    XII_IGNORE_UNUSED(uiStoredSize);
    XII_IGNORE_UNUSED(duration);
  }
};
