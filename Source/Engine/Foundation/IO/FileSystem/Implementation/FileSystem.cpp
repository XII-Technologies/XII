#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/Implementation/StringIterator.h>
#include <Foundation/Strings/StringView.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Foundation, FileSystem)

  ON_CORESYSTEMS_STARTUP
  {
    xiiFileSystem::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiFileSystem::Shutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiFileSystem::FileSystemData* xiiFileSystem::s_pData = nullptr;
xiiString                      xiiFileSystem::s_sSdkRootDir;
xiiMap<xiiString, xiiString>   xiiFileSystem::s_SpecialDirectories;


void xiiFileSystem::RegisterDataDirectoryFactory(xiiDataDirFactory Factory, float fPriority /*= 0*/)
{
  XII_LOCK(s_pData->m_FsMutex);

  auto& data       = s_pData->m_DataDirFactories.ExpandAndGetRef();
  data.m_Factory   = Factory;
  data.m_fPriority = fPriority;
}

xiiEventSubscriptionID xiiFileSystem::RegisterEventHandler(xiiEvent<const FileEvent&>::Handler handler)
{
  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  return s_pData->m_Event.AddEventHandler(handler);
}

void xiiFileSystem::UnregisterEventHandler(xiiEvent<const FileEvent&>::Handler handler)
{
  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  s_pData->m_Event.RemoveEventHandler(handler);
}

void xiiFileSystem::UnregisterEventHandler(xiiEventSubscriptionID subscriptionId)
{
  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  s_pData->m_Event.RemoveEventHandler(subscriptionId);
}

void xiiFileSystem::CleanUpRootName(xiiStringBuilder& sRoot)
{
  // this cleaning might actually make the root name empty
  // e.g. ":" becomes ""
  // which is intended to support passing through of absolute paths
  // ie. mounting the empty dir "" under the root ":" will allow to write directly to files using absolute paths

  while (sRoot.StartsWith(":"))
    sRoot.Shrink(1, 0);

  while (sRoot.EndsWith("/"))
    sRoot.Shrink(0, 1);

  sRoot.ToUpper();
}

xiiResult xiiFileSystem::AddDataDirectory(const char* szDataDirectory, const char* szGroup, const char* szRootName, DataDirUsage Usage)
{
  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  XII_ASSERT_DEV(Usage != AllowWrites || !xiiStringUtils::IsNullOrEmpty(szRootName), "A data directory must have a non-empty, unique name to be mounted for write access");

  xiiStringBuilder sPath = szDataDirectory;
  sPath.MakeCleanPath();

  if (!sPath.IsEmpty() && !sPath.EndsWith("/"))
    sPath.Append("/");

  xiiStringBuilder sCleanRootName = szRootName;
  CleanUpRootName(sCleanRootName);

  XII_LOCK(s_pData->m_FsMutex);

  bool failed = false;
  if (FindDataDirectoryWithRoot(sCleanRootName) != nullptr)
  {
    xiiLog::Error("A data directory with root name '{0}' already exists.", sCleanRootName);
    failed = true;
  }

  if (!failed)
  {
    s_pData->m_DataDirFactories.Sort([](const auto& a, const auto& b) { return a.m_fPriority < b.m_fPriority; });

    // use the factory that was added last as the one with the highest priority -> allows to override already added factories
    for (xiiInt32 i = s_pData->m_DataDirFactories.GetCount() - 1; i >= 0; --i)
    {
      xiiDataDirectoryType* pDataDir = s_pData->m_DataDirFactories[i].m_Factory(sPath, szGroup, szRootName, Usage);

      if (pDataDir != nullptr)
      {
        DataDirectory dd;
        dd.m_Usage          = Usage;
        dd.m_pDataDirectory = pDataDir;
        dd.m_sRootName      = sCleanRootName;
        dd.m_sGroup         = szGroup;

        s_pData->m_DataDirectories.PushBack(dd);

        {
          // Broadcast that a data directory was added
          FileEvent fe;
          fe.m_EventType         = FileEventType::AddDataDirectorySucceeded;
          fe.m_szFileOrDirectory = sPath;
          fe.m_szOther           = sCleanRootName;
          fe.m_pDataDir          = pDataDir;
          s_pData->m_Event.Broadcast(fe);
        }

        return XII_SUCCESS;
      }
    }
  }

  {
    // Broadcast that adding a data directory failed
    FileEvent fe;
    fe.m_EventType         = FileEventType::AddDataDirectoryFailed;
    fe.m_szFileOrDirectory = sPath;
    fe.m_szOther           = sCleanRootName;
    s_pData->m_Event.Broadcast(fe);
  }

  xiiLog::Error("Adding Data Directory '{0}' failed.", xiiArgSensitive(szDataDirectory, "Path"));
  return XII_FAILURE;
}


bool xiiFileSystem::RemoveDataDirectory(const char* szRootName)
{
  xiiStringBuilder sCleanRootName = szRootName;
  CleanUpRootName(sCleanRootName);

  XII_LOCK(s_pData->m_FsMutex);

  for (xiiUInt32 i = 0; i < s_pData->m_DataDirectories.GetCount();)
  {
    if (s_pData->m_DataDirectories[i].m_sRootName == sCleanRootName)
    {
      {
        // Broadcast that a data directory is about to be removed
        FileEvent fe;
        fe.m_EventType         = FileEventType::RemoveDataDirectory;
        fe.m_szFileOrDirectory = s_pData->m_DataDirectories[i].m_pDataDirectory->GetDataDirectoryPath();
        fe.m_szOther           = s_pData->m_DataDirectories[i].m_sRootName;
        fe.m_pDataDir          = s_pData->m_DataDirectories[i].m_pDataDirectory;
        s_pData->m_Event.Broadcast(fe);
      }

      s_pData->m_DataDirectories[i].m_pDataDirectory->RemoveDataDirectory();
      s_pData->m_DataDirectories.RemoveAtAndCopy(i);

      return true;
    }
    else
      ++i;
  }

  return false;
}

xiiUInt32 xiiFileSystem::RemoveDataDirectoryGroup(const char* szGroup)
{
  if (s_pData == nullptr)
    return 0;

  XII_LOCK(s_pData->m_FsMutex);

  xiiUInt32 uiRemoved = 0;

  for (xiiUInt32 i = 0; i < s_pData->m_DataDirectories.GetCount();)
  {
    if (s_pData->m_DataDirectories[i].m_sGroup == szGroup)
    {
      {
        // Broadcast that a data directory is about to be removed
        FileEvent fe;
        fe.m_EventType         = FileEventType::RemoveDataDirectory;
        fe.m_szFileOrDirectory = s_pData->m_DataDirectories[i].m_pDataDirectory->GetDataDirectoryPath();
        fe.m_szOther           = s_pData->m_DataDirectories[i].m_sRootName;
        fe.m_pDataDir          = s_pData->m_DataDirectories[i].m_pDataDirectory;
        s_pData->m_Event.Broadcast(fe);
      }

      ++uiRemoved;

      s_pData->m_DataDirectories[i].m_pDataDirectory->RemoveDataDirectory();
      s_pData->m_DataDirectories.RemoveAtAndCopy(i);
    }
    else
      ++i;
  }

  return uiRemoved;
}

void xiiFileSystem::ClearAllDataDirectories()
{
  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  XII_LOCK(s_pData->m_FsMutex);

  for (xiiInt32 i = s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    {
      // Broadcast that a data directory is about to be removed
      FileEvent fe;
      fe.m_EventType         = FileEventType::RemoveDataDirectory;
      fe.m_szFileOrDirectory = s_pData->m_DataDirectories[i].m_pDataDirectory->GetDataDirectoryPath();
      fe.m_szOther           = s_pData->m_DataDirectories[i].m_sRootName;
      fe.m_pDataDir          = s_pData->m_DataDirectories[i].m_pDataDirectory;
      s_pData->m_Event.Broadcast(fe);
    }

    s_pData->m_DataDirectories[i].m_pDataDirectory->RemoveDataDirectory();
  }

  s_pData->m_DataDirectories.Clear();
}

xiiDataDirectoryType* xiiFileSystem::FindDataDirectoryWithRoot(const char* szRootName)
{
  if (xiiStringUtils::IsNullOrEmpty(szRootName))
    return nullptr;

  XII_LOCK(s_pData->m_FsMutex);

  for (const auto& dd : s_pData->m_DataDirectories)
  {
    if (dd.m_sRootName.IsEqual_NoCase(szRootName))
    {
      return dd.m_pDataDirectory;
    }
  }

  return nullptr;
}

xiiUInt32 xiiFileSystem::GetNumDataDirectories()
{
  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  return s_pData->m_DataDirectories.GetCount();
}

xiiDataDirectoryType* xiiFileSystem::GetDataDirectory(xiiUInt32 uiDataDirIndex)
{
  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  return s_pData->m_DataDirectories[uiDataDirIndex].m_pDataDirectory;
}

const char* xiiFileSystem::GetDataDirRelativePath(const char* szPath, xiiUInt32 uiDataDir)
{
  XII_LOCK(s_pData->m_FsMutex);

  // if an absolute path is given, this will check whether the absolute path would fall into this data directory
  // if yes, the prefix path is removed and then only the relative path is given to the data directory type
  // otherwise the data directory would prepend its own path and thus create an invalid path to work with

  // first check the redirected directory
  const xiiString128& sRedDirPath = s_pData->m_DataDirectories[uiDataDir].m_pDataDirectory->GetRedirectedDataDirectoryPath();

  if (!sRedDirPath.IsEmpty() && xiiStringUtils::StartsWith_NoCase(szPath, sRedDirPath))
  {
    const char* szRelPath = &szPath[sRedDirPath.GetElementCount()];

    // if the relative path still starts with a path-separator, skip it
    if (xiiPathUtils::IsPathSeparator(*szRelPath))
      ++szRelPath;

    return szRelPath;
  }

  // then check the original mount path
  const xiiString128& sDirPath = s_pData->m_DataDirectories[uiDataDir].m_pDataDirectory->GetDataDirectoryPath();

  // If the data dir is empty we return the paths as is or the code below would remove the '/' in front of an
  // absolute path.
  if (!sDirPath.IsEmpty() && xiiStringUtils::StartsWith_NoCase(szPath, sDirPath))
  {
    const char* szRelPath = &szPath[sDirPath.GetElementCount()];

    // if the relative path still starts with a path-separator, skip it
    if (xiiPathUtils::IsPathSeparator(*szRelPath))
      ++szRelPath;

    return szRelPath;
  }

  return szPath;
}


xiiFileSystem::DataDirectory* xiiFileSystem::GetDataDirForRoot(const xiiString& sRoot)
{
  XII_LOCK(s_pData->m_FsMutex);

  for (xiiInt32 i = (xiiInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    if (s_pData->m_DataDirectories[i].m_sRootName == sRoot)
      return &s_pData->m_DataDirectories[i];
  }

  return nullptr;
}


void xiiFileSystem::DeleteFile(const char* szFile)
{
  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  if (xiiPathUtils::IsAbsolutePath(szFile))
  {
    xiiOSFile::DeleteFile(szFile).IgnoreResult();
    return;
  }

  xiiString sRootName;
  szFile = ExtractRootName(szFile, sRootName);

  XII_ASSERT_DEV(!sRootName.IsEmpty(), "Files can only be deleted with a rooted path name.");

  if (sRootName.IsEmpty())
    return;

  XII_LOCK(s_pData->m_FsMutex);

  for (xiiInt32 i = (xiiInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    // do not delete data from directories that are mounted as read only
    if (s_pData->m_DataDirectories[i].m_Usage != AllowWrites)
      continue;

    if (s_pData->m_DataDirectories[i].m_sRootName != sRootName)
      continue;

    const char* szRelPath = GetDataDirRelativePath(szFile, i);

    {
      // Broadcast that a file is about to be deleted
      // This can be used to check out files or mark them as deleted in a revision control system
      FileEvent fe;
      fe.m_EventType         = FileEventType::DeleteFile;
      fe.m_szFileOrDirectory = szRelPath;
      fe.m_pDataDir          = s_pData->m_DataDirectories[i].m_pDataDirectory;
      fe.m_szOther           = sRootName;
      s_pData->m_Event.Broadcast(fe);
    }

    s_pData->m_DataDirectories[i].m_pDataDirectory->DeleteFile(szRelPath);
  }
}

bool xiiFileSystem::ExistsFile(const char* szFile)
{
  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  xiiString sRootName;
  szFile = ExtractRootName(szFile, sRootName);

  const bool bOneSpecificDataDir = !sRootName.IsEmpty();

  XII_LOCK(s_pData->m_FsMutex);

  for (xiiInt32 i = (xiiInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    if (!sRootName.IsEmpty() && s_pData->m_DataDirectories[i].m_sRootName != sRootName)
      continue;

    const char* szRelPath = GetDataDirRelativePath(szFile, i);

    if (s_pData->m_DataDirectories[i].m_pDataDirectory->ExistsFile(szRelPath, bOneSpecificDataDir))
      return true;
  }

  return false;
}


xiiResult xiiFileSystem::GetFileStats(const char* szFileOrFolder, xiiFileStats& out_Stats)
{
  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  XII_LOCK(s_pData->m_FsMutex);

  xiiString sRootName;
  szFileOrFolder = ExtractRootName(szFileOrFolder, sRootName);

  const bool bOneSpecificDataDir = !sRootName.IsEmpty();

  for (xiiInt32 i = (xiiInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    if (!sRootName.IsEmpty() && s_pData->m_DataDirectories[i].m_sRootName != sRootName)
      continue;

    const char* szRelPath = GetDataDirRelativePath(szFileOrFolder, i);

    if (s_pData->m_DataDirectories[i].m_pDataDirectory->GetFileStats(szRelPath, bOneSpecificDataDir, out_Stats).Succeeded())
      return XII_SUCCESS;
  }

  return XII_FAILURE;
}

const char* xiiFileSystem::ExtractRootName(const char* szPath, xiiString& rootName)
{
  rootName.Clear();

  if (!xiiStringUtils::StartsWith(szPath, ":"))
    return szPath;

  xiiStringBuilder    sCur;
  const xiiStringView view(szPath);
  xiiStringIterator   it = view.GetIteratorFront();
  ++it;

  while (it.IsValid() && (it.GetCharacter() != '/'))
  {
    sCur.Append(it.GetCharacter());
    ++it;
  }

  XII_ASSERT_DEV(it.IsValid(), "Cannot parse the path \"{0}\". The data-dir root name starts with a ':' but does not end with '/'.", szPath);

  sCur.ToUpper();
  rootName = sCur;
  ++it;

  return it.GetData(); // return the string after the data-dir filter declaration
}

xiiDataDirectoryReader* xiiFileSystem::GetFileReader(const char* szFile, xiiFileShareMode::Enum FileShareMode, bool bAllowFileEvents)
{
  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  if (xiiStringUtils::IsNullOrEmpty(szFile))
    return nullptr;

  XII_LOCK(s_pData->m_FsMutex);

  xiiString sRootName;
  szFile = ExtractRootName(szFile, sRootName);

  // clean up the path to get rid of ".." etc.
  xiiStringBuilder sPath = szFile;
  sPath.MakeCleanPath();

  const bool bOneSpecificDataDir = !sRootName.IsEmpty();

  // the last added data directory has the highest priority
  for (xiiInt32 i = (xiiInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    // if a root is used, ignore all directories that do not have the same root name
    if (bOneSpecificDataDir && s_pData->m_DataDirectories[i].m_sRootName != sRootName)
      continue;

    const char* szRelPath = GetDataDirRelativePath(sPath, i);

    if (bAllowFileEvents)
    {
      // Broadcast that we now try to open this file
      // Could be useful to check this file out before it is accessed
      FileEvent fe;
      fe.m_EventType         = FileEventType::OpenFileAttempt;
      fe.m_szFileOrDirectory = szRelPath;
      fe.m_szOther           = sRootName;
      fe.m_pDataDir          = s_pData->m_DataDirectories[i].m_pDataDirectory;
      s_pData->m_Event.Broadcast(fe);
    }

    // Let the data directory try to open the file.
    xiiDataDirectoryReader* pReader = s_pData->m_DataDirectories[i].m_pDataDirectory->OpenFileToRead(szRelPath, FileShareMode, bOneSpecificDataDir);

    if (bAllowFileEvents && pReader != nullptr)
    {
      // Broadcast that this file has been opened.
      FileEvent fe;
      fe.m_EventType         = FileEventType::OpenFileSucceeded;
      fe.m_szFileOrDirectory = szRelPath;
      fe.m_szOther           = sRootName;
      fe.m_pDataDir          = s_pData->m_DataDirectories[i].m_pDataDirectory;
      s_pData->m_Event.Broadcast(fe);

      return pReader;
    }
  }

  if (bAllowFileEvents)
  {
    // Broadcast that opening this file failed.
    FileEvent fe;
    fe.m_EventType         = FileEventType::OpenFileFailed;
    fe.m_szFileOrDirectory = sPath;
    s_pData->m_Event.Broadcast(fe);
  }

  return nullptr;
}

xiiDataDirectoryWriter* xiiFileSystem::GetFileWriter(const char* szFile, xiiFileShareMode::Enum FileShareMode, bool bAllowFileEvents)
{
  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  if (xiiStringUtils::IsNullOrEmpty(szFile))
    return nullptr;

  XII_LOCK(s_pData->m_FsMutex);

  xiiString sRootName;

  if (!xiiPathUtils::IsAbsolutePath(szFile))
  {
    XII_ASSERT_DEV(xiiStringUtils::StartsWith(szFile, ":"),
                   "Only native absolute paths or rooted paths (starting with a colon and then the data dir root name) are allowed for "
                   "writing to files. This path is neither: '{0}'",
                   szFile);
    szFile = ExtractRootName(szFile, sRootName);
  }

  // clean up the path to get rid of ".." etc.
  xiiStringBuilder sPath = szFile;
  sPath.MakeCleanPath();

  // the last added data directory has the highest priority
  for (xiiInt32 i = (xiiInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    if (s_pData->m_DataDirectories[i].m_Usage != AllowWrites)
      continue;

    // ignore all directories that have not the category that is currently requested
    if (s_pData->m_DataDirectories[i].m_sRootName != sRootName)
      continue;

    const char* szRelPath = GetDataDirRelativePath(sPath, i);

    if (bAllowFileEvents)
    {
      // Broadcast that we now try to open this file
      // Could be useful to check this file out before it is accessed
      FileEvent fe;
      fe.m_EventType         = FileEventType::CreateFileAttempt;
      fe.m_szFileOrDirectory = szRelPath;
      fe.m_szOther           = sRootName;
      fe.m_pDataDir          = s_pData->m_DataDirectories[i].m_pDataDirectory;
      s_pData->m_Event.Broadcast(fe);
    }

    xiiDataDirectoryWriter* pWriter = s_pData->m_DataDirectories[i].m_pDataDirectory->OpenFileToWrite(szRelPath, FileShareMode);

    if (bAllowFileEvents && pWriter != nullptr)
    {
      // Broadcast that this file has been created.
      FileEvent fe;
      fe.m_EventType         = FileEventType::CreateFileSucceeded;
      fe.m_szFileOrDirectory = szRelPath;
      fe.m_szOther           = sRootName;
      fe.m_pDataDir          = s_pData->m_DataDirectories[i].m_pDataDirectory;
      s_pData->m_Event.Broadcast(fe);

      return pWriter;
    }
  }

  if (bAllowFileEvents)
  {
    // Broadcast that creating this file failed.
    FileEvent fe;
    fe.m_EventType         = FileEventType::CreateFileFailed;
    fe.m_szFileOrDirectory = sPath;
    s_pData->m_Event.Broadcast(fe);
  }

  return nullptr;
}

xiiResult xiiFileSystem::ResolvePath(const char* szPath, xiiStringBuilder* out_sAbsolutePath, xiiStringBuilder* out_sDataDirRelativePath, xiiDataDirectoryType** out_ppDataDir /*= nullptr*/)
{
  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  XII_LOCK(s_pData->m_FsMutex);

  xiiStringBuilder absPath, relPath;

  if (xiiStringUtils::StartsWith(szPath, ":"))
  {
    // writing is only allowed using rooted paths
    xiiString sRootName;
    ExtractRootName(szPath, sRootName);

    DataDirectory* pDataDir = GetDataDirForRoot(sRootName);

    if (pDataDir == nullptr)
      return XII_FAILURE;

    if (out_ppDataDir != nullptr)
      *out_ppDataDir = pDataDir->m_pDataDirectory;

    relPath = &szPath[sRootName.GetElementCount() + 2];

    absPath = pDataDir->m_pDataDirectory->GetRedirectedDataDirectoryPath(); /// \todo We might also need the none-redirected path as an output
    absPath.AppendPath(relPath);
  }
  else if (xiiPathUtils::IsAbsolutePath(szPath))
  {
    absPath = szPath;
    absPath.MakeCleanPath();

    for (xiiUInt32 dd = s_pData->m_DataDirectories.GetCount(); dd > 0; --dd)
    {
      auto& dir = s_pData->m_DataDirectories[dd - 1];

      if (xiiPathUtils::IsSubPath(dir.m_pDataDirectory->GetRedirectedDataDirectoryPath(), absPath))
      {
        if (out_sAbsolutePath)
          *out_sAbsolutePath = absPath;

        if (out_sDataDirRelativePath)
        {
          *out_sDataDirRelativePath = absPath;
          out_sDataDirRelativePath->MakeRelativeTo(dir.m_pDataDirectory->GetRedirectedDataDirectoryPath()).IgnoreResult();
        }

        if (out_ppDataDir)
          *out_ppDataDir = dir.m_pDataDirectory;

        return XII_SUCCESS;
      }
    }

    return XII_FAILURE;
  }
  else
  {
    // try to get a reader -> if we get one, the file does indeed exist
    xiiDataDirectoryReader* pReader = xiiFileSystem::GetFileReader(szPath, xiiFileShareMode::SharedReads, true);

    if (!pReader)
      return XII_FAILURE;

    if (out_ppDataDir != nullptr)
      *out_ppDataDir = pReader->GetDataDirectory();

    relPath = pReader->GetFilePath();

    absPath = pReader->GetDataDirectory()->GetRedirectedDataDirectoryPath(); /// \todo We might also need the none-redirected path as an output
    absPath.AppendPath(relPath);

    pReader->Close();
  }

  if (out_sAbsolutePath)
    *out_sAbsolutePath = absPath;

  if (out_sDataDirRelativePath)
    *out_sDataDirRelativePath = relPath;

  return XII_SUCCESS;
}

xiiResult xiiFileSystem::FindFolderWithSubPath(xiiStringBuilder& result, const char* szStartDirectory, const char* szSubPath, const char* szRedirectionFileName /*= nullptr*/)
{
  xiiStringBuilder sStartDirAbs = szStartDirectory;
  sStartDirAbs.MakeCleanPath();

  // in this case the given path and the absolute path are different
  // but we want to return the same path format as is given
  // ie. if we get ":MyRoot\Bla" with "MyRoot" pointing to "C:\Game", then the result should be
  // ":MyRoot\blub", rather than "C:\Game\blub"
  if (sStartDirAbs.StartsWith(":"))
  {
    xiiStringBuilder abs;
    if (ResolvePath(sStartDirAbs, &abs, nullptr).Failed())
    {
      result.Clear();
      return XII_FAILURE;
    }

    sStartDirAbs = abs;
  }

  result = szStartDirectory;
  result.MakeCleanPath();

  xiiStringBuilder FullPath, sRedirection;

  while (!result.IsEmpty())
  {
    sRedirection.Clear();

    if (!xiiStringUtils::IsNullOrEmpty(szRedirectionFileName))
    {
      FullPath = sStartDirAbs;
      FullPath.AppendPath(szRedirectionFileName);

      xiiOSFile f;
      if (f.Open(FullPath, xiiFileOpenMode::Read).Succeeded())
      {
        xiiDataBuffer db;
        f.ReadAll(db);
        sRedirection.Set(xiiStringView((const char*)db.GetData(), db.GetCount()));
      }
    }

    // first try with the redirection
    if (!sRedirection.IsEmpty())
    {
      FullPath = sStartDirAbs;
      FullPath.AppendPath(sRedirection);
      FullPath.AppendPath(szSubPath);
      FullPath.MakeCleanPath();

      if (xiiOSFile::ExistsDirectory(FullPath) || xiiOSFile::ExistsFile(FullPath))
      {
        result.AppendPath(sRedirection);
        result.MakeCleanPath();
        return XII_SUCCESS;
      }
    }

    // then try without the redirection
    FullPath = sStartDirAbs;
    FullPath.AppendPath(szSubPath);
    FullPath.MakeCleanPath();

    if (xiiOSFile::ExistsDirectory(FullPath) || xiiOSFile::ExistsFile(FullPath))
    {
      return XII_SUCCESS;
    }

    result.PathParentDirectory();
    sStartDirAbs.PathParentDirectory();
  }

  return XII_FAILURE;
}

bool xiiFileSystem::ResolveAssetRedirection(const char* szPathOrAssetGuid, xiiStringBuilder& out_sRedirection)
{
  XII_LOCK(s_pData->m_FsMutex);

  for (auto& dd : s_pData->m_DataDirectories)
  {
    if (dd.m_pDataDirectory->ResolveAssetRedirection(szPathOrAssetGuid, out_sRedirection))
      return true;
  }

  out_sRedirection = szPathOrAssetGuid;
  return false;
}

void xiiFileSystem::ReloadAllExternalDataDirectoryConfigs()
{
  XII_LOG_BLOCK("ReloadAllExternalDataDirectoryConfigs");

  XII_LOCK(s_pData->m_FsMutex);

  for (auto& dd : s_pData->m_DataDirectories)
  {
    dd.m_pDataDirectory->ReloadExternalConfigs();
  }
}

void xiiFileSystem::Startup()
{
  s_pData = XII_DEFAULT_NEW(FileSystemData);
}

void xiiFileSystem::Shutdown()
{
  {
    XII_LOCK(s_pData->m_FsMutex);

    s_pData->m_DataDirFactories.Clear();

    ClearAllDataDirectories();
  }

  XII_DEFAULT_DELETE(s_pData);
}

xiiResult xiiFileSystem::DetectSdkRootDirectory(const char* szExpectedSubFolder /*= "Data/Base"*/)
{
  if (!s_sSdkRootDir.IsEmpty())
    return XII_SUCCESS;

  xiiStringBuilder sdkRoot;

#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
  // Probably this is what needs to be done on all mobile platforms as well
  sdkRoot = xiiOSFile::GetApplicationDirectory();
#elif XII_ENABLED(XII_PLATFORM_ANDROID)
  sdkRoot = xiiOSFile::GetApplicationDirectory();
#else
  if (xiiFileSystem::FindFolderWithSubPath(sdkRoot, xiiOSFile::GetApplicationDirectory(), szExpectedSubFolder, "xiiSdkRoot.txt").Failed())
  {
    xiiLog::Error("Could not find SDK root. Application dir is '{0}'. Searched for parent with '{1}' sub-folder.", xiiOSFile::GetApplicationDirectory(), szExpectedSubFolder);
    return XII_FAILURE;
  }
#endif

  xiiFileSystem::SetSdkRootDirectory(sdkRoot);
  return XII_SUCCESS;
}

void xiiFileSystem::SetSdkRootDirectory(const char* szSdkDir)
{
  xiiStringBuilder s = szSdkDir;
  s.MakeCleanPath();

  s_sSdkRootDir = s;
}

const char* xiiFileSystem::GetSdkRootDirectory()
{
  XII_ASSERT_DEV(!s_sSdkRootDir.IsEmpty(), "The project directory has not been set through 'xiiFileSystem::SetSdkRootDirectory'.");
  return s_sSdkRootDir.GetData();
}

void xiiFileSystem::SetSpecialDirectory(const char* szName, const char* szReplacement)
{
  xiiStringBuilder tmp = szName;
  tmp.ToLower();

  if (szReplacement == nullptr)
  {
    s_SpecialDirectories.Remove(tmp);
  }
  else
  {
    s_SpecialDirectories[tmp] = szReplacement;
  }
}

xiiResult xiiFileSystem::ResolveSpecialDirectory(const char* szDirectory, xiiStringBuilder& out_Path)
{
  if (xiiStringUtils::IsNullOrEmpty(szDirectory) || szDirectory[0] != '>')
  {
    out_Path = szDirectory;
    return XII_SUCCESS;
  }

  const char* szStart = szDirectory + 1; // skip the '>'

  const char* szEnd = xiiStringUtils::FindSubString(szStart, "/");

  if (szEnd == nullptr)
    szEnd = szStart + xiiStringUtils::GetStringElementCount(szStart);

  xiiStringBuilder sName;
  sName.SetSubString_FromTo(szStart, szEnd);
  sName.ToLower();

  const auto it = s_SpecialDirectories.Find(sName);
  if (it.IsValid())
  {
    out_Path = it.Value();
    out_Path.AppendPath(szEnd); // szEnd might be on \0 or a slash
    out_Path.MakeCleanPath();
    return XII_SUCCESS;
  }

  if (sName == "sdk")
  {
    out_Path = GetSdkRootDirectory();
    out_Path.AppendPath(&szDirectory[4]);
    out_Path.MakeCleanPath();
    return XII_SUCCESS;
  }

  if (sName == "user")
  {
    out_Path = xiiOSFile::GetUserDataFolder();
    out_Path.AppendPath(&szDirectory[5]);
    out_Path.MakeCleanPath();
    return XII_SUCCESS;
  }

  if (sName == "temp")
  {
    out_Path = xiiOSFile::GetTempDataFolder();
    out_Path.AppendPath(&szDirectory[5]);
    out_Path.MakeCleanPath();
    return XII_SUCCESS;
  }

  if (sName == "appdir")
  {
    out_Path = xiiOSFile::GetApplicationDirectory();
    out_Path.AppendPath(&szDirectory[7]);
    out_Path.MakeCleanPath();
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}


xiiMutex& xiiFileSystem::GetMutex()
{
  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  return s_pData->m_FsMutex;
}

#if XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS)

void xiiFileSystem::StartSearch(xiiFileSystemIterator& iterator, const char* szSearchTerm, xiiBitflags<xiiFileSystemIteratorFlags> flags /*= xiiFileSystemIteratorFlags::Default*/)
{
  XII_LOCK(s_pData->m_FsMutex);

  xiiHybridArray<xiiString, 16> folders;
  xiiStringBuilder              sDdPath;

  for (const auto& dd : s_pData->m_DataDirectories)
  {
    sDdPath = dd.m_pDataDirectory->GetRedirectedDataDirectoryPath();

    if (ResolvePath(sDdPath, &sDdPath, nullptr).Failed())
      continue;

    if (sDdPath.IsEmpty() || !xiiOSFile::ExistsDirectory(sDdPath))
      continue;


    folders.PushBack(sDdPath);
  }

  iterator.StartMultiFolderSearch(folders, szSearchTerm, flags);
}

#endif

xiiResult xiiFileSystem::CreateDirectoryStructure(const char* szPath)
{
  xiiStringBuilder sRedir;
  XII_SUCCEED_OR_RETURN(ResolveSpecialDirectory(szPath, sRedir));

  return xiiOSFile::CreateDirectoryStructure(sRedir);
}

XII_STATICLINK_FILE(Foundation, Foundation_IO_FileSystem_Implementation_FileSystem);
