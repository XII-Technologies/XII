#include <ToolsFoundation/ToolsFoundationDLL.h>

#if XII_ENABLED(XII_SUPPORTS_DIRECTORY_WATCHER) && XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS)

#  include <ToolsFoundation/FileSystem/FileSystemModel.h>
#  include <ToolsFoundation/FileSystem/FileSystemWatcher.h>

#  include <Foundation/Algorithm/HashStream.h>
#  include <Foundation/Configuration/SubSystem.h>
#  include <Foundation/IO/FileSystem/FileReader.h>
#  include <Foundation/IO/FileSystem/FileSystem.h>
#  include <Foundation/IO/MemoryStream.h>
#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Time/Stopwatch.h>
#  include <Foundation/Utilities/Progress.h>

XII_IMPLEMENT_SINGLETON(xiiFileSystemModel);

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(ToolsFoundation, FileSystemModel)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    XII_DEFAULT_NEW(xiiFileSystemModel);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiFileSystemModel* pDummy = xiiFileSystemModel::GetSingleton();
    XII_DEFAULT_DELETE(pDummy);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

namespace
{
  thread_local xiiHybridArray<xiiFileChangedEvent, 2, xiiStaticAllocatorWrapper>   g_PostponedFiles;
  thread_local bool                                                                g_bInFileBroadcast = false;
  thread_local xiiHybridArray<xiiFolderChangedEvent, 2, xiiStaticAllocatorWrapper> g_PostponedFolders;
  thread_local bool                                                                g_bInFolderBroadcast = false;
} // namespace

xiiFolderChangedEvent::xiiFolderChangedEvent(xiiStringView sFile, Type type) :
  m_sPath(sFile), m_Type(type)
{
}

xiiFileChangedEvent::xiiFileChangedEvent(xiiStringView sFile, xiiFileStatus status, Type type) :
  m_sPath(sFile), m_Status(status), m_Type(type)
{
}

bool xiiFileSystemModel::IsSameFile(const xiiStringView sAbsolutePathA, const xiiStringView sAbsolutePathB)
{
#  if (XII_ENABLED(XII_SUPPORTS_CASE_INSENSITIVE_PATHS))
  return sAbsolutePathA.IsEqual_NoCase(sAbsolutePathB);
#  else
  return sAbsolutePathA.IsEqual(sAbsolutePathB);
#  endif
}

////////////////////////////////////////////////////////////////////////
// xiiAssetFiles
////////////////////////////////////////////////////////////////////////

xiiFileSystemModel::xiiFileSystemModel() :
  m_SingletonRegistrar(this)
{
}

xiiFileSystemModel::~xiiFileSystemModel() = default;

void xiiFileSystemModel::Initialize(const xiiApplicationFileSystemConfig& fileSystemConfig, xiiMap<xiiString, xiiFileStatus>&& referencedFiles, xiiMap<xiiString, xiiFileStatus::Status>&& referencedFolders)
{
  {
    XII_LOCK(m_FilesMutex);
    m_FileSystemConfig = fileSystemConfig;

    m_ReferencedFiles   = std::move(referencedFiles);
    m_ReferencedFolders = std::move(referencedFolders);

    xiiStringBuilder sDataDirPath;
    m_DataDirRoots.Reserve(m_FileSystemConfig.m_DataDirs.GetCount());
    for (xiiUInt32 i = 0; i < m_FileSystemConfig.m_DataDirs.GetCount(); ++i)
    {
      if (xiiFileSystem::ResolveSpecialDirectory(m_FileSystemConfig.m_DataDirs[i].m_sDataDirSpecialPath, sDataDirPath).Failed())
      {
        xiiLog::Error("Failed to resolve data directory named '{}' at '{}'", m_FileSystemConfig.m_DataDirs[i].m_sRootName, m_FileSystemConfig.m_DataDirs[i].m_sDataDirSpecialPath);
        m_DataDirRoots.PushBack({});
      }
      else
      {
        sDataDirPath.MakeCleanPath();
        sDataDirPath.TrimWordEnd("/");

        m_DataDirRoots.PushBack(sDataDirPath);
        // The root should always be in the model so that every file's parent folder is present in the model.
        m_ReferencedFolders.FindOrAdd(sDataDirPath).Value() = xiiFileStatus::Status::Valid;
      }
    }

    m_pWatcher            = XII_DEFAULT_NEW(xiiFileSystemWatcher, m_FileSystemConfig);
    m_WatcherSubscription = m_pWatcher->m_Events.AddEventHandler(xiiMakeDelegate(&xiiFileSystemModel::OnAssetWatcherEvent, this));
    m_pWatcher->Initialize();
    m_bInitialized = true;
  }
  FireFileChangedEvent({}, {}, xiiFileChangedEvent::Type::ModelReset);
  FireFolderChangedEvent({}, xiiFolderChangedEvent::Type::ModelReset);
}


