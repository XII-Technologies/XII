/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/FileEnums.h>
#include <Foundation/Strings/String.h>

class xiiDataDirectoryReaderWriterBase;
class xiiDataDirectoryReader;
class xiiDataDirectoryWriter;
struct xiiFileStats;
class xiiDataDirectoryType;

/// Describes in which mode a data directory is mounted.
enum class xiiDataDirUsage
{
  ReadOnly,
  AllowWrites,
};

struct xiiDataDirectoryInfo
{
  xiiDataDirUsage m_Usage;

  xiiString             m_sRootName;
  xiiString             m_sGroup;
  xiiDataDirectoryType* m_pDataDirType = nullptr;
};

/// The base class for all data directory types.
///
/// There are different data directory types, such as a simple folder, a ZIP file or some kind of library
/// (e.g. image files from procedural data). Even a HTTP server that actually transmits files over a network
/// can provided by implementing it as a data directory type.
/// Data directories are added through xiiFileSystem, which uses factories to decide which xiiDataDirectoryType
/// to use for handling which data directory.
class XII_FOUNDATION_DLL xiiDataDirectoryType
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiDataDirectoryType);

public:
  xiiDataDirectoryType()          = default;
  virtual ~xiiDataDirectoryType() = default;

  /// Returns the absolute path to the data directory.
  const xiiString128& GetDataDirectoryPath() const { return m_sDataDirectoryPath; }

  /// By default this is the same as GetDataDirectoryPath(), but derived implementations may use a different location where they
  /// actually get the files from.
  virtual const xiiString128& GetRedirectedDataDirectoryPath() const { return GetDataDirectoryPath(); }

  /// Some data directory types may use external configuration files (e.g. asset lookup tables)
  ///        that may get updated, while the directory is mounted. This function allows each directory type to implement
  ///        reloading and reapplying of configurations, without dismounting and remounting the data directory.
  virtual void ReloadExternalConfigs() {};

protected:
  friend class xiiFileSystem;

  /// Tries to setup the data directory. Can fail, if the type is incorrect (e.g. a ZIP file data directory type cannot handle a
  /// simple folder and vice versa)
  xiiResult InitializeDataDirectory(xiiStringView sDataDirPath);

  /// Must be implemented to create a xiiDataDirectoryReader for accessing the given file. Returns nullptr if the file could not be
  /// opened.
  ///
  /// \param szFile is given as a path relative to the data directory's path.
  /// So unless the data directory path is empty, this will never be an absolute path.
  /// If a rooted path was given, the root name is also removed and only the relative part is passed along.
  /// \param bSpecificallyThisDataDir This is true when the original path specified to open the file through exactly this data directory,
  /// by using a rooted path.
  /// If an absolute path is used, which incidentally matches the prefix of this data directory, bSpecificallyThisDataDir is NOT set to
  /// true, as there might be other data directories that also match.
  virtual xiiDataDirectoryReader* OpenFileToRead(xiiStringView sFile, xiiFileShareMode::Enum FileShareMode, bool bSpecificallyThisDataDir) = 0;

  /// Must be implemented to create a xiiDataDirectoryWriter for accessing the given file. Returns nullptr if the file could not be
  /// opened.
  ///
  /// If it always returns nullptr (default) the data directory is read-only (at least through this type).
  virtual xiiDataDirectoryWriter* OpenFileToWrite(xiiStringView sFile, xiiFileShareMode::Enum fileShareMode)
  {
    XII_IGNORE_UNUSED(sFile);
    XII_IGNORE_UNUSED(fileShareMode);
    return nullptr;
  }

  /// This function is called by the filesystem when a data directory is removed.
  ///
  /// It should delete itself using the proper allocator.
  virtual void RemoveDataDirectory() = 0;

  /// If a Data Directory Type supports it, this function will remove the given file from it.
  virtual void DeleteFile(xiiStringView sFile) { XII_IGNORE_UNUSED(sFile); }

  /// This function checks whether the given file exists in this data directory.
  ///
  /// The default implementation simply calls xiiOSFile::ExistsFile
  /// An optimized implementation might look this information up in some hash-map.
  virtual bool ExistsFile(xiiStringView sFile, bool bOneSpecificDataDir);

  /// Upon success returns the xiiFileStats for a file in this data directory.
  virtual xiiResult GetFileStats(xiiStringView sFileOrFolder, bool bOneSpecificDataDir, xiiFileStats& out_Stats) = 0;

  /// If this data directory knows how to redirect the given path, it should do so and return true.
  /// Called by xiiFileSystem::ResolveAssetRedirection
  virtual bool ResolveAssetRedirection(xiiStringView sPathOrAssetGuid, xiiStringBuilder& out_sRedirection)
  {
    XII_IGNORE_UNUSED(sPathOrAssetGuid);
    XII_IGNORE_UNUSED(out_sRedirection);
    return false;
  }

