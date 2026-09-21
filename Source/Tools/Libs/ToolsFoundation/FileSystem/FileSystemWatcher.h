/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>

#if XII_ENABLED(XII_SUPPORTS_DIRECTORY_WATCHER)

#  include <Foundation/Application/Config/FileSystemConfig.h>
#  include <Foundation/IO/DirectoryWatcher.h>
#  include <Foundation/Threading/TaskSystem.h>

class xiiTask;

/// Event fired by xiiFileSystemWatcher::m_Events.
struct xiiFileSystemWatcherEvent
{
  enum class Type
  {
    FileAdded,
    FileRemoved,
    FileChanged,
    DirectoryAdded,
    DirectoryRemoved,
  };

  xiiStringView m_sPath;
  Type          m_Type;
};

/// Creates a file system watcher for the given filesystem config and fires any changes on a worker task via an event.
class XII_TOOLSFOUNDATION_DLL xiiFileSystemWatcher
{
public:
  xiiFileSystemWatcher(const xiiApplicationFileSystemConfig& fileSystemConfig);
  ~xiiFileSystemWatcher();

  /// Once called, file system watchers are created for each data directory and changes are observed.
  void Initialize();

  /// Waits for all pending tasks to complete and then stops observing changes and destroys file system watchers.
  void Deinitialize();

  /// Needs to be called at regular intervals (e.g. each frame) to restart background tasks.
  void MainThreadTick();

public:
  xiiEvent<const xiiFileSystemWatcherEvent&, xiiMutex> m_Events;

private:
  // On file move / rename operations we want the new file to be seen first before the old file delete event so that we can correctly detect this as a move instead of a delete operation. We achieve this by delaying each event by a fixed number of frames.
  static constexpr xiiUInt32 s_AddedFrameDelay   = 5;
  static constexpr xiiUInt32 s_RemovedFrameDelay = 10;
  // Sometimes moving a file triggers a modified event on the old file. To prevent this from triggering the removal to be seen before the addition, we also delay modified events by the same amount as remove events.
  static constexpr xiiUInt32 s_ModifiedFrameDelay = 10;

  struct WatcherResult
  {
    xiiString                 m_sFile;
    xiiDirectoryWatcherAction m_Action;
    xiiDirectoryWatcherType   m_Type;
  };

  struct PendingUpdate
  {
    xiiString m_sAbsPath;
    xiiUInt32 m_uiFrameDelay = 0;
  };

  /// Handles a single change notification by a directory watcher.
  void HandleWatcherChange(const WatcherResult& res);
  /// Handles update delays to allow compacting multiple changes.
  void NotifyChanges();
  /// Adds a change with the given delay to the container. If the entry is already present, only its delay is increased.
  void AddEntry(xiiDynamicArray<PendingUpdate>& container, const xiiStringView sAbsPath, xiiUInt32 uiFrameDelay);
  /// Reduces the delay counter of every item in the container. If a delay reaches zero, it is removed and the callback is fired.
  void ConsumeEntry(xiiDynamicArray<PendingUpdate>& container, xiiFileSystemWatcherEvent::Type type, const xiiDelegate<void(const xiiString& sAbsPath, xiiFileSystemWatcherEvent::Type type)>& consume);

private:
  // Immutable data after StartInitialize
  xiiApplicationFileSystemConfig m_FileSystemConfig;

  // Watchers
  mutable xiiMutex                        m_WatcherMutex;
  xiiHybridArray<xiiDirectoryWatcher*, 6> m_Watchers;
  xiiSharedPtr<xiiTask>                   m_pWatcherTask;
  xiiSharedPtr<xiiTask>                   m_pNotifyTask;
  xiiTaskGroupID                          m_WatcherGroup;
  xiiTaskGroupID                          m_NotifyGroup;
  xiiAtomicBool                           m_bShutdown = false;

  // Pending operations
  xiiHybridArray<PendingUpdate, 4> m_FileAdded;
  xiiHybridArray<PendingUpdate, 4> m_FileRemoved;
  xiiHybridArray<PendingUpdate, 4> m_FileChanged;
  xiiHybridArray<PendingUpdate, 4> m_DirectoryAdded;
  xiiHybridArray<PendingUpdate, 4> m_DirectoryRemoved;
};

#endif