void xiiFileSystemModel::Deinitialize(xiiMap<xiiString, xiiFileStatus>* out_pReferencedFiles, xiiMap<xiiString, xiiFileStatus::Status>* out_pReferencedFolders)
{
  {
    XII_LOCK(m_FilesMutex);

    m_pWatcher->m_Events.RemoveEventHandler(m_WatcherSubscription);
    m_pWatcher->Deinitialize();
    m_pWatcher.Clear();

    if (out_pReferencedFiles)
    {
      m_ReferencedFiles.Swap(*out_pReferencedFiles);
    }
    if (out_pReferencedFolders)
    {
      m_ReferencedFolders.Swap(*out_pReferencedFolders);
    }
    m_ReferencedFiles.Clear();
    m_ReferencedFolders.Clear();
    m_LockedFiles.Clear();
    m_FileSystemConfig = xiiApplicationFileSystemConfig();
    m_DataDirRoots.Clear();
    m_bInitialized = false;
  }
  FireFileChangedEvent({}, {}, xiiFileChangedEvent::Type::ModelReset);
  FireFolderChangedEvent({}, xiiFolderChangedEvent::Type::ModelReset);
}

void xiiFileSystemModel::MainThreadTick()
{
  if (m_pWatcher)
    m_pWatcher->MainThreadTick();
}

const xiiFileSystemModel::LockedFiles xiiFileSystemModel::GetFiles() const
{
  return LockedFiles(m_FilesMutex, &m_ReferencedFiles);
}


const xiiFileSystemModel::LockedFolders xiiFileSystemModel::GetFolders() const
{
  return LockedFolders(m_FilesMutex, &m_ReferencedFolders);
}

void xiiFileSystemModel::NotifyOfChange(xiiStringView sAbsolutePath)
{
  if (!m_bInitialized)
    return;

  XII_ASSERT_DEV(xiiPathUtils::IsAbsolutePath(sAbsolutePath), "Only absolute paths are supported for directory iteration.");

  xiiStringBuilder sPath(sAbsolutePath);
  sPath.MakeCleanPath();

  // We ignore any changes outside the model's data dirs.
  if (FindDataDir(sAbsolutePath) == -1)
    return;

  HandleSingleFile(sPath, true);
}

void xiiFileSystemModel::CheckFileSystem()
{
  if (!m_bInitialized)
    return;

  XII_PROFILE_SCOPE("CheckFileSystem");

  xiiUniquePtr<xiiProgressRange> range = nullptr;
  if (xiiThreadUtils::IsMainThread())
    range = XII_DEFAULT_NEW(xiiProgressRange, "Check File-System for Assets", m_FileSystemConfig.m_DataDirs.GetCount(), false);

  {
    SetAllStatusUnknown();

    // check every data directory
    for (auto& dd : m_FileSystemConfig.m_DataDirs)
    {
      xiiStringBuilder sTemp;
      xiiFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).IgnoreResult();

      if (xiiThreadUtils::IsMainThread())
        range->BeginNextStep(dd.m_sDataDirSpecialPath);

      CheckFolder(sTemp);
    }

    RemoveStaleFileInfos();
  }

  if (xiiThreadUtils::IsMainThread())
  {
    range = nullptr;
    // Broadcast reset only if we are on the main thread.
    // Otherwise we are on the init task thread and the reset will be called on the main thread by WaitForInitialize.
    FireFileChangedEvent({}, {}, xiiFileChangedEvent::Type::ModelReset);
    FireFolderChangedEvent({}, xiiFolderChangedEvent::Type::ModelReset);
  }
}