protected:
  friend class xiiDataDirectoryReaderWriterBase;

  /// This is automatically called whenever a xiiDataDirectoryReaderWriterBase that was opened by this type is being closed.
  ///
  /// It allows the xiiDataDirectoryType to return the reader/writer to a pool of reusable objects, or to destroy it
  /// using the proper allocator.
  virtual void OnReaderWriterClose(xiiDataDirectoryReaderWriterBase* pClosed) { XII_IGNORE_UNUSED(pClosed); }

  /// This function should only be used by a Factory (which should be a static function in the respective xiiDataDirectoryType).
  ///
  /// It is used to initialize the data directory. If this xiiDataDirectoryType cannot handle the given type,
  /// it must return XII_FAILURE and the Factory needs to clean it up properly.
  virtual xiiResult InternalInitializeDataDirectory(xiiStringView sDirectory) = 0;

  /// Derived classes can use 'GetDataDirectoryPath' to access this data.
  xiiString128 m_sDataDirectoryPath;
};



/// This is the base class for all data directory readers/writers.
///
/// Different data directory types (ZIP file, simple folder, etc.) use different reader/writer types.
class XII_FOUNDATION_DLL xiiDataDirectoryReaderWriterBase
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiDataDirectoryReaderWriterBase);

public:
  /// The derived class should pass along whether it is a reader or writer.
  xiiDataDirectoryReaderWriterBase(xiiInt32 iDataDirUserData, bool bIsReader);

  virtual ~xiiDataDirectoryReaderWriterBase() = default;

  /// Used by xiiDataDirectoryType's to try to open the given file. They need to pass along their own pointer.
  xiiResult Open(xiiStringView sFile, xiiDataDirectoryType* pOwnerDataDirectory, xiiFileShareMode::Enum fileShareMode);

  /// Closes this data stream.
  void Close();

  /// Returns the relative path of this file within the owner data directory.
  const xiiString128& GetFilePath() const;

  /// Returns the pointer to the data directory, which created this reader/writer.
  xiiDataDirectoryType* GetDataDirectory() const;

  /// Returns true if this is a reader stream, false if it is a writer stream.
  bool IsReader() const { return m_bIsReader; }

  /// Returns the current total size of the file.
  virtual xiiUInt64 GetFileSize() const = 0;

  xiiInt32 GetDataDirUserData() const { return m_iDataDirUserData; }

protected:
  /// This function must be implemented by the derived class.
  virtual xiiResult InternalOpen(xiiFileShareMode::Enum FileShareMode) = 0;

  /// This function must be implemented by the derived class.
  virtual void InternalClose() = 0;

  bool                  m_bIsReader;
  xiiInt32              m_iDataDirUserData = 0;
  xiiDataDirectoryType* m_pDataDirType;
  xiiString128          m_sFilePath;
};

/// A base class for readers that handle reading from a (virtual) file inside a data directory.
///
/// Different data directory types (ZIP file, simple folder, etc.) use different reader/writer types.
class XII_FOUNDATION_DLL xiiDataDirectoryReader : public xiiDataDirectoryReaderWriterBase
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiDataDirectoryReader);

public:
  xiiDataDirectoryReader(xiiInt32 iDataDirUserData) :
    xiiDataDirectoryReaderWriterBase(iDataDirUserData, true)
  {
  }

  virtual xiiUInt64 Read(void* pBuffer, xiiUInt64 uiBytes) = 0;

  /// Helper method to skip a number of bytes (implementations of the directory reader may implement this more efficiently for example)
  virtual xiiUInt64 Skip(xiiUInt64 uiBytes)
  {
    xiiUInt8 uiTempBuffer[1024];

    xiiUInt64 uiBytesSkipped = 0;

    while (uiBytesSkipped < uiBytes)
    {
      xiiUInt64 uiBytesToRead = xiiMath::Min<xiiUInt64>(uiBytes - uiBytesSkipped, 1024);

      xiiUInt64 uiBytesRead = Read(uiTempBuffer, uiBytesToRead);

      uiBytesSkipped += uiBytesRead;

      // Terminate early if the stream didn't read as many bytes as we requested (EOF for example)
      if (uiBytesRead < uiBytesToRead)
        break;
    }

    return uiBytesSkipped;
  }
};

/// A base class for writers that handle writing to a (virtual) file inside a data directory.
///
/// Different data directory types (ZIP file, simple folder, etc.) use different reader/writer types.
class XII_FOUNDATION_DLL xiiDataDirectoryWriter : public xiiDataDirectoryReaderWriterBase
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiDataDirectoryWriter);

public:
  xiiDataDirectoryWriter(xiiInt32 iDataDirUserData) :
    xiiDataDirectoryReaderWriterBase(iDataDirUserData, false)
  {
  }

  virtual xiiResult Write(const void* pBuffer, xiiUInt64 uiBytes) = 0;
};


#include <Foundation/IO/FileSystem/Implementation/DataDirType_inl.h>
