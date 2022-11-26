#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/EditorProcessCommunicationChannel.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/LogEntry.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/TaskSystem.h>

struct xiiAssetCuratorEvent;
class xiiTask;
struct xiiAssetInfo;

/// \brief Creates a file system watcher for the given filesystem config and informs the xiiAssetCurator
/// of any changes.
class XII_EDITORFRAMEWORK_DLL xiiAssetWatcher
{
public:
  xiiAssetWatcher(const xiiApplicationFileSystemConfig& fileSystemConfig);
  ~xiiAssetWatcher();

  /// \brief Needs to be called every frame. Handles update delays to allow compacting multiple changes.
  void MainThreadTick();

private:
  friend class xiiDirectoryUpdateTask;
  friend class xiiAssetCurator;
  struct WatcherResult
  {
    xiiString                 sFile;
    xiiDirectoryWatcherAction action;
    xiiDirectoryWatcherType   type;
  };

  static constexpr xiiUInt32 s_FrameDelay = 5;
  struct PendingUpdate
  {
    xiiString sAbsPath;
    xiiUInt32 m_uiFrameDelay = s_FrameDelay;
  };


  void HandleWatcherChange(const WatcherResult& res);
  void UpdateFile(const char* szAbsPath);
  void UpdateDirectory(const char* szAbsPath);

private:
  mutable xiiMutex                        m_WatcherMutex;
  xiiHybridArray<xiiDirectoryWatcher*, 6> m_Watchers;
  xiiSharedPtr<xiiTask>                   m_pWatcherTask;
  xiiTaskGroupID                          m_WatcherGroup;
  xiiAtomicBool                           m_bShutdown = false;

  xiiHybridArray<xiiTaskGroupID, 4> m_DirectoryUpdates;
  xiiHybridArray<PendingUpdate, 4>  m_UpdateFile;
  xiiHybridArray<PendingUpdate, 4>  m_UpdateDirectory;

  xiiApplicationFileSystemConfig m_FileSystemConfig;
};

/// \brief Task to scan a directory and inform the xiiAssetCurator of any changes.
class xiiDirectoryUpdateTask final : public xiiTask
{
public:
  xiiDirectoryUpdateTask(xiiAssetWatcher* pWatcher, const char* szFolder);
  ~xiiDirectoryUpdateTask();

  xiiAssetWatcher* m_pWatcher = nullptr;
  xiiString        m_sFolder;

private:
  virtual void Execute() override;
};