xiiResult xiiFileSystemModel::FindFile(xiiStringView sPath, xiiFileStatus& out_stat) const
{
  if (!m_bInitialized)
    return XII_FAILURE;

  XII_LOCK(m_FilesMutex);
  xiiMap<xiiString, xiiFileStatus>::ConstIterator it;
  if (xiiPathUtils::IsAbsolutePath(sPath))
  {
    it = m_ReferencedFiles.Find(sPath);
  }
  else
  {
    // Data dir parent relative?
    for (const auto& dd : m_FileSystemConfig.m_DataDirs)
    {
      xiiStringBuilder sDataDir;
      xiiFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).AssertSuccess();
      sDataDir.PathParentDirectory();
      sDataDir.AppendPath(sPath);
      it = m_ReferencedFiles.Find(sDataDir);
      if (it.IsValid())
        break;
    }

    if (!it.IsValid())
    {
      // Data dir relative?
      for (const auto& dd : m_FileSystemConfig.m_DataDirs)
      {
        xiiStringBuilder sDataDir;
        xiiFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).AssertSuccess();
        sDataDir.AppendPath(sPath);
        it = m_ReferencedFiles.Find(sDataDir);
        if (it.IsValid())
          break;
      }
    }
  }

  if (it.IsValid())
  {
    out_stat = it.Value();
    return XII_SUCCESS;
  }
  return XII_FAILURE;
}


xiiResult xiiFileSystemModel::FindFile(xiiDelegate<bool(const xiiString&, const xiiFileStatus&)> visitor) const
{
  if (!m_bInitialized)
    return XII_FAILURE;

  XII_LOCK(m_FilesMutex);
  for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
  {
    if (visitor(it.Key(), it.Value()))
      return XII_SUCCESS;
  }
  return XII_FAILURE;
}


xiiResult xiiFileSystemModel::LinkDocument(xiiStringView sAbsolutePath, const xiiUuid& documentId)
{
  if (!m_bInitialized || !documentId.IsValid())
    return XII_FAILURE;

  xiiFileStatus fileStatus;
  bool          bDocumentLinkChanged = false;
  {
    XII_LOCK(m_FilesMutex);
    auto it = m_ReferencedFiles.Find(sAbsolutePath);
    if (it.IsValid())
    {
      bDocumentLinkChanged    = it.Value().m_DocumentID != documentId;
      it.Value().m_DocumentID = documentId;
      fileStatus              = it.Value();
    }
    else
    {
      return XII_FAILURE;
    }
  }

  if (bDocumentLinkChanged)
  {
    FireFileChangedEvent(sAbsolutePath, fileStatus, xiiFileChangedEvent::Type::DocumentLinked);
  }
  return XII_SUCCESS;
}

xiiResult xiiFileSystemModel::UnlinkDocument(xiiStringView sAbsolutePath)
{
  if (!m_bInitialized)
    return XII_FAILURE;

  xiiFileStatus fileStatus;
  bool          bDocumentLinkChanged = false;
  {
    XII_LOCK(m_FilesMutex);
    auto it = m_ReferencedFiles.Find(sAbsolutePath);
    if (it.IsValid())
    {
      bDocumentLinkChanged = it.Value().m_DocumentID != xiiUuid();
      it.Value().m_DocumentID.SetInvalid();
      fileStatus = it.Value();
    }
    else
    {
      return XII_FAILURE;
    }
  }

  if (bDocumentLinkChanged)
  {
    FireFileChangedEvent(sAbsolutePath, fileStatus, xiiFileChangedEvent::Type::DocumentUnlinked);
  }
  return XII_SUCCESS;
}

