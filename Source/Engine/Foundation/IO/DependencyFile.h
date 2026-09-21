/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Time/Timestamp.h>

/// This class represents a set of files of which one wants to know when any one of them changes.
///
/// xiiDependencyFile stores a list of files that are the 'dependency set'. It can be serialized.
/// Through HasAnyFileChanged() one can detect whether any of the files has changed, since the last call to StoreCurrentTimeStamp().
/// The time stamp that is retrieved through StoreCurrentTimeStamp() will also be serialized.
class XII_FOUNDATION_DLL xiiDependencyFile
{
public:
  xiiDependencyFile();

  /// Clears all files that were added with AddFileDependency()
  void Clear();

  /// Adds one file as a dependency to the list
  void AddFileDependency(xiiStringView sFile);

  /// Allows read access to all currently stored file dependencies
  const xiiHybridArray<xiiString, 16>& GetFileDependencies() const { return m_AssetTransformDependencies; }

  /// Writes the current state to a stream. Note that you probably should call StoreCurrentTimeStamp() before this, to serialize the latest
  /// file stamp
  xiiResult WriteDependencyFile(xiiStreamWriter& ref_stream) const;

  /// Reads the state from a stream. Call HasAnyFileChanged() afterwards to determine whether anything has changed since when the data was
  /// serialized.
  xiiResult ReadDependencyFile(xiiStreamReader& ref_stream);

  /// Writes the current state to a file. Note that you probably should call StoreCurrentTimeStamp() before this, to serialize the latest file
  /// stamp
  xiiResult WriteDependencyFile(xiiStringView sFile) const;

  /// Reads the state from a file. Call HasAnyFileChanged() afterwards to determine whether anything has changed since when the data was
  /// serialized.
  xiiResult ReadDependencyFile(xiiStringView sFile);

  /// Retrieves the current file time stamps from the filesystem and determines whether any file has changed since the last call to
  /// StoreCurrentTimeStamp() (or ReadDependencyFile())
  bool HasAnyFileChanged() const;

  /// Retrieves the current file time stamps from the filesystem and stores it for later comparison. This value is also serialized through
  /// WriteDependencyFile(), so it should be called before that, to store the latest state.
  void StoreCurrentTimeStamp();

private:
  static xiiResult RetrieveFileTimeStamp(xiiStringView sFile, xiiTimestamp& out_Result);

  xiiHybridArray<xiiString, 16> m_AssetTransformDependencies;
  xiiInt64                      m_iMaxTimeStampStored  = 0;
  xiiUInt64                     m_uiSumTimeStampStored = 0;

  struct FileCheckCache
  {
    xiiTimestamp m_FileTimestamp;
    xiiTime      m_LastCheck;
  };

  static xiiMutex                          s_FileTimestampsLock;
  static xiiMap<xiiString, FileCheckCache> s_FileTimestamps;
};
