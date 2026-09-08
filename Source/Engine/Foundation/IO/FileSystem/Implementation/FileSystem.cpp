/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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

void xiiFileSystem::RegisterDataDirectoryFactory(xiiDataDirFactory factory, float fPriority /*= 0*/)
{
  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  XII_LOCK(s_pData->m_FsMutex);

  auto& data       = s_pData->m_DataDirFactories.ExpandAndGetRef();
  data.m_Factory   = factory;
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

xiiResult xiiFileSystem::AddDataDirectory(xiiStringView sDataDirectory, xiiStringView sGroup, xiiStringView sRootName, xiiDataDirUsage usage)
{
  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  XII_ASSERT_DEV(usage != xiiDataDirUsage::AllowWrites || !sRootName.IsEmpty(), "A data directory must have a non-empty, unique name to be mounted for write access");

  xiiStringBuilder sPath = sDataDirectory;
  sPath.MakeCleanPath();

  if (!sPath.IsEmpty() && !sPath.EndsWith("/"))
    sPath.Append("/");

  xiiStringBuilder sCleanRootName = sRootName;
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
      xiiDataDirectoryType* pDataDir = s_pData->m_DataDirFactories[i].m_Factory(sPath, sGroup, sRootName, usage);

      if (pDataDir != nullptr)
      {
        xiiDataDirectoryInfo dd;
        dd.m_Usage        = usage;
        dd.m_pDataDirType = pDataDir;
        dd.m_sRootName    = sCleanRootName;
        dd.m_sGroup       = sGroup;

        s_pData->m_DataDirectories.PushBack(dd);

        {
          // Broadcast that a data directory was added
          FileEvent fe;
          fe.m_EventType        = FileEventType::AddDataDirectorySucceeded;
          fe.m_sFileOrDirectory = sPath;
          fe.m_sOther           = sCleanRootName;
          fe.m_pDataDir         = pDataDir;
          s_pData->m_Event.Broadcast(fe);
        }

        xiiLog::Dev("Added Data Directory '{}' -> '{}'.", sRootName, sDataDirectory);
        return XII_SUCCESS;
      }
    }
  }

  {
    // Broadcast that adding a data directory failed
    FileEvent fe;
    fe.m_EventType        = FileEventType::AddDataDirectoryFailed;
    fe.m_sFileOrDirectory = sPath;
    fe.m_sOther           = sCleanRootName;
    s_pData->m_Event.Broadcast(fe);
  }

  xiiLog::Error("Adding Data Directory '{0}' failed.", xiiArgSensitive(sDataDirectory, "Path"));
  return XII_FAILURE;
}


bool xiiFileSystem::RemoveDataDirectory(xiiStringView sRootName)
{
  xiiStringBuilder sCleanRootName = sRootName;
  CleanUpRootName(sCleanRootName);

  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  XII_LOCK(s_pData->m_FsMutex);

  for (xiiUInt32 i = 0; i < s_pData->m_DataDirectories.GetCount();)
  {
    const auto& directory = s_pData->m_DataDirectories[i];

    if (directory.m_sRootName == sCleanRootName)
    {
      {
        // Broadcast that a data directory is about to be removed
        FileEvent fe;
        fe.m_EventType        = FileEventType::RemoveDataDirectory;
        fe.m_sFileOrDirectory = directory.m_pDataDirType->GetDataDirectoryPath();
        fe.m_sOther           = directory.m_sRootName;
        fe.m_pDataDir         = directory.m_pDataDirType;
        s_pData->m_Event.Broadcast(fe);
      }

      directory.m_pDataDirType->RemoveDataDirectory();
      s_pData->m_DataDirectories.RemoveAtAndCopy(i);

      return true;
    }
    else
      ++i;
  }

  return false;
}