xiiResult xiiFileSystemModel::HashFile(xiiStringView sAbsolutePath, xiiFileStatus& out_stat)
{
  if (!m_bInitialized)
    return XII_FAILURE;

  XII_ASSERT_DEV(xiiPathUtils::IsAbsolutePath(sAbsolutePath), "Only absolute paths are supported for hashing.");

  xiiStringBuilder sAbsolutePath2(sAbsolutePath);
  sAbsolutePath2.MakeCleanPath();

  xiiFileStats statDep;
  if (xiiOSFile::GetFileStats(sAbsolutePath2, statDep).Failed())
  {
    xiiLog::Error("Failed to hash file '{0}', retrieve stats failed", sAbsolutePath2);
    return XII_FAILURE;
  }

  // We ignore any changes outside the model's data dirs.
  if (FindDataDir(sAbsolutePath2) != -1)
  {
    {
      XII_LOCK(m_FilesMutex);
      auto it = m_ReferencedFiles.Find(sAbsolutePath2);
      if (it.IsValid())
      {
        out_stat = it.Value();
      }
    }

    // We can only hash files that are tracked.
    if (out_stat.m_Status == xiiFileStatus::Status::Unknown)
    {
      out_stat = HandleSingleFile(sAbsolutePath2, statDep, false);
      if (out_stat.m_Status == xiiFileStatus::Status::Unknown)
      {
        xiiLog::Error("Failed to hash file '{0}', update failed", sAbsolutePath2);
        return XII_FAILURE;
      }
    }

    // if the file has been modified, make sure to get updated data
    if (!out_stat.m_LastModified.Compare(statDep.m_LastModificationTime, xiiTimestamp::CompareMode::Identical) || out_stat.m_uiHash == 0)
    {
      FILESYSTEM_PROFILE(sAbsolutePath2);
      xiiFileReader file;
      if (file.Open(sAbsolutePath2).Failed())
      {
        MarkFileLocked(sAbsolutePath2);
        xiiLog::Error("Failed to hash file '{0}', open failed", sAbsolutePath2);
        return XII_FAILURE;
      }

      // We need to request the stats again wile while we have shared read access or we might trigger a race condition of writes to the file between the last stat call and the current file open.
      if (xiiOSFile::GetFileStats(sAbsolutePath2, statDep).Failed())
      {
        xiiLog::Error("Failed to hash file '{0}', retrieve stats failed", sAbsolutePath2);
        return XII_FAILURE;
      }
      out_stat.m_LastModified = statDep.m_LastModificationTime;
      out_stat.m_uiHash       = xiiFileSystemModel::HashFile(file, nullptr);
      out_stat.m_Status       = xiiFileStatus::Status::Valid;

      // Update state. No need to compare timestamps we hold a lock on the file via the reader.
      XII_LOCK(m_FilesMutex);
      m_ReferencedFiles.Insert(sAbsolutePath2, out_stat);
    }
    return XII_SUCCESS;
  }
  else
  {
    {
      XII_LOCK(m_FilesMutex);
      auto it = m_TransiendFiles.Find(sAbsolutePath2);
      if (it.IsValid())
      {
        out_stat = it.Value();
      }
    }

    // if the file has been modified, make sure to get updated data
    if (!out_stat.m_LastModified.Compare(statDep.m_LastModificationTime, xiiTimestamp::CompareMode::Identical) || out_stat.m_uiHash == 0)
    {
      FILESYSTEM_PROFILE(sAbsolutePath2);
      xiiFileReader file;
      if (file.Open(sAbsolutePath2).Failed())
      {
        xiiLog::Error("Failed to hash file '{0}', open failed", sAbsolutePath2);
        return XII_FAILURE;
      }

      // We need to request the stats again wile while we have shared read access or we might trigger a race condition of writes to the file between the last stat call and the current file open.
      if (xiiOSFile::GetFileStats(sAbsolutePath2, statDep).Failed())
      {
        xiiLog::Error("Failed to hash file '{0}', retrieve stats failed", sAbsolutePath2);
        return XII_FAILURE;
      }
      out_stat.m_LastModified = statDep.m_LastModificationTime;
      out_stat.m_uiHash       = xiiFileSystemModel::HashFile(file, nullptr);
      out_stat.m_Status       = xiiFileStatus::Status::Valid;

      // Update state. No need to compare timestamps we hold a lock on the file via the reader.
      XII_LOCK(m_FilesMutex);
      m_TransiendFiles.Insert(sAbsolutePath2, out_stat);
    }
    return XII_SUCCESS;
  }
}


