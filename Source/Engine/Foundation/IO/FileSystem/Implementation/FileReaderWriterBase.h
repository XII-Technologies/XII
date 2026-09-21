/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/IO/Stream.h>

/// The base class for all file readers.
/// Provides access to xiiFileSystem::GetFileReader, which is necessary to get access to the streams that
/// xiiDataDirectoryType's provide.
/// Derive from this class if you want to implement different policies on how to read files.
/// E.g. the default reader (xiiFileReader) implements a buffered read policy (using an internal cache).
class XII_FOUNDATION_DLL xiiFileReaderBase : public xiiStreamReader
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiFileReaderBase);

public:
  xiiFileReaderBase() { m_pDataDirReader = nullptr; }

  /// Returns the absolute path with which the file was opened (including the prefix of the data directory).
  xiiString128 GetFilePathAbsolute() const
  {
    xiiStringBuilder sAbs = m_pDataDirReader->GetDataDirectory()->GetRedirectedDataDirectoryPath();
    sAbs.AppendPath(m_pDataDirReader->GetFilePath().GetView());
    return sAbs;
  }

  /// Returns the relative path of the file within its data directory (excluding the prefix of the data directory).
  xiiString128 GetFilePathRelative() const { return m_pDataDirReader->GetFilePath(); }

  /// Returns the xiiDataDirectoryType over which this file has been opened.
  xiiDataDirectoryType* GetDataDirectory() const { return m_pDataDirReader->GetDataDirectory(); }

  /// Returns true, if the file is currently open.
  bool IsOpen() const { return m_pDataDirReader != nullptr; }

  /// Returns the current total size of the file.
  xiiUInt64 GetFileSize() const { return m_pDataDirReader->GetFileSize(); }

protected:
  xiiDataDirectoryReader* GetFileReader(xiiStringView sFile, xiiFileShareMode::Enum FileShareMode, bool bAllowFileEvents)
  {
    return xiiFileSystem::GetFileReader(sFile, FileShareMode, bAllowFileEvents);
  }

  xiiDataDirectoryReader* m_pDataDirReader;
};


/// The base class for all file writers.
/// Provides access to xiiFileSystem::GetFileWriter, which is necessary to get access to the streams that
/// xiiDataDirectoryType's provide.
/// Derive from this class if you want to implement different policies on how to write files.
/// E.g. the default writer (xiiFileWriter) implements a buffered write policy (using an internal cache).
class XII_FOUNDATION_DLL xiiFileWriterBase : public xiiStreamWriter
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiFileWriterBase);

public:
  xiiFileWriterBase() { m_pDataDirWriter = nullptr; }

  /// Returns the absolute path with which the file was opened (including the prefix of the data directory).
  xiiString128 GetFilePathAbsolute() const
  {
    xiiStringBuilder sAbs = m_pDataDirWriter->GetDataDirectory()->GetRedirectedDataDirectoryPath();
    sAbs.AppendPath(m_pDataDirWriter->GetFilePath().GetView());
    return sAbs;
  }

  /// Returns the relative path of the file within its data directory (excluding the prefix of the data directory).
  xiiString128 GetFilePathRelative() const { return m_pDataDirWriter->GetFilePath(); }

  /// Returns the xiiDataDirectoryType over which this file has been opened.
  xiiDataDirectoryType* GetDataDirectory() const { return m_pDataDirWriter->GetDataDirectory(); }

  /// Returns true, if the file is currently open.
  bool IsOpen() const { return m_pDataDirWriter != nullptr; }

  /// Returns the current total size of the file.
  xiiUInt64 GetFileSize() const { return m_pDataDirWriter->GetFileSize(); } // [tested]

protected:
  xiiDataDirectoryWriter* GetFileWriter(xiiStringView sFile, xiiFileShareMode::Enum FileShareMode, bool bAllowFileEvents)
  {
    return xiiFileSystem::GetFileWriter(sFile, FileShareMode, bAllowFileEvents);
  }

  xiiDataDirectoryWriter* m_pDataDirWriter;
};