xiiUInt32 xiiFileSystem::RemoveDataDirectoryGroup(xiiStringView sGroup)
{
  if (s_pData == nullptr)
    return 0;

  XII_LOCK(s_pData->m_FsMutex);

  xiiUInt32 uiRemoved = 0;

  for (xiiUInt32 i = 0; i < s_pData->m_DataDirectories.GetCount();)
  {
    if (s_pData->m_DataDirectories[i].m_sGroup == sGroup)
    {
      {
        // Broadcast that a data directory is about to be removed
        FileEvent fe;
        fe.m_EventType        = FileEventType::RemoveDataDirectory;
        fe.m_sFileOrDirectory = s_pData->m_DataDirectories[i].m_pDataDirType->GetDataDirectoryPath();
        fe.m_sOther           = s_pData->m_DataDirectories[i].m_sRootName;
        fe.m_pDataDir         = s_pData->m_DataDirectories[i].m_pDataDirType;
        s_pData->m_Event.Broadcast(fe);
      }

      ++uiRemoved;

      s_pData->m_DataDirectories[i].m_pDataDirType->RemoveDataDirectory();
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
      fe.m_EventType        = FileEventType::RemoveDataDirectory;
      fe.m_sFileOrDirectory = s_pData->m_DataDirectories[i].m_pDataDirType->GetDataDirectoryPath();
      fe.m_sOther           = s_pData->m_DataDirectories[i].m_sRootName;
      fe.m_pDataDir         = s_pData->m_DataDirectories[i].m_pDataDirType;
      s_pData->m_Event.Broadcast(fe);
    }

    s_pData->m_DataDirectories[i].m_pDataDirType->RemoveDataDirectory();
  }

  s_pData->m_DataDirectories.Clear();
}