xiiUInt64 xiiFileSystemModel::HashFile(xiiStreamReader& ref_inputStream, xiiStreamWriter* pPassThroughStream)
{
  xiiHashStreamWriter64 hsw;

  FILESYSTEM_PROFILE("HashFile");
  xiiUInt8 cachedBytes[1024 * 10];

  while (true)
  {
    const xiiUInt64 uiRead = ref_inputStream.ReadBytes(cachedBytes, XII_ARRAY_SIZE(cachedBytes));

    if (uiRead == 0)
      break;

    hsw.WriteBytes(cachedBytes, uiRead).AssertSuccess();

    if (pPassThroughStream != nullptr)
      pPassThroughStream->WriteBytes(cachedBytes, uiRead).AssertSuccess();
  }

  return hsw.GetHashValue();
}

xiiResult xiiFileSystemModel::ReadDocument(xiiStringView sAbsolutePath, const xiiDelegate<xiiUuid(const xiiFileStatus&, xiiStreamReader&)>& callback)
{
  if (!m_bInitialized)
    return XII_FAILURE;

  // try to read the asset file
  xiiFileReader file;
  if (file.Open(sAbsolutePath) == XII_FAILURE)
  {
    MarkFileLocked(sAbsolutePath);
    xiiLog::Error("Failed to open file '{0}'", sAbsolutePath);
    return XII_FAILURE;
  }

  // Get model state.
  xiiFileStatus stat;
  {
    XII_LOCK(m_FilesMutex);
    auto it = m_ReferencedFiles.Find(sAbsolutePath);
    if (!it.IsValid())
      return XII_FAILURE;

    stat = it.Value();
  }

  // Get current state.
  xiiFileStats statDep;
  if (xiiOSFile::GetFileStats(sAbsolutePath, statDep).Failed())
  {
    xiiLog::Error("Failed to retrieve file stats '{0}'", sAbsolutePath);
    return XII_FAILURE;
  }

  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamReader         MemReader(&storage);
  MemReader.SetDebugSourceInformation(sAbsolutePath);

  xiiMemoryStreamWriter MemWriter(&storage);
  stat.m_LastModified = statDep.m_LastModificationTime;
  stat.m_Status       = xiiFileStatus::Status::Valid;
  stat.m_uiHash       = xiiFileSystemModel::HashFile(file, &MemWriter);

  if (callback.IsValid())
  {
    stat.m_DocumentID = callback(stat, MemReader);
  }

  bool bFileChanged         = false;
  bool bDocumentLinkChanged = false;
  {
    // Update state. No need to compare timestamps we hold a lock on the file via the reader.
    XII_LOCK(m_FilesMutex);
    auto it = m_ReferencedFiles.Find(sAbsolutePath);
    if (it.IsValid())
    {
      bFileChanged         = !it.Value().m_LastModified.Compare(stat.m_LastModified, xiiTimestamp::CompareMode::Identical);
      bDocumentLinkChanged = it.Value().m_DocumentID != stat.m_DocumentID;
      it.Value()           = stat;
    }
    else
    {
      XII_REPORT_FAILURE("A file was removed from the model while we had a lock on it.");
    }

    if (bFileChanged)
    {
      FireFileChangedEvent(sAbsolutePath, stat, xiiFileChangedEvent::Type::FileChanged);
    }
    if (bDocumentLinkChanged)
    {
      FireFileChangedEvent(sAbsolutePath, stat, xiiFileChangedEvent::Type::DocumentLinked);
    }
  }

  return XII_SUCCESS;
}

