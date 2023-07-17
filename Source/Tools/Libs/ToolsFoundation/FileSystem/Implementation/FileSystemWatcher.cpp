#include <ToolsFoundation/ToolsFoundationDLL.h>

#if XII_ENABLED(XII_SUPPORTS_DIRECTORY_WATCHER)

#  include <ToolsFoundation/FileSystem/FileSystemWatcher.h>

#  include <Foundation/IO/DirectoryWatcher.h>
#  include <Foundation/IO/FileSystem/FileSystem.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Threading/DelegateTask.h>

////////////////////////////////////////////////////////////////////////
// xiiAssetWatcher
////////////////////////////////////////////////////////////////////////

xiiFileSystemWatcher::xiiFileSystemWatcher(const xiiApplicationFileSystemConfig& fileSystemConfig)
{
  m_FileSystemConfig = fileSystemConfig;
}


xiiFileSystemWatcher::~xiiFileSystemWatcher() = default;

void xiiFileSystemWatcher::Initialize()
{
  XII_PROFILE_SCOPE("Initialize");

  for (auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    xiiStringBuilder sTemp;
    if (xiiFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
    {
      xiiLog::Error("Failed to init directory watcher for dir '{0}'", dd.m_sDataDirSpecialPath);
      continue;
    }

    xiiDirectoryWatcher* pWatcher = XII_DEFAULT_NEW(xiiDirectoryWatcher);
    xiiResult            res      = pWatcher->OpenDirectory(sTemp, xiiDirectoryWatcher::Watch::Deletes | xiiDirectoryWatcher::Watch::Writes | xiiDirectoryWatcher::Watch::Creates | xiiDirectoryWatcher::Watch::Renames | xiiDirectoryWatcher::Watch::Subdirectories);

    if (res.Failed())
    {
      XII_DEFAULT_DELETE(pWatcher);
      xiiLog::Error("Failed to init directory watcher for dir '{0}'", sTemp);
      continue;
    }

    m_Watchers.PushBack(pWatcher);
  }

  m_pWatcherTask = XII_DEFAULT_NEW(xiiDelegateTask<void>, "Watcher Changes", [this]() {
    xiiHybridArray<WatcherResult, 16> watcherResults;
    for (xiiDirectoryWatcher* pWatcher : m_Watchers)
    {
      pWatcher->EnumerateChanges([pWatcher, &watcherResults](xiiStringView sFilename, xiiDirectoryWatcherAction action, xiiDirectoryWatcherType type) { watcherResults.PushBack({sFilename, action, type}); });
    }
    for (const WatcherResult& res : watcherResults)
    {
      HandleWatcherChange(res);
    } //
  });
  // This is a separate task as these trigger callbacks which can potentially take a long time and we can't have the watcher changes task be blocked for so long or notifications might get lost.
  m_pNotifyTask = XII_DEFAULT_NEW(xiiDelegateTask<void>, "Watcher Notify", [this]() { NotifyChanges(); });
}

void xiiFileSystemWatcher::Deinitialize()
{
  m_bShutdown = true;
  xiiTaskGroupID watcherGroup;
  xiiTaskGroupID notifyGroup;
  {
    XII_LOCK(m_WatcherMutex);
    watcherGroup = m_WatcherGroup;
    notifyGroup  = m_NotifyGroup;
  }
  xiiTaskSystem::WaitForGroup(watcherGroup);
  xiiTaskSystem::WaitForGroup(notifyGroup);
  {
    XII_LOCK(m_WatcherMutex);
    m_pWatcherTask.Clear();
    m_pNotifyTask.Clear();
    for (xiiDirectoryWatcher* pWatcher : m_Watchers)
    {
      XII_DEFAULT_DELETE(pWatcher);
    }
    m_Watchers.Clear();
  }
}

void xiiFileSystemWatcher::MainThreadTick()
{
  XII_PROFILE_SCOPE("xiiAssetWatcherTick");
  XII_LOCK(m_WatcherMutex);
  if (!m_bShutdown && m_pWatcherTask && xiiTaskSystem::IsTaskGroupFinished(m_WatcherGroup))
  {
    m_WatcherGroup = xiiTaskSystem::StartSingleTask(m_pWatcherTask, xiiTaskPriority::LongRunningHighPriority);
  }
  if (!m_bShutdown && m_pNotifyTask && xiiTaskSystem::IsTaskGroupFinished(m_NotifyGroup))
  {
    m_NotifyGroup = xiiTaskSystem::StartSingleTask(m_pNotifyTask, xiiTaskPriority::LongRunningHighPriority);
  }
}


void xiiFileSystemWatcher::NotifyChanges()
{
  auto NotifyChange = [this](const xiiString& sAbsPath, xiiFileSystemWatcherEvent::Type type) {
    xiiFileSystemWatcherEvent e;
    e.m_sPath = sAbsPath;
    e.m_Type  = type;
    m_Events.Broadcast(e);
  };

  // Files
  ConsumeEntry(m_FileAdded, xiiFileSystemWatcherEvent::Type::FileAdded, NotifyChange);
  ConsumeEntry(m_FileChanged, xiiFileSystemWatcherEvent::Type::FileChanged, NotifyChange);
  ConsumeEntry(m_FileRemoved, xiiFileSystemWatcherEvent::Type::FileRemoved, NotifyChange);

  // Directories
  ConsumeEntry(m_DirectoryAdded, xiiFileSystemWatcherEvent::Type::DirectoryAdded, NotifyChange);
  ConsumeEntry(m_DirectoryRemoved, xiiFileSystemWatcherEvent::Type::DirectoryRemoved, NotifyChange);
}

void xiiFileSystemWatcher::HandleWatcherChange(const WatcherResult& res)
{
  switch (res.m_Action)
  {
    case xiiDirectoryWatcherAction::None:
      XII_ASSERT_DEV(false, "None event should never happen");
      break;
    case xiiDirectoryWatcherAction::RenamedNewName:
    case xiiDirectoryWatcherAction::Added:
    {
      if (res.m_Type == xiiDirectoryWatcherType::Directory)
      {
        AddEntry(m_DirectoryAdded, res.m_sFile, s_AddedFrameDelay);
      }
      else
      {
        AddEntry(m_FileAdded, res.m_sFile, s_AddedFrameDelay);
      }
    }
    break;
    case xiiDirectoryWatcherAction::RenamedOldName:
    case xiiDirectoryWatcherAction::Removed:
    {
      if (res.m_Type == xiiDirectoryWatcherType::Directory)
      {
        AddEntry(m_DirectoryRemoved, res.m_sFile, s_RemovedFrameDelay);
      }
      else
      {
        AddEntry(m_FileRemoved, res.m_sFile, s_RemovedFrameDelay);
      }
    }
    break;
    case xiiDirectoryWatcherAction::Modified:
    {
      if (res.m_Type == xiiDirectoryWatcherType::Directory)
      {
        // Can a directory even be modified? In any case, we ignore this change.
        // UpdateEntry(m_DirectoryRemoved, res.sFile, s_RemovedFrameDelay);
      }
      else
      {
        AddEntry(m_FileChanged, res.m_sFile, s_AddedFrameDelay);
      }
    }
    break;
  }
}

void xiiFileSystemWatcher::AddEntry(xiiDynamicArray<PendingUpdate>& container, const xiiStringView sAbsPath, xiiUInt32 uiFrameDelay)
{
  XII_LOCK(m_WatcherMutex);
  for (PendingUpdate& update : container)
  {
    if (update.m_sAbsPath == sAbsPath)
    {
      update.m_uiFrameDelay = uiFrameDelay;
      return;
    }
  }
  PendingUpdate& update = container.ExpandAndGetRef();
  update.m_uiFrameDelay = uiFrameDelay;
  update.m_sAbsPath     = sAbsPath;
}

void xiiFileSystemWatcher::ConsumeEntry(xiiDynamicArray<PendingUpdate>& container, xiiFileSystemWatcherEvent::Type type, const xiiDelegate<void(const xiiString& sAbsPath, xiiFileSystemWatcherEvent::Type type)>& consume)
{
  xiiHybridArray<PendingUpdate, 16> updates;
  {
    XII_LOCK(m_WatcherMutex);
    for (xiiUInt32 i = container.GetCount(); i > 0; --i)
    {
      PendingUpdate& update = container[i - 1];
      --update.m_uiFrameDelay;
      if (update.m_uiFrameDelay == 0)
      {
        updates.PushBack(update);
        container.RemoveAtAndSwap(i - 1);
      }
    }
  }
  for (const PendingUpdate& update : updates)
  {
    consume(update.m_sAbsPath, type);
  }
}

#endif