const xiiDataDirectoryInfo* xiiFileSystem::FindDataDirectoryWithRoot(xiiStringView sRootName)
{
  if (sRootName.IsEmpty())
    return nullptr;

  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  XII_LOCK(s_pData->m_FsMutex);

  for (const auto& dd : s_pData->m_DataDirectories)
  {
    if (dd.m_sRootName.IsEqual_NoCase(sRootName))
    {
      return &dd;
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

  return s_pData->m_DataDirectories[uiDataDirIndex].m_pDataDirType;
}

const xiiDataDirectoryInfo& xiiFileSystem::GetDataDirectoryInfo(xiiUInt32 uiDataDirIndex)
{
  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  return s_pData->m_DataDirectories[uiDataDirIndex];
}

xiiStringView xiiFileSystem::GetDataDirRelativePath(xiiStringView sPath, xiiUInt32 uiDataDir)
{
  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  XII_LOCK(s_pData->m_FsMutex);

  // if an absolute path is given, this will check whether the absolute path would fall into this data directory
  // if yes, the prefix path is removed and then only the relative path is given to the data directory type
  // otherwise the data directory would prepend its own path and thus create an invalid path to work with

  // first check the redirected directory
  const xiiString128& sRedDirPath = s_pData->m_DataDirectories[uiDataDir].m_pDataDirType->GetRedirectedDataDirectoryPath();

  if (!sRedDirPath.IsEmpty() && sPath.StartsWith_NoCase(sRedDirPath))
  {
    xiiStringView sRelPath(sPath.GetStartPointer() + sRedDirPath.GetElementCount(), sPath.GetEndPointer());

    // if the relative path still starts with a path-separator, skip it
    if (xiiPathUtils::IsPathSeparator(sRelPath.GetCharacter()))
    {
      sRelPath.ChopAwayFirstCharacterUtf8();
    }

    return sRelPath;
  }

  // then check the original mount path
  const xiiString128& sDirPath = s_pData->m_DataDirectories[uiDataDir].m_pDataDirType->GetDataDirectoryPath();

  // If the data dir is empty we return the paths as is or the code below would remove the '/' in front of an
  // absolute path.
  if (!sDirPath.IsEmpty() && sPath.StartsWith_NoCase(sDirPath))
  {
    xiiStringView sRelPath(sPath.GetStartPointer() + sDirPath.GetElementCount(), sPath.GetEndPointer());

    // if the relative path still starts with a path-separator, skip it
    if (xiiPathUtils::IsPathSeparator(sRelPath.GetCharacter()))
    {
      sRelPath.ChopAwayFirstCharacterUtf8();
    }

    return sRelPath;
  }

  return sPath;
}


xiiDataDirectoryInfo* xiiFileSystem::GetDataDirForRoot(const xiiString& sRoot)
{
  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  XII_LOCK(s_pData->m_FsMutex);

  for (xiiInt32 i = (xiiInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    if (s_pData->m_DataDirectories[i].m_sRootName == sRoot)
      return &s_pData->m_DataDirectories[i];
  }

  return nullptr;
}

void xiiFileSystem::DeleteFile(xiiStringView sFile)
{
  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  if (xiiPathUtils::IsAbsolutePath(sFile))
  {
    xiiOSFile::DeleteFile(sFile).IgnoreResult();
    return;
  }

  xiiString sRootName;
  sFile = ExtractRootName(sFile, sRootName);

  XII_ASSERT_DEV(!sRootName.IsEmpty(), "Files can only be deleted with a rooted path name.");

  if (sRootName.IsEmpty())
    return;

  XII_LOCK(s_pData->m_FsMutex);

  for (xiiInt32 i = (xiiInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    // do not delete data from directories that are mounted as read only
    if (s_pData->m_DataDirectories[i].m_Usage != xiiDataDirUsage::AllowWrites)
      continue;

    if (s_pData->m_DataDirectories[i].m_sRootName != sRootName)
      continue;

    xiiStringView sRelPath = GetDataDirRelativePath(sFile, i);

    {
      // Broadcast that a file is about to be deleted
      // This can be used to check out files or mark them as deleted in a revision control system
      FileEvent fe;
      fe.m_EventType        = FileEventType::DeleteFile;
      fe.m_sFileOrDirectory = sRelPath;
      fe.m_pDataDir         = s_pData->m_DataDirectories[i].m_pDataDirType;
      fe.m_sOther           = sRootName;
      s_pData->m_Event.Broadcast(fe);
    }

    s_pData->m_DataDirectories[i].m_pDataDirType->DeleteFile(sRelPath);
  }
}

bool xiiFileSystem::ExistsFile(xiiStringView sFile)
{
  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  xiiString sRootName;
  sFile = ExtractRootName(sFile, sRootName);

  const bool bOneSpecificDataDir = !sRootName.IsEmpty();

  XII_LOCK(s_pData->m_FsMutex);

  for (xiiInt32 i = (xiiInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    if (!sRootName.IsEmpty() && s_pData->m_DataDirectories[i].m_sRootName != sRootName)
      continue;

    xiiStringView sRelPath = GetDataDirRelativePath(sFile, i);

    if (s_pData->m_DataDirectories[i].m_pDataDirType->ExistsFile(sRelPath, bOneSpecificDataDir))
      return true;
  }

  return false;
}


xiiResult xiiFileSystem::GetFileStats(xiiStringView sFileOrFolder, xiiFileStats& out_stats)
{
  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  XII_LOCK(s_pData->m_FsMutex);

  if (sFileOrFolder.IsEmpty())
    return XII_FAILURE;

  xiiString sRootName;
  sFileOrFolder = ExtractRootName(sFileOrFolder, sRootName);

  const bool bOneSpecificDataDir = !sRootName.IsEmpty();

  for (xiiInt32 i = (xiiInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    if (!sRootName.IsEmpty() && s_pData->m_DataDirectories[i].m_sRootName != sRootName)
      continue;

    xiiStringView sRelPath = GetDataDirRelativePath(sFileOrFolder, i);

    if (s_pData->m_DataDirectories[i].m_pDataDirType->GetFileStats(sRelPath, bOneSpecificDataDir, out_stats).Succeeded())
      return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiStringView xiiFileSystem::ExtractRootName(xiiStringView sPath, xiiString& rootName)
{
  xiiStringView root, path;
  xiiPathUtils::GetRootedPathParts(sPath, root, path);

  xiiStringBuilder rootUpr = root;
  rootUpr.ToUpper();
  rootName = rootUpr;
  return path;
}

xiiDataDirectoryReader* xiiFileSystem::GetFileReader(xiiStringView sFile, xiiFileShareMode::Enum FileShareMode, bool bAllowFileEvents)
{
  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  if (sFile.IsEmpty())
    return nullptr;

  XII_LOCK(s_pData->m_FsMutex);

  xiiString sRootName;
  sFile = ExtractRootName(sFile, sRootName);

  // clean up the path to get rid of ".." etc.
  xiiStringBuilder sPath = sFile;
  sPath.MakeCleanPath();

  const bool bOneSpecificDataDir = !sRootName.IsEmpty();

  // the last added data directory has the highest priority
  for (xiiInt32 i = (xiiInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    // if a root is used, ignore all directories that do not have the same root name
    if (bOneSpecificDataDir && s_pData->m_DataDirectories[i].m_sRootName != sRootName)
      continue;

    xiiStringView sRelPath = GetDataDirRelativePath(sPath, i);

    if (bAllowFileEvents)
    {
      // Broadcast that we now try to open this file
      // Could be useful to check this file out before it is accessed
      FileEvent fe;
      fe.m_EventType        = FileEventType::OpenFileAttempt;
      fe.m_sFileOrDirectory = sRelPath;
      fe.m_sOther           = sRootName;
      fe.m_pDataDir         = s_pData->m_DataDirectories[i].m_pDataDirType;
      s_pData->m_Event.Broadcast(fe);
    }

    // Let the data directory try to open the file.
    xiiDataDirectoryReader* pReader = s_pData->m_DataDirectories[i].m_pDataDirType->OpenFileToRead(sRelPath, FileShareMode, bOneSpecificDataDir);

    if (bAllowFileEvents && pReader != nullptr)
    {
      // Broadcast that this file has been opened.
      FileEvent fe;
      fe.m_EventType        = FileEventType::OpenFileSucceeded;
      fe.m_sFileOrDirectory = sRelPath;
      fe.m_sOther           = sRootName;
      fe.m_pDataDir         = s_pData->m_DataDirectories[i].m_pDataDirType;
      s_pData->m_Event.Broadcast(fe);

      return pReader;
    }
  }

  if (bAllowFileEvents)
  {
    // Broadcast that opening this file failed.
    FileEvent fe;
    fe.m_EventType        = FileEventType::OpenFileFailed;
    fe.m_sFileOrDirectory = sPath;
    s_pData->m_Event.Broadcast(fe);
  }

  return nullptr;
}

xiiDataDirectoryWriter* xiiFileSystem::GetFileWriter(xiiStringView sFile, xiiFileShareMode::Enum FileShareMode, bool bAllowFileEvents)
{
  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  if (sFile.IsEmpty())
    return nullptr;

  XII_LOCK(s_pData->m_FsMutex);

  xiiString sRootName;

  if (!xiiPathUtils::IsAbsolutePath(sFile))
  {
    XII_ASSERT_DEV(sFile.StartsWith(":"),
                   "Only native absolute paths or rooted paths (starting with a colon and then the data dir root name) are allowed for "
                   "writing to files. This path is neither: '{0}'",
                   sFile);
    sFile = ExtractRootName(sFile, sRootName);
  }

  // clean up the path to get rid of ".." etc.
  xiiStringBuilder sPath = sFile;
  sPath.MakeCleanPath();

  // the last added data directory has the highest priority
  for (xiiInt32 i = (xiiInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    if (s_pData->m_DataDirectories[i].m_Usage != xiiDataDirUsage::AllowWrites)
      continue;

    // ignore all directories that have not the category that is currently requested
    if (s_pData->m_DataDirectories[i].m_sRootName != sRootName)
      continue;

    xiiStringView sRelPath = GetDataDirRelativePath(sPath, i);

    if (bAllowFileEvents)
    {
      // Broadcast that we now try to open this file
      // Could be useful to check this file out before it is accessed
      FileEvent fe;
      fe.m_EventType        = FileEventType::CreateFileAttempt;
      fe.m_sFileOrDirectory = sRelPath;
      fe.m_sOther           = sRootName;
      fe.m_pDataDir         = s_pData->m_DataDirectories[i].m_pDataDirType;
      s_pData->m_Event.Broadcast(fe);
    }

    xiiDataDirectoryWriter* pWriter = s_pData->m_DataDirectories[i].m_pDataDirType->OpenFileToWrite(sRelPath, FileShareMode);

    if (bAllowFileEvents && pWriter != nullptr)
    {
      // Broadcast that this file has been created.
      FileEvent fe;
      fe.m_EventType        = FileEventType::CreateFileSucceeded;
      fe.m_sFileOrDirectory = sRelPath;
      fe.m_sOther           = sRootName;
      fe.m_pDataDir         = s_pData->m_DataDirectories[i].m_pDataDirType;
      s_pData->m_Event.Broadcast(fe);

      return pWriter;
    }
  }

  if (bAllowFileEvents)
  {
    // Broadcast that creating this file failed.
    FileEvent fe;
    fe.m_EventType        = FileEventType::CreateFileFailed;
    fe.m_sFileOrDirectory = sPath;
    s_pData->m_Event.Broadcast(fe);
  }

  return nullptr;
}

xiiResult xiiFileSystem::ResolvePath(xiiStringView sPath, xiiStringBuilder* out_pAbsolutePath, xiiStringBuilder* out_pDataDirRelativePath, const xiiDataDirectoryInfo** out_pDataDir /*= nullptr*/)
{
  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  XII_LOCK(s_pData->m_FsMutex);

  xiiStringBuilder absPath, relPath;

  if (sPath.StartsWith(":"))
  {
    // writing is only allowed using rooted paths
    xiiString sRootName;
    ExtractRootName(sPath, sRootName);

    const xiiDataDirectoryInfo* pDataDir = GetDataDirForRoot(sRootName);

    if (pDataDir == nullptr)
      return XII_FAILURE;

    if (out_pDataDir != nullptr)
      *out_pDataDir = pDataDir;

    relPath = sPath.GetShrunk(sRootName.GetCharacterCount() + 2);

    absPath = pDataDir->m_pDataDirType->GetRedirectedDataDirectoryPath(); /// \todo We might also need the none-redirected path as an output
    absPath.AppendPath(relPath);
  }
  else if (xiiPathUtils::IsAbsolutePath(sPath))
  {
    absPath = sPath;
    absPath.MakeCleanPath();

    for (xiiUInt32 dd = s_pData->m_DataDirectories.GetCount(); dd > 0; --dd)
    {
      auto& dir = s_pData->m_DataDirectories[dd - 1];

      if (xiiPathUtils::IsSubPath(dir.m_pDataDirType->GetRedirectedDataDirectoryPath(), absPath))
      {
        if (out_pAbsolutePath)
          *out_pAbsolutePath = absPath;

        if (out_pDataDirRelativePath)
        {
          *out_pDataDirRelativePath = absPath;
          out_pDataDirRelativePath->MakeRelativeTo(dir.m_pDataDirType->GetRedirectedDataDirectoryPath()).IgnoreResult();
        }

        if (out_pDataDir)
          *out_pDataDir = &dir;

        return XII_SUCCESS;
      }
    }

    return XII_FAILURE;
  }
  else
  {
    // try to get a reader -> if we get one, the file does indeed exist
    xiiDataDirectoryReader* pReader = xiiFileSystem::GetFileReader(sPath, xiiFileShareMode::SharedReads, true);

    if (!pReader)
      return XII_FAILURE;

    if (out_pDataDir != nullptr)
    {
      for (xiiUInt32 dd = s_pData->m_DataDirectories.GetCount(); dd > 0; --dd)
      {
        auto& dir = s_pData->m_DataDirectories[dd - 1];

        if (dir.m_pDataDirType == pReader->GetDataDirectory())
        {
          *out_pDataDir = &dir;
        }
      }
    }

    relPath = pReader->GetFilePath();

    absPath = pReader->GetDataDirectory()->GetRedirectedDataDirectoryPath(); /// \todo We might also need the none-redirected path as an output
    absPath.AppendPath(relPath);

    pReader->Close();
  }

  if (out_pAbsolutePath)
    *out_pAbsolutePath = absPath;

  if (out_pDataDirRelativePath)
    *out_pDataDirRelativePath = relPath;

  return XII_SUCCESS;
}

xiiResult xiiFileSystem::FindFolderWithSubPath(xiiStringBuilder& ref_sResult, xiiStringView sStartDirectory, xiiStringView sSubPath, xiiStringView sRedirectionFileName /*= nullptr*/)
{
  xiiStringBuilder sStartDirAbs = sStartDirectory;
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
      ref_sResult.Clear();
      return XII_FAILURE;
    }

    sStartDirAbs = abs;
  }

  ref_sResult = sStartDirectory;
  ref_sResult.MakeCleanPath();

  xiiStringBuilder FullPath, sRedirection;

  while (!ref_sResult.IsEmpty())
  {
    sRedirection.Clear();

    if (!sRedirectionFileName.IsEmpty())
    {
      FullPath = sStartDirAbs;
      FullPath.AppendPath(sRedirectionFileName);

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
      FullPath.AppendPath(sSubPath);
      FullPath.MakeCleanPath();

      if (xiiOSFile::ExistsDirectory(FullPath) || xiiOSFile::ExistsFile(FullPath))
      {
        ref_sResult.AppendPath(sRedirection);
        ref_sResult.MakeCleanPath();
        return XII_SUCCESS;
      }
    }

    // then try without the redirection
    FullPath = sStartDirAbs;
    FullPath.AppendPath(sSubPath);
    FullPath.MakeCleanPath();

    if (xiiOSFile::ExistsDirectory(FullPath) || xiiOSFile::ExistsFile(FullPath))
    {
      return XII_SUCCESS;
    }

    ref_sResult.PathParentDirectory();
    sStartDirAbs.PathParentDirectory();
  }

  return XII_FAILURE;
}

bool xiiFileSystem::ResolveAssetRedirection(xiiStringView sPathOrAssetGuid, xiiStringBuilder& out_sRedirection)
{
  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  XII_LOCK(s_pData->m_FsMutex);

  for (auto& dd : s_pData->m_DataDirectories)
  {
    if (dd.m_pDataDirType->ResolveAssetRedirection(sPathOrAssetGuid, out_sRedirection))
      return true;
  }

  out_sRedirection = sPathOrAssetGuid;
  return false;
}

xiiStringView xiiFileSystem::MigrateFileLocation(xiiStringView sOldLocation, xiiStringView sNewLocation)
{
  xiiStringBuilder sOldPathFull, sNewPathFull;

  if (ResolvePath(sOldLocation, &sOldPathFull, nullptr).Failed() || sOldPathFull.IsEmpty())
  {
    // If the old path could not be resolved, use the new path
    return sNewLocation;
  }

  ResolvePath(sNewLocation, &sNewPathFull, nullptr).AssertSuccess();

  if (!ExistsFile(sOldPathFull))
  {
    // old path doesn't exist -> use the new
    return sNewLocation;
  }

  // old path does exist -> deal with it

  if (ExistsFile(sNewPathFull))
  {
    // new path also exists -> delete the old one (in all data directories), use the new one
    DeleteFile(sOldLocation); // location, not full path
    return sNewLocation;
  }

  // new one doesn't exist -> try to move old to new
  if (xiiOSFile::MoveFileOrDirectory(sOldPathFull, sNewPathFull).Failed())
  {
    // if the old location exists, but we can't move the file, return the old location to use
    return sOldLocation;
  }

  // deletes the file in the old location in ALL data directories,
  // so that they can't interfere with the new file in the future
  DeleteFile(sOldLocation); // location, not full path

  // if we successfully moved the file to the new location, use the new location
  return sNewLocation;
}

void xiiFileSystem::ReloadAllExternalDataDirectoryConfigs()
{
  XII_LOG_BLOCK("ReloadAllExternalDataDirectoryConfigs");

  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  XII_LOCK(s_pData->m_FsMutex);

  for (auto& dd : s_pData->m_DataDirectories)
  {
    dd.m_pDataDirType->ReloadExternalConfigs();
  }
}

void xiiFileSystem::Startup()
{
  s_pData = XII_DEFAULT_NEW(FileSystemData);
}

void xiiFileSystem::Shutdown()
{
  {
    XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
    XII_LOCK(s_pData->m_FsMutex);

    s_pData->m_DataDirFactories.Clear();

    ClearAllDataDirectories();
  }

  XII_DEFAULT_DELETE(s_pData);
}

xiiResult xiiFileSystem::DetectSdkRootDirectory(xiiStringView sExpectedSubFolder /*= "Data/Base"*/)
{
  if (!s_sSdkRootDir.IsEmpty())
    return XII_SUCCESS;

  xiiStringBuilder sSDKRoot;
  if (xiiFileSystem::FindFolderWithSubPath(sSDKRoot, xiiOSFile::GetApplicationDirectory(), sExpectedSubFolder, "xiiSdkRoot.txt").Failed())
  {
    xiiLog::Error("Could not find SDK root. Application dir is '{0}'. Searched for parent with '{1}' sub-folder.", xiiOSFile::GetApplicationDirectory(), sExpectedSubFolder);
    return XII_FAILURE;
  }

  xiiFileSystem::SetSdkRootDirectory(sSDKRoot);
  return XII_SUCCESS;
}

void xiiFileSystem::SetSdkRootDirectory(xiiStringView sSdkDir)
{
  xiiStringBuilder s = sSdkDir;
  s.MakeCleanPath();

  s_sSdkRootDir = s;
}

xiiStringView xiiFileSystem::GetSdkRootDirectory()
{
  XII_ASSERT_DEV(!s_sSdkRootDir.IsEmpty(), "The project directory has not been set through 'xiiFileSystem::SetSdkRootDirectory'.");
  return s_sSdkRootDir;
}

void xiiFileSystem::SetSpecialDirectory(xiiStringView sName, xiiStringView sReplacement)
{
  xiiStringBuilder tmp = sName;
  tmp.ToLower();

  if (sReplacement.IsEmpty())
  {
    s_SpecialDirectories.Remove(tmp);
  }
  else
  {
    s_SpecialDirectories[tmp] = sReplacement;

    xiiLog::Dev("Setting special directory '{}' to '{}'.", sName, sReplacement);
  }
}

xiiResult xiiFileSystem::ResolveSpecialDirectory(xiiStringView sDirectory, xiiStringBuilder& out_sPath)
{
  if (sDirectory.IsEmpty() || !sDirectory.StartsWith(">"))
  {
    out_sPath = sDirectory;
    return XII_SUCCESS;
  }

  // skip the '>'
  sDirectory.ChopAwayFirstCharacterAscii();
  const char* szStart = sDirectory.GetStartPointer();

  const char* szEnd = sDirectory.FindSubString("/");

  if (szEnd == nullptr)
    szEnd = szStart + xiiStringUtils::GetStringElementCount(szStart);

  xiiStringBuilder sName;
  sName.SetSubString_FromTo(szStart, szEnd);
  sName.ToLower();

  const auto it = s_SpecialDirectories.Find(sName);
  if (it.IsValid())
  {
    out_sPath = it.Value();
    out_sPath.AppendPath(szEnd); // szEnd might be on \0 or a slash
    out_sPath.MakeCleanPath();
    return XII_SUCCESS;
  }

  if (sName == "sdk")
  {
    sDirectory.Shrink(3, 0);
    out_sPath = GetSdkRootDirectory();
    out_sPath.AppendPath(sDirectory);
    out_sPath.MakeCleanPath();
    return XII_SUCCESS;
  }

  if (sName == "user")
  {
    sDirectory.Shrink(4, 0);
    out_sPath = xiiOSFile::GetUserDataFolder();
    out_sPath.AppendPath(sDirectory);
    out_sPath.MakeCleanPath();
    return XII_SUCCESS;
  }

  if (sName == "temp")
  {
    sDirectory.Shrink(4, 0);
    out_sPath = xiiOSFile::GetTempDataFolder();
    out_sPath.AppendPath(sDirectory);
    out_sPath.MakeCleanPath();
    return XII_SUCCESS;
  }

  if (sName == "appdir")
  {
    sDirectory.Shrink(6, 0);
    out_sPath = xiiOSFile::GetApplicationDirectory();
    out_sPath.AppendPath(sDirectory);
    out_sPath.MakeCleanPath();
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

void xiiFileSystem::StartSearch(xiiFileSystemIterator& ref_iterator, xiiStringView sSearchTerm, xiiBitflags<xiiFileSystemIteratorFlags> flags /*= xiiFileSystemIteratorFlags::Default*/)
{
  XII_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  XII_LOCK(s_pData->m_FsMutex);

  xiiTemporaryHybridArray<xiiString, 16> folders;
  xiiStringBuilder                       sDdPath, sRelPath;

  if (sSearchTerm.IsRootedPath())
  {
    const xiiStringView root = sSearchTerm.GetRootedPathRootName();

    const xiiDataDirectoryInfo* pDataDir = FindDataDirectoryWithRoot(root);
    if (pDataDir == nullptr)
      return;

    sSearchTerm.SetStartPosition(root.GetEndPointer());

    if (!sSearchTerm.IsEmpty())
    {
      // root name should be followed by a slash
      sSearchTerm.ChopAwayFirstCharacterAscii();
    }

    folders.PushBack(pDataDir->m_pDataDirType->GetRedirectedDataDirectoryPath().GetView());
  }
  else if (sSearchTerm.IsAbsolutePath())
  {
    for (xiiUInt32 idx = s_pData->m_DataDirectories.GetCount(); idx > 0; --idx)
    {
      const auto& dd = s_pData->m_DataDirectories[idx - 1];

      sDdPath = dd.m_pDataDirType->GetRedirectedDataDirectoryPath();

      sRelPath = sSearchTerm;

      if (!sDdPath.IsEmpty())
      {
        if (sRelPath.MakeRelativeTo(sDdPath).Failed())
          continue;

        // this would use "../" if necessary, which we don't want
        if (sRelPath.StartsWith(".."))
          continue;
      }

      sSearchTerm = sRelPath;

      folders.PushBack(sDdPath);
      break;
    }
  }
  else
  {
    for (xiiUInt32 idx = s_pData->m_DataDirectories.GetCount(); idx > 0; --idx)
    {
      const auto& dd = s_pData->m_DataDirectories[idx - 1];

      sDdPath = dd.m_pDataDirType->GetRedirectedDataDirectoryPath();

      folders.PushBack(sDdPath);
    }
  }

  ref_iterator.StartMultiFolderSearch(folders, sSearchTerm, flags);
}

#endif

xiiResult xiiFileSystem::CreateDirectoryStructure(xiiStringView sPath)
{
  xiiStringBuilder sRedir;
  XII_SUCCEED_OR_RETURN(ResolveSpecialDirectory(sPath, sRedir));

  if (sRedir.IsRootedPath())
  {
    xiiFileSystem::ResolvePath(sRedir, &sRedir, nullptr).AssertSuccess();
  }

  if (!sRedir.IsAbsolutePath())
    return XII_FAILURE;

  return xiiOSFile::CreateDirectoryStructure(sRedir);
}

XII_STATICLINK_FILE(Foundation, Foundation_IO_FileSystem_Implementation_FileSystem);