void xiiFileSystemModel::SetAllStatusUnknown()
{
  XII_LOCK(m_FilesMutex);
  for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_Status = xiiFileStatus::Status::Unknown;
  }

  for (auto it = m_ReferencedFolders.GetIterator(); it.IsValid(); ++it)
  {
    it.Value() = xiiFileStatus::Status::Unknown;
  }
}


void xiiFileSystemModel::RemoveStaleFileInfos()
{
  xiiSet<xiiString> unknownFiles;
  xiiSet<xiiString> unknownFolders;
  {
    XII_LOCK(m_FilesMutex);
    for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
    {
      // search for files that existed previously but have not been found anymore recently
      if (it.Value().m_Status == xiiFileStatus::Status::Unknown)
      {
        unknownFiles.Insert(it.Key());
      }
    }
    for (auto it = m_ReferencedFolders.GetIterator(); it.IsValid(); ++it)
    {
      // search for folders that existed previously but have not been found anymore recently
      if (it.Value() == xiiFileStatus::Status::Unknown)
      {
        unknownFolders.Insert(it.Key());
      }
    }
  }

  for (const xiiString& sFile : unknownFiles)
  {
    HandleSingleFile(sFile, false);
  }
  for (const xiiString& sFolders : unknownFolders)
  {
    HandleSingleFile(sFolders, false);
  }
}


void xiiFileSystemModel::CheckFolder(xiiStringView sAbsolutePath)
{
  xiiStringBuilder sAbsolutePath2 = sAbsolutePath;
  sAbsolutePath2.MakeCleanPath();
  XII_ASSERT_DEV(xiiPathUtils::IsAbsolutePath(sAbsolutePath2), "Only absolute paths are supported for directory iteration.");
  sAbsolutePath2.TrimWordEnd("/");

  if (sAbsolutePath2.IsEmpty())
    return;

  // We ignore any changes outside the model's data dirs.
  if (FindDataDir(sAbsolutePath2) == -1)
    return;

  bool bExists = false;
  {
    XII_LOCK(m_FilesMutex);
    bExists = m_ReferencedFolders.Contains(sAbsolutePath2);
  }
  if (!bExists)
  {
    // If the folder does not exist yet we call NotifyOfChange which handles add / removal recursively as well.
    NotifyOfChange(sAbsolutePath2);
    return;
  }

  xiiFileSystemIterator iterator;
  iterator.StartSearch(sAbsolutePath2, xiiFileSystemIteratorFlags::ReportFilesAndFoldersRecursive);

  if (!iterator.IsValid())
    return;

  xiiStringBuilder sPath;

  xiiSet<xiiString> visitedFiles;
  xiiSet<xiiString> visitedFolders;
  visitedFolders.Insert(sAbsolutePath2);

  for (; iterator.IsValid(); iterator.Next())
  {
    sPath = iterator.GetCurrentPath();
    sPath.AppendPath(iterator.GetStats().m_sName);
    sPath.MakeCleanPath();
    if (iterator.GetStats().m_bIsDirectory)
      visitedFolders.Insert(sPath);
    else
      visitedFiles.Insert(sPath);
    HandleSingleFile(sPath, iterator.GetStats(), false);
  }

  xiiDynamicArray<xiiString> missingFiles;
  xiiDynamicArray<xiiString> missingFolders;

  {
    XII_LOCK(m_FilesMutex);

    for (auto it = m_ReferencedFiles.LowerBound(sAbsolutePath2); it.IsValid() && it.Key().StartsWith(sAbsolutePath2); ++it)
    {
      if (!visitedFiles.Contains(it.Key()))
        missingFiles.PushBack(it.Key());
    }

    for (auto it = m_ReferencedFolders.LowerBound(sAbsolutePath2); it.IsValid() && it.Key().StartsWith(sAbsolutePath2); ++it)
    {
      if (!visitedFolders.Contains(it.Key()))
        missingFolders.PushBack(it.Key());
    }
  }

  for (const xiiString& sFile : missingFiles)
  {
    HandleSingleFile(sFile, false);
  }

  // Delete sub-folders before parent folders.
  missingFolders.Sort([](const xiiString& lhs, const xiiString& rhs) -> bool { return xiiStringUtils::Compare(lhs, rhs) > 0; });
  for (const xiiString& sFolder : missingFolders)
  {
    HandleSingleFile(sFolder, false);
  }
}

