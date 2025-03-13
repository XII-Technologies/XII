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

xiiFolderChangedEvent::xiiFolderChangedEvent(const xiiDataDirPath& file, Type type) :
  m_Path(file), m_Type(type)
{
}

xiiFileChangedEvent::xiiFileChangedEvent(const xiiDataDirPath& file, xiiFileStatus status, Type type) :
  m_Path(file), m_Status(status), m_Type(type)
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

void xiiFileSystemModel::Initialize(const xiiApplicationFileSystemConfig& fileSystemConfig, xiiFileSystemModel::FilesMap&& referencedFiles, xiiFileSystemModel::FoldersMap&& referencedFolders)
{
  {
    XII_PROFILE_SCOPE("Initialize");
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
        sDataDirPath.Trim(nullptr, "/");

        m_DataDirRoots.PushBack(sDataDirPath);

        // The root should always be in the model so that every file's parent folder is present in the model.
        m_ReferencedFolders.FindOrAdd(xiiDataDirPath(sDataDirPath, m_DataDirRoots, i)).Value() = xiiFileStatus::Status::Valid;
      }
    }

    // Update data dir index and remove files no longer inside a data dir.
    for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid();)
    {
      const bool bValid = it.Key().UpdateDataDirInfos(m_DataDirRoots, it.Key().GetDataDirIndex());
      if (!bValid)
      {
        it = m_ReferencedFiles.Remove(it);
      }
      else
      {
        ++it;
      }
    }
    for (auto it = m_ReferencedFolders.GetIterator(); it.IsValid();)
    {
      const bool bValid = it.Key().UpdateDataDirInfos(m_DataDirRoots, it.Key().GetDataDirIndex());
      if (!bValid)
      {
        it = m_ReferencedFolders.Remove(it);
      }
      else
      {
        ++it;
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


void xiiFileSystemModel::Deinitialize(xiiFileSystemModel::FilesMap* out_pReferencedFiles, xiiFileSystemModel::FoldersMap* out_pReferencedFolders)
{
  {
    XII_LOCK(m_FilesMutex);
    XII_PROFILE_SCOPE("Deinitialize");
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
  sPath.Trim(nullptr, "/");
  if (sPath.IsEmpty())
    return;
  xiiDataDirPath folder(sPath, m_DataDirRoots);

  // We ignore any changes outside the model's data dirs.
  if (!folder.IsValid())
    return;

  HandleSingleFile(std::move(folder), true);
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
    for (xiiUInt32 i = 0; i < m_FileSystemConfig.m_DataDirs.GetCount(); i++)
    {
      auto& dd = m_FileSystemConfig.m_DataDirs[i];
      if (xiiThreadUtils::IsMainThread())
        range->BeginNextStep(dd.m_sDataDirSpecialPath);
      if (!m_DataDirRoots[i].IsEmpty())
      {
        CheckFolder(m_DataDirRoots[i]);
      }
    }

    RemoveStaleFileInfos();
  }

  if (xiiThreadUtils::IsMainThread())
  {
    range = nullptr;
  }

  FireFileChangedEvent({}, {}, xiiFileChangedEvent::Type::ModelReset);
  FireFolderChangedEvent({}, xiiFolderChangedEvent::Type::ModelReset);
}


xiiResult xiiFileSystemModel::FindFile(xiiStringView sPath, xiiFileStatus& out_stat) const
{
  if (!m_bInitialized)
    return XII_FAILURE;

  XII_LOCK(m_FilesMutex);
  xiiFileSystemModel::FilesMap::ConstIterator it;
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


xiiResult xiiFileSystemModel::FindFile(xiiDelegate<bool(const xiiDataDirPath&, const xiiFileStatus&)> visitor) const
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

  xiiDataDirPath filePath;
  xiiFileStatus  fileStatus;
  {
    XII_LOCK(m_FilesMutex);
    auto it = m_ReferencedFiles.Find(sAbsolutePath);
    if (it.IsValid())
    {
      // Store status before updates so we can fire the unlink if a guid was already set.
      fileStatus              = it.Value();
      it.Value().m_DocumentID = documentId;
      filePath                = it.Key();
    }
    else
    {
      return XII_FAILURE;
    }
  }

  if (fileStatus.m_DocumentID != documentId)
  {
    if (fileStatus.m_DocumentID.IsValid())
    {
      FireFileChangedEvent(filePath, fileStatus, xiiFileChangedEvent::Type::DocumentUnlinked);
    }
    fileStatus.m_DocumentID = documentId;
    FireFileChangedEvent(std::move(filePath), fileStatus, xiiFileChangedEvent::Type::DocumentLinked);
  }
  return XII_SUCCESS;
}

xiiResult xiiFileSystemModel::UnlinkDocument(xiiStringView sAbsolutePath)
{
  if (!m_bInitialized)
    return XII_FAILURE;

  xiiDataDirPath filePath;
  xiiFileStatus  fileStatus;
  bool           bDocumentLinkChanged = false;
  {
    XII_LOCK(m_FilesMutex);
    auto it = m_ReferencedFiles.Find(sAbsolutePath);
    if (it.IsValid())
    {
      bDocumentLinkChanged    = it.Value().m_DocumentID.IsValid();
      fileStatus              = it.Value();
      it.Value().m_DocumentID = xiiUuid::MakeInvalid();
      filePath                = it.Key();
    }
    else
    {
      return XII_FAILURE;
    }
  }

  if (bDocumentLinkChanged)
  {
    FireFileChangedEvent(std::move(filePath), fileStatus, xiiFileChangedEvent::Type::DocumentUnlinked);
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
  sAbsolutePath2.Trim("", "/");
  if (sAbsolutePath2.IsEmpty())
    return XII_FAILURE;

  xiiDataDirPath file(sAbsolutePath2, m_DataDirRoots);

  xiiFileStats statDep;
  if (xiiOSFile::GetFileStats(sAbsolutePath2, statDep).Failed())
  {
    xiiLog::Error("Failed to hash file '{0}', retrieve stats failed", sAbsolutePath2);
    return XII_FAILURE;
  }

  // We ignore any changes outside the model's data dirs.
  if (file.IsValid())
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
      out_stat = HandleSingleFile(file, statDep, false);
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
      xiiFileReader fileReader;
      if (fileReader.Open(sAbsolutePath2).Failed())
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
      out_stat.m_uiHash       = xiiFileSystemModel::HashFile(fileReader, nullptr);
      out_stat.m_Status       = xiiFileStatus::Status::Valid;

      // Update state. No need to compare timestamps we hold a lock on the file via the reader.
      XII_LOCK(m_FilesMutex);
      m_ReferencedFiles.Insert(file, out_stat);
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
      xiiFileReader modifiedFile;
      if (modifiedFile.Open(sAbsolutePath2).Failed())
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
      out_stat.m_uiHash       = xiiFileSystemModel::HashFile(modifiedFile, nullptr);
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

xiiResult xiiFileSystemModel::ReadDocument(xiiStringView sAbsolutePath, const xiiDelegate<void(const xiiFileStatus&, xiiStreamReader&)>& callback)
{
  if (!m_bInitialized)
    return XII_FAILURE;

  xiiStringBuilder sAbsolutePath2(sAbsolutePath);
  sAbsolutePath2.MakeCleanPath();
  sAbsolutePath2.Trim(nullptr, "/");

  // try to read the asset file
  xiiFileReader file;
  if (file.Open(sAbsolutePath2) == XII_FAILURE)
  {
    MarkFileLocked(sAbsolutePath2);
    xiiLog::Error("Failed to open file '{0}'", sAbsolutePath2);
    return XII_FAILURE;
  }

  // Get model state.
  xiiFileStatus stat;
  {
    XII_LOCK(m_FilesMutex);
    auto it = m_ReferencedFiles.Find(sAbsolutePath2);
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
    callback(stat, MemReader);
  }

  bool bFileChanged = false;
  {
    // Update state. No need to compare timestamps we hold a lock on the file via the reader.
    XII_LOCK(m_FilesMutex);
    auto it = m_ReferencedFiles.Find(sAbsolutePath2);
    if (it.IsValid())
    {
      bFileChanged = !it.Value().m_LastModified.Compare(stat.m_LastModified, xiiTimestamp::CompareMode::Identical);
      it.Value()   = stat;
    }
    else
    {
      XII_REPORT_FAILURE("A file was removed from the model while we had a lock on it.");
    }

    if (bFileChanged)
    {
      FireFileChangedEvent(it.Key(), stat, xiiFileChangedEvent::Type::FileChanged);
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
  xiiSet<xiiDataDirPath> unknownFiles;
  xiiSet<xiiDataDirPath> unknownFolders;
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

  for (const xiiDataDirPath& file : unknownFiles)
  {
    HandleSingleFile(file, false);
  }
  for (const xiiDataDirPath& folders : unknownFolders)
  {
    HandleSingleFile(folders, false);
  }
}


void xiiFileSystemModel::CheckFolder(xiiStringView sAbsolutePath)
{
  xiiStringBuilder sAbsolutePath2 = sAbsolutePath;
  sAbsolutePath2.MakeCleanPath();
  XII_ASSERT_DEV(xiiPathUtils::IsAbsolutePath(sAbsolutePath2), "Only absolute paths are supported for directory iteration.");
  sAbsolutePath2.Trim(nullptr, "/");

  if (sAbsolutePath2.IsEmpty())
    return;

  xiiDataDirPath folder(sAbsolutePath2, m_DataDirRoots);

  // We ignore any changes outside the model's data dirs.
  if (!folder.IsValid())
    return;

  bool bExists = false;
  {
    XII_LOCK(m_FilesMutex);
    bExists = m_ReferencedFolders.Contains(folder);
  }
  if (!bExists)
  {
    // If the folder does not exist yet we call NotifyOfChange which handles add / removal recursively as well.
    NotifyOfChange(folder);
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

    xiiDataDirPath path(sPath, m_DataDirRoots, folder.GetDataDirIndex());
    HandleSingleFile(std::move(path), iterator.GetStats(), false);
  }

  xiiDynamicArray<xiiString> missingFiles;
  xiiDynamicArray<xiiString> missingFolders;

  {
    XII_LOCK(m_FilesMutex);

    // As we are using xiiCompareDataDirPath, entries of different casing interleave but we are only interested in the ones with matching casing so we skip the rest.
    for (auto it = m_ReferencedFiles.LowerBound(sAbsolutePath2.GetView()); it.IsValid(); ++it)
    {
      if (xiiPathUtils::IsSubPath(sAbsolutePath2, it.Key().GetAbsolutePath()) && !visitedFiles.Contains(it.Key().GetAbsolutePath()))
        missingFiles.PushBack(it.Key().GetAbsolutePath());
      if (!it.Key().GetAbsolutePath().StartsWith_NoCase(sAbsolutePath2))
        break;
    }

    for (auto it = m_ReferencedFolders.LowerBound(sAbsolutePath2.GetView()); it.IsValid(); ++it)
    {
      if (xiiPathUtils::IsSubPath(sAbsolutePath2, it.Key().GetAbsolutePath()) && !visitedFolders.Contains(it.Key().GetAbsolutePath()))
        missingFolders.PushBack(it.Key().GetAbsolutePath());
      if (!it.Key().GetAbsolutePath().StartsWith_NoCase(sAbsolutePath2))
        break;
    }
  }

  for (xiiString& sFile : missingFiles)
  {
    xiiDataDirPath path(std::move(sFile), m_DataDirRoots, folder.GetDataDirIndex());
    HandleSingleFile(std::move(path), false);
  }

  // Delete sub-folders before parent folders.
  missingFolders.Sort([](const xiiString& lhs, const xiiString& rhs) -> bool { return xiiStringUtils::Compare(lhs, rhs) > 0; });
  for (xiiString& sFolder : missingFolders)
  {
    xiiDataDirPath path(std::move(sFolder), m_DataDirRoots, folder.GetDataDirIndex());
    HandleSingleFile(std::move(path), false);
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

xiiFileStatus xiiFileSystemModel::HandleSingleFile(xiiDataDirPath absolutePath, bool bRecurseIntoFolders)
{
  FILESYSTEM_PROFILE("HandleSingleFile");

  xiiFileStats    Stats;
  const xiiResult statCheck = xiiOSFile::GetFileStats(absolutePath, Stats);

#  if XII_ENABLED(XII_PLATFORM_WINDOWS)
  if (statCheck.Succeeded() && Stats.m_sName != xiiPathUtils::GetFileNameAndExtension(absolutePath))
  {
    // Casing has changed.
    xiiStringBuilder sCorrectCasingPath = absolutePath.GetAbsolutePath();
    sCorrectCasingPath.ChangeFileNameAndExtension(Stats.m_sName);
    xiiDataDirPath correctCasingPath(sCorrectCasingPath.GetView(), m_DataDirRoots, absolutePath.GetDataDirIndex());
    // Add new casing
    xiiFileStatus res = HandleSingleFile(std::move(correctCasingPath), Stats, bRecurseIntoFolders);
    // Remove old casing
    RemoveFileOrFolder(absolutePath, bRecurseIntoFolders);
    return res;
  }
#  endif

  if (statCheck.Failed())
  {
    RemoveFileOrFolder(absolutePath, bRecurseIntoFolders);
    return {};
  }

  return HandleSingleFile(std::move(absolutePath), Stats, bRecurseIntoFolders);
}


xiiFileStatus xiiFileSystemModel::HandleSingleFile(xiiDataDirPath absolutePath, const xiiFileStats& FileStat, bool bRecurseIntoFolders)
{
  FILESYSTEM_PROFILE("HandleSingleFile2");

  if (FileStat.m_bIsDirectory)
  {
    xiiFileStatus status;
    status.m_Status = xiiFileStatus::Status::Valid;

    bool bExisted = false;
    {
      XII_LOCK(m_FilesMutex);
      auto it    = m_ReferencedFolders.FindOrAdd(absolutePath, &bExisted);
      it.Value() = xiiFileStatus::Status::Valid;
    }

    if (!bExisted)
    {
      FireFolderChangedEvent(absolutePath, xiiFolderChangedEvent::Type::FolderAdded);
      if (bRecurseIntoFolders)
        CheckFolder(absolutePath);
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
      auto           it    = m_ReferencedFiles.FindOrAdd(absolutePath, &bExisted);
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
      FireFileChangedEvent(absolutePath, status, xiiFileChangedEvent::Type::FileAdded);
    }
    else if (bFileChanged)
    {
      FireFileChangedEvent(absolutePath, status, xiiFileChangedEvent::Type::FileChanged);
    }
    return status;
  }
}

void xiiFileSystemModel::RemoveFileOrFolder(const xiiDataDirPath& absolutePath, bool bRecurseIntoFolders)
{
  xiiFileStatus fileStatus;
  bool          bFileExisted   = false;
  bool          bFolderExisted = false;
  {
    XII_LOCK(m_FilesMutex);
    if (auto it = m_ReferencedFiles.Find(absolutePath); it.IsValid())
    {
      bFileExisted = true;
      fileStatus   = it.Value();
      m_ReferencedFiles.Remove(it);
    }
    if (auto it = m_ReferencedFolders.Find(absolutePath); it.IsValid())
    {
      bFolderExisted = true;
      m_ReferencedFolders.Remove(it);
    }
  }

  if (bFileExisted)
  {
    FireFileChangedEvent(absolutePath, fileStatus, xiiFileChangedEvent::Type::FileRemoved);
  }

  if (bFolderExisted)
  {
    if (bRecurseIntoFolders)
    {
      xiiSet<xiiDataDirPath> previouslyKnownFiles;
      {
        FILESYSTEM_PROFILE("FindReferencedFiles");
        XII_LOCK(m_FilesMutex);
        auto itlowerBound = m_ReferencedFiles.LowerBound(absolutePath);
        while (itlowerBound.IsValid())
        {
          if (xiiPathUtils::IsSubPath(absolutePath, itlowerBound.Key().GetAbsolutePath()))
          {
            previouslyKnownFiles.Insert(itlowerBound.Key());
          }
          // As we are using xiiCompareDataDirPath, entries of different casing interleave but we are only interested in the ones with matching casing so we skip the rest.
          if (!itlowerBound.Key().GetAbsolutePath().StartsWith_NoCase(absolutePath.GetAbsolutePath()))
          {
            break;
          }
          ++itlowerBound;
        }
      }
      {
        FILESYSTEM_PROFILE("HandleRemovedFiles");
        for (const xiiDataDirPath& file : previouslyKnownFiles)
        {
          RemoveFileOrFolder(file, false);
        }
      }
    }
    FireFolderChangedEvent(absolutePath, xiiFolderChangedEvent::Type::FolderRemoved);
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

void xiiFileSystemModel::FireFileChangedEvent(const xiiDataDirPath& file, xiiFileStatus fileStatus, xiiFileChangedEvent::Type type)
{
  // We queue up all requests on a thread and only return once the list is empty. The reason for this is that:
  // A: We don't want to allow recursive event calling as it creates limbo states in the model and hard to debug bugs.
  // B: If a user calls NotifyOfChange, the function should only return if the event and any indirect events that were triggered by the event handlers have been processed.

  xiiFileChangedEvent& e = g_PostponedFiles.ExpandAndGetRef();
  e.m_Path               = file;
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
    // Need to make a copy as new elements can be added and the array resized during broadcast.
    xiiFileChangedEvent tempEvent = std::move(g_PostponedFiles[i]);
    m_FileChangedEvents.Broadcast(tempEvent);
  }
  g_PostponedFiles.Clear();
}

void xiiFileSystemModel::FireFolderChangedEvent(const xiiDataDirPath& file, xiiFolderChangedEvent::Type type)
{
  // See comment in FireFileChangedEvent.
  xiiFolderChangedEvent& e = g_PostponedFolders.ExpandAndGetRef();
  e.m_Path                 = file;
  e.m_Type                 = type;

  if (g_bInFolderBroadcast)
  {
    return;
  }

  g_bInFolderBroadcast = true;
  XII_SCOPE_EXIT(g_bInFolderBroadcast = false);

  for (xiiUInt32 i = 0; i < g_PostponedFolders.GetCount(); i++)
  {
    // Need to make a copy as new elements can be added and the array resized during broadcast.
    xiiFolderChangedEvent tempEvent = std::move(g_PostponedFolders[i]);
    m_FolderChangedEvents.Broadcast(tempEvent);
  }
  g_PostponedFolders.Clear();
}

#endif
