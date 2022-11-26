#pragma once

#include <Foundation/IO/Archive/Archive.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/Types/Delegate.h>

/// \brief Utility class to build an xiiArchive file from files/folders on disk
///
/// All functionality for writing an xiiArchive file is available through xiiArchiveUtils.
class XII_FOUNDATION_DLL xiiArchiveBuilder
{
public:
  struct SourceEntry
  {
    xiiString                 m_sAbsSourcePath; ///< The source file to read
    xiiString                 m_sRelTargetPath; ///< Under which relative path to store it in the xiiArchive
    xiiArchiveCompressionMode m_CompressionMode = xiiArchiveCompressionMode::Uncompressed;
  };

  // all the source files from disk that should be put into the xiiArchive
  xiiDeque<SourceEntry> m_Entries;

  enum class InclusionMode
  {
    Exclude,       ///< Do not add this file to the archive
    Uncompressed,  ///< Add the file to the archive, but do not even try to compress it
    Compress_zstd, ///< Add the file and try out compression. If compression does not help, the file will end up uncompressed in the
                   ///< archive.
  };

  /// \brief Custom decider whether to include a file into the archive
  typedef xiiDelegate<InclusionMode(const char*)> InclusionCallback;

  /// \brief Iterates over all files in a folder and adds them to m_Entries for later.
  ///
  /// The callback can be used to exclude certain files or to deactivate compression on them.
  /// \note If no callback is given, the default is to store all files uncompressed!
  void AddFolder(const char* szAbsFolderPath, xiiArchiveCompressionMode defaultMode = xiiArchiveCompressionMode::Uncompressed, InclusionCallback callback = InclusionCallback());

  /// \brief Overwrites the given file with the archive
  xiiResult WriteArchive(const char* szFile) const;

  /// \brief Writes the previously gathered files to the file stream
  xiiResult WriteArchive(xiiStreamWriter& stream) const;

protected:
  /// Override this to get a callback when the next file is being written to the output
  virtual bool WriteNextFileCallback(xiiUInt32 uiCurEntry, xiiUInt32 uiMaxEntries, const char* szSourceFile) const;
  /// Override this to get a progress report for writing a single file to the output
  virtual bool WriteFileProgressCallback(xiiUInt64 bytesWritten, xiiUInt64 bytesTotal) const;
};