void xiiFileSystemModel::OnAssetWatcherEvent(const xiiFileSystemWatcherEvent& e)
{
  switch (e.m_Type)
  {
    case xiiFileSystemWatcherEvent::Type::FileAdded:
    case xiiFileSystemWatcherEvent::Type::FileRemoved:
    case xiiFileSystemWatcherEvent::Type::FileChanged:
    case xiiFileSystemWatcherEvent::Type::DirectoryAdded:
    case xiiFileSystemWatcherEvent::Type::DirectoryRemoved:
      NotifyOfChange(e.m_sPath);
      break;
  }
}

xiiFileStatus xiiFileSystemModel::HandleSingleFile(const xiiString& sAbsolutePath, bool bRecurseIntoFolders)
{
  FILESYSTEM_PROFILE("HandleSingleFile");

  xiiFileStats Stats;
  if (xiiOSFile::GetFileStats(sAbsolutePath, Stats).Failed())
  {
    xiiFileStatus fileStatus;
    bool          bFileExisted   = false;
    bool          bFolderExisted = false;
    {
      XII_LOCK(m_FilesMutex);
      if (auto it = m_ReferencedFiles.Find(sAbsolutePath); it.IsValid())
      {
        bFileExisted = true;
        fileStatus   = it.Value();
        m_ReferencedFiles.Remove(it);
      }
      if (auto it = m_ReferencedFolders.Find(sAbsolutePath); it.IsValid())
      {
        bFolderExisted = true;
        m_ReferencedFolders.Remove(it);
      }
    }

    if (bFileExisted)
    {
      FireFileChangedEvent(sAbsolutePath, fileStatus, xiiFileChangedEvent::Type::FileRemoved);
    }

    if (bFolderExisted)
    {
      if (bRecurseIntoFolders)
      {
        xiiSet<xiiString> previouslyKnownFiles;
        {
          FILESYSTEM_PROFILE("FindReferencedFiles");
          XII_LOCK(m_FilesMutex);
          auto itlowerBound = m_ReferencedFiles.LowerBound(sAbsolutePath);
          while (itlowerBound.IsValid() && itlowerBound.Key().StartsWith(sAbsolutePath))
          {
            previouslyKnownFiles.Insert(itlowerBound.Key());
            ++itlowerBound;
          }
        }
        {
          FILESYSTEM_PROFILE("HandleRemovedFiles");
          for (const xiiString& sFile : previouslyKnownFiles)
          {
            HandleSingleFile(sFile, false);
          }
        }
      }
      FireFolderChangedEvent(sAbsolutePath, xiiFolderChangedEvent::Type::FolderRemoved);
    }

    return {};
  }

  return HandleSingleFile(sAbsolutePath, Stats, bRecurseIntoFolders);
}


