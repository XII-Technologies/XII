#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetWatcher.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>

////////////////////////////////////////////////////////////////////////
// xiiAssetWatcher
////////////////////////////////////////////////////////////////////////

xiiAssetWatcher::xiiAssetWatcher(const xiiApplicationFileSystemConfig& fileSystemConfig)
{
  XII_PROFILE_SCOPE("xiiAssetWatcher");
  m_FileSystemConfig = fileSystemConfig;
  for (auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    xiiStringBuilder sTemp;
    if (xiiFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
    {
      xiiLog::Error("Failed to init directory watcher for dir '{0}'", dd.m_sDataDirSpecialPath);
      continue;
    }

    xiiDirectoryWatcher* pWatcher = XII_DEFAULT_NEW(xiiDirectoryWatcher);
    xiiResult            res =
      pWatcher->OpenDirectory(sTemp, xiiDirectoryWatcher::Watch::Deletes | xiiDirectoryWatcher::Watch::Writes | xiiDirectoryWatcher::Watch::Creates | xiiDirectoryWatcher::Watch::Renames | xiiDirectoryWatcher::Watch::Subdirectories);

    if (res.Failed())
    {
      XII_DEFAULT_DELETE(pWatcher);
      xiiLog::Error("Failed to init directory watcher for dir '{0}'", sTemp);
      continue;
    }

    m_Watchers.PushBack(pWatcher);
  }

  m_pWatcherTask = XII_DEFAULT_NEW(xiiDelegateTask<void>, "Watcher Update", [this]() {
    xiiHybridArray<WatcherResult, 16> watcherResults;
    for (xiiDirectoryWatcher* pWatcher : m_Watchers)
    {
      pWatcher->EnumerateChanges([pWatcher, &watcherResults](const char* szFilename, xiiDirectoryWatcherAction action, xiiDirectoryWatcherType type) {
        watcherResults.PushBack({szFilename, action, type});
      });
    }
    for (const WatcherResult& res : watcherResults)
    {
      HandleWatcherChange(res);
    }
  });
}


xiiAssetWatcher::~xiiAssetWatcher()
{
  m_bShutdown = true;
  xiiTaskGroupID watcherGroup;
  {
    XII_LOCK(m_WatcherMutex);
    watcherGroup = m_WatcherGroup;
  }
  xiiTaskSystem::WaitForGroup(watcherGroup);
  {
    XII_LOCK(m_WatcherMutex);
    m_pWatcherTask.Clear();
    for (xiiDirectoryWatcher* pWatcher : m_Watchers)
    {
      XII_DEFAULT_DELETE(pWatcher);
    }
    m_Watchers.Clear();
  }

  do
  {
    xiiTaskGroupID id;
    {
      XII_LOCK(m_WatcherMutex);
      if (m_DirectoryUpdates.IsEmpty())
        break;
      id = m_DirectoryUpdates.PeekBack();
    }
    xiiTaskSystem::WaitForGroup(id);
  } while (true);

  XII_LOCK(m_WatcherMutex);
  XII_ASSERT_DEV(m_DirectoryUpdates.IsEmpty(), "All directory updates should have finished.");
}

void xiiAssetWatcher::MainThreadTick()
{
  XII_PROFILE_SCOPE("xiiAssetWatcherTick");
  XII_LOCK(m_WatcherMutex);
  if (!m_bShutdown && m_pWatcherTask && xiiTaskSystem::IsTaskGroupFinished(m_WatcherGroup))
  {
    m_WatcherGroup = xiiTaskSystem::StartSingleTask(m_pWatcherTask, xiiTaskPriority::LongRunningHighPriority);
  }

  // Files
  xiiAssetCurator* pCurator = xiiAssetCurator::GetSingleton();

  for (xiiUInt32 i = m_UpdateFile.GetCount(); i > 0; --i)
  {
    PendingUpdate& update = m_UpdateFile[i - 1];
    --update.m_uiFrameDelay;
    if (update.m_uiFrameDelay == 0)
    {
      pCurator->NotifyOfFileChange(update.sAbsPath);
      m_UpdateFile.RemoveAtAndSwap(i - 1);
    }
  }

  // Directories
  for (xiiUInt32 i = m_UpdateDirectory.GetCount(); i > 0; --i)
  {
    PendingUpdate& update = m_UpdateDirectory[i - 1];
    --update.m_uiFrameDelay;
    if (update.m_uiFrameDelay == 0 && !m_bShutdown)
    {
      xiiSharedPtr<xiiTask> pTask = XII_DEFAULT_NEW(xiiDirectoryUpdateTask, this, update.sAbsPath);
      xiiTaskGroupID        id    = xiiTaskSystem::StartSingleTask(pTask, xiiTaskPriority::LongRunningHighPriority, [this](xiiTaskGroupID id) {
        XII_LOCK(m_WatcherMutex);
        m_DirectoryUpdates.RemoveAndSwap(id);
      });
      m_DirectoryUpdates.PushBack(id);

      m_UpdateDirectory.RemoveAtAndSwap(i - 1);
    }
  }
}

void xiiAssetWatcher::HandleWatcherChange(const WatcherResult& res)
{
  xiiAssetCurator* pCurator = xiiAssetCurator::GetSingleton();
  xiiFileStats     stat;
  xiiResult        stats       = xiiOSFile::GetFileStats(res.sFile, stat);
  bool             isFileKnown = false;
  {
    XII_LOCK(pCurator->m_CuratorMutex);
    isFileKnown = pCurator->m_ReferencedFiles.Find(res.sFile).IsValid();
  }

  switch (res.action)
  {
    case xiiDirectoryWatcherAction::None:
      XII_ASSERT_DEV(false, "None event should never happen");
      break;
    case xiiDirectoryWatcherAction::Added:
    {
      if (stats == XII_SUCCESS)
      {
        if (stat.m_bIsDirectory)
        {
          UpdateDirectory(res.sFile);
        }
        else
        {
          UpdateFile(res.sFile);
        }
      }
    }
    break;
    case xiiDirectoryWatcherAction::Removed:
    {
      if (isFileKnown)
      {
        UpdateFile(res.sFile);
      }
      else
      {
        UpdateDirectory(res.sFile);
      }
    }
    break;
    case xiiDirectoryWatcherAction::Modified:
      if (stats == XII_SUCCESS)
      {
        if (stat.m_bIsDirectory)
        {
          // TODO: Can directories be modified?
        }
        else
        {
          UpdateFile(res.sFile);
        }
      }
      break;
    case xiiDirectoryWatcherAction::RenamedOldName:
      // Ignore, we scan entire parent dir in the following RenamedNewName event.
      break;
    case xiiDirectoryWatcherAction::RenamedNewName:
    {
      // Rescan parent directory.
      // TODO: Renames on root will rescan the entire data dir.
      // However, fixing this would require dynamic recursion if we detect that
      // a folder was actually renamed.
      xiiStringBuilder sParentFolder = res.sFile;
      sParentFolder.PathParentDirectory();
      UpdateDirectory(sParentFolder);
    }
    break;
  }
}

void xiiAssetWatcher::UpdateFile(const char* szAbsPath)
{
  XII_LOCK(m_WatcherMutex);
  for (PendingUpdate& update : m_UpdateFile)
  {
    if (update.sAbsPath == szAbsPath)
    {
      update.m_uiFrameDelay = s_FrameDelay;
      return;
    }
  }
  PendingUpdate& update = m_UpdateFile.ExpandAndGetRef();
  update.m_uiFrameDelay = s_FrameDelay;
  update.sAbsPath       = szAbsPath;
}

void xiiAssetWatcher::UpdateDirectory(const char* szAbsPath)
{
  xiiStringBuilder sAbsPath = szAbsPath;
  XII_LOCK(m_WatcherMutex);
  for (PendingUpdate& update : m_UpdateDirectory)
  {
    // No need to add this directory if itself or a parent directory is already queued.
    if (sAbsPath.IsPathBelowFolder(update.sAbsPath))
    {
      // Reset delay as we are probably doing a bigger operation right now.
      update.m_uiFrameDelay = s_FrameDelay;
      return;
    }
  }
  PendingUpdate& update = m_UpdateDirectory.ExpandAndGetRef();
  update.m_uiFrameDelay = s_FrameDelay;
  update.sAbsPath       = sAbsPath;
}

////////////////////////////////////////////////////////////////////////
// xiiDirectoryUpdateTask
////////////////////////////////////////////////////////////////////////

xiiDirectoryUpdateTask::xiiDirectoryUpdateTask(xiiAssetWatcher* pWatcher, const char* szFolder) :
  m_pWatcher(pWatcher), m_sFolder(szFolder)
{
  ConfigureTask("xiiDirectoryUpdateTask", xiiTaskNesting::Never);
}

xiiDirectoryUpdateTask::~xiiDirectoryUpdateTask() {}

void xiiDirectoryUpdateTask::Execute()
{
  xiiAssetCurator*  pCurator = xiiAssetCurator::GetSingleton();
  xiiSet<xiiString> previouslyKnownFiles;
  {
    CURATOR_PROFILE("FindReferencedFiles");
    // Find all currently known files that are under the given folder.
    XII_LOCK(pCurator->m_CuratorMutex);
    auto itlowerBound = pCurator->m_ReferencedFiles.LowerBound(m_sFolder);
    while (itlowerBound.IsValid() && itlowerBound.Key().StartsWith_NoCase(m_sFolder))
    {
      previouslyKnownFiles.Insert(itlowerBound.Key());
      ++itlowerBound;
    }
  }

  {
    CURATOR_PROFILE("IterateDataDirectory");
    // Iterate folder to find all actually existing files on disk.
    xiiSet<xiiString> knownFiles;
    pCurator->IterateDataDirectory(m_sFolder, &knownFiles);

    // Not encountered files are now removed which the xiiAssetCurator must be informed about.
    previouslyKnownFiles.Difference(knownFiles);
  }

  CURATOR_PROFILE("HandleRemovedFiles");
  for (const xiiString& sFile : previouslyKnownFiles)
  {
    pCurator->HandleSingleFile(sFile);
  }
}