xiiFileStatus xiiFileSystemModel::HandleSingleFile(const xiiString& sAbsolutePath, const xiiFileStats& FileStat, bool bRecurseIntoFolders)
{
  FILESYSTEM_PROFILE("HandleSingleFile2");

  if (FileStat.m_bIsDirectory)
  {
    xiiFileStatus status;
    status.m_Status = xiiFileStatus::Status::Valid;

    bool bExisted = false;
    {
      XII_LOCK(m_FilesMutex);
      auto it    = m_ReferencedFolders.FindOrAdd(sAbsolutePath, &bExisted);
      it.Value() = xiiFileStatus::Status::Valid;
    }

    if (!bExisted)
    {
      FireFolderChangedEvent(sAbsolutePath, xiiFolderChangedEvent::Type::FolderAdded);
      if (bRecurseIntoFolders)
        CheckFolder(sAbsolutePath);
    }

    return status;
  }
  else
  {
    xiiFileStatus status;
    bool          bExisted     = false;
    bool          bFileChanged = false;
    {
      XII_LOCK(m_FilesMutex);
      auto           it    = m_ReferencedFiles.FindOrAdd(sAbsolutePath, &bExisted);
      xiiFileStatus& value = it.Value();
      bFileChanged         = !value.m_LastModified.Compare(FileStat.m_LastModificationTime, xiiTimestamp::CompareMode::Identical);
      if (bFileChanged)
      {
        value.m_uiHash = 0;
      }

      // If the state is unknown, we loaded it from the cache and need to fire FileChanged to update dependent systems.
      // #TODO_ASSET This behaviors should be changed once the asset cache is stored less lossy.
      bFileChanged |= value.m_Status == xiiFileStatus::Status::Unknown;
      // mark the file as valid (i.e. we saw it on disk, so it hasn't been deleted or such)
      value.m_Status       = xiiFileStatus::Status::Valid;
      value.m_LastModified = FileStat.m_LastModificationTime;
      status               = value;
    }

    if (!bExisted)
    {
      FireFileChangedEvent(sAbsolutePath, status, xiiFileChangedEvent::Type::FileAdded);
    }
    else if (bFileChanged)
    {
      FireFileChangedEvent(sAbsolutePath, status, xiiFileChangedEvent::Type::FileChanged);
    }
    return status;
  }
}

void xiiFileSystemModel::MarkFileLocked(xiiStringView sAbsolutePath)
{
  XII_LOCK(m_FilesMutex);
  auto it = m_ReferencedFiles.Find(sAbsolutePath);
  if (it.IsValid())
  {
    it.Value().m_Status = xiiFileStatus::Status::FileLocked;
    m_LockedFiles.Insert(sAbsolutePath);
  }
}

void xiiFileSystemModel::FireFileChangedEvent(xiiStringView sFile, xiiFileStatus fileStatus, xiiFileChangedEvent::Type type)
{
  // We queue up all requests on a thread and only return once the list is empty. The reason for this is that:
  // A: We don't want to allow recursive event calling as it creates limbo states in the model and hard to debug bugs.
  // B: If a user calls NotifyOfChange, the function should only return if the event and any indirect events that were triggered by the event handlers have been processed.

  xiiFileChangedEvent& e = g_PostponedFiles.ExpandAndGetRef();
  e.m_sPath              = sFile;
  e.m_Status             = fileStatus;
  e.m_Type               = type;

  if (g_bInFileBroadcast)
  {
    return;
  }

  g_bInFileBroadcast = true;
  XII_SCOPE_EXIT(g_bInFileBroadcast = false);

  for (xiiUInt32 i = 0; i < g_PostponedFiles.GetCount(); i++)
  {
    m_FileChangedEvents.Broadcast(g_PostponedFiles[i]);
  }
  g_PostponedFiles.Clear();
}

void xiiFileSystemModel::FireFolderChangedEvent(xiiStringView sFile, xiiFolderChangedEvent::Type type)
{
  // See comment in FireFileChangedEvent.
  xiiFolderChangedEvent& e = g_PostponedFolders.ExpandAndGetRef();
  e.m_sPath                = sFile;
  e.m_Type                 = type;

  if (g_bInFolderBroadcast)
  {
    return;
  }

  g_bInFolderBroadcast = true;
  XII_SCOPE_EXIT(g_bInFolderBroadcast = false);

  for (xiiUInt32 i = 0; i < g_PostponedFolders.GetCount(); i++)
  {
    m_FolderChangedEvents.Broadcast(g_PostponedFolders[i]);
  }
  g_PostponedFolders.Clear();
}

xiiInt32 xiiFileSystemModel::FindDataDir(const xiiStringView path)
{
  for (xiiUInt32 i = 0; i < m_DataDirRoots.GetCount(); ++i)
  {
    if (path.StartsWith(m_DataDirRoots[i]))
    {
      return (xiiInt32)i;
    }
  }
  return -1;
}

#endif
