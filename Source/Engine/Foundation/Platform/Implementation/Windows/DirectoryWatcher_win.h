/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/IO/Implementation/FileSystemMirror.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Platform/Implementation/Windows/DirectoryWatcherCasing_win.h>
#include <Foundation/Platform/Implementation/Windows/DosDevicePath_win.h>

// Comment in to get verbose output on the function of the directory watcher
// #define DEBUG_FILE_WATCHER

#ifdef DEBUG_FILE_WATCHER
#  define DEBUG_LOG(...) xiiLog::Warning(__VA_ARGS__)
#else
#  define DEBUG_LOG(...)
#endif
namespace
{
  xiiCVarBool cvar_ForceNonNTFS("DirectoryWatcher.ForceNonNTFS", false, xiiCVarFlags::Default, "Forces the use of ReadDirectoryChanges instead of ReadDirectoryChangesEx");

  struct MoveEvent
  {
    xiiString m_sPath;
    bool      m_bIsDirectory = false;

    void Clear()
    {
      m_sPath.Clear();
    }

    bool IsEmpty() const
    {
      return m_sPath.IsEmpty();
    }
  };

  struct Change
  {
    xiiStringBuilder m_sEventFilePath;
    bool             m_bIsFile;
    DWORD            m_uiAction;
    LARGE_INTEGER    m_LastModificationTime;
  };

  using xiiFileSystemMirrorType = xiiFileSystemMirror<bool>;

  void GetChangesNTFS(xiiStringView sDirectoryPath, const xiiHybridArray<xiiUInt8, 4096>& buffer, xiiDynamicArray<Change>& ref_changes)
  {
    xiiUInt32 uiChanges = 1;
    auto      pInfo     = (const FILE_NOTIFY_EXTENDED_INFORMATION*)buffer.GetData();

    while (pInfo->NextEntryOffset != 0)
    {
      ++uiChanges;

      pInfo = (const FILE_NOTIFY_EXTENDED_INFORMATION*)(((xiiUInt8*)pInfo) + pInfo->NextEntryOffset);
    }

    ref_changes.Reserve(uiChanges);

    pInfo = (const FILE_NOTIFY_EXTENDED_INFORMATION*)buffer.GetData();

    while (true)
    {
      auto pDirectory   = xiiArrayPtr<const WCHAR>(pInfo->FileName, pInfo->FileNameLength / sizeof(WCHAR));
      int  iBytesNeeded = WideCharToMultiByte(CP_UTF8, 0, pDirectory.GetPtr(), pDirectory.GetCount(), nullptr, 0, nullptr, nullptr);
      if (iBytesNeeded > 0)
      {
        xiiTemporaryHybridArray<char, 1024> directory;
        directory.SetCountUninitialized(iBytesNeeded);
        WideCharToMultiByte(CP_UTF8, 0, pDirectory.GetPtr(), pDirectory.GetCount(), directory.GetData(), directory.GetCount(), nullptr, nullptr);

        Change& currentChange                = ref_changes.ExpandAndGetRef();
        currentChange.m_uiAction             = pInfo->Action;
        currentChange.m_LastModificationTime = pInfo->LastModificationTime;
        currentChange.m_bIsFile              = (pInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
        currentChange.m_sEventFilePath       = sDirectoryPath;
        currentChange.m_sEventFilePath.AppendPath(xiiStringView(directory.GetData(), directory.GetCount()));
        currentChange.m_sEventFilePath.MakeCleanPath();
      }

      if (pInfo->NextEntryOffset == 0)
      {
        break;
      }
      else
      {
        pInfo = (const FILE_NOTIFY_EXTENDED_INFORMATION*)(((xiiUInt8*)pInfo) + pInfo->NextEntryOffset);
      }
    }
  }

  void GetChangesNonNTFS(xiiStringView sDirectoryPath, const xiiHybridArray<xiiUInt8, 4096>& buffer, xiiDynamicArray<Change>& ref_changes)
  {
    xiiUInt32 uiChanges = 1;
    auto      pInfo     = (const FILE_NOTIFY_INFORMATION*)buffer.GetData();

    while (pInfo->NextEntryOffset != 0)
    {
      ++uiChanges;

      pInfo = (const FILE_NOTIFY_INFORMATION*)(((xiiUInt8*)pInfo) + pInfo->NextEntryOffset);
    }

    ref_changes.Reserve(ref_changes.GetCount() + uiChanges);

    pInfo = (const FILE_NOTIFY_INFORMATION*)buffer.GetData();

    while (true)
    {
      auto pDirectory   = xiiArrayPtr<const WCHAR>(pInfo->FileName, pInfo->FileNameLength / sizeof(WCHAR));
      int  iBytesNeeded = WideCharToMultiByte(CP_UTF8, 0, pDirectory.GetPtr(), pDirectory.GetCount(), nullptr, 0, nullptr, nullptr);

      if (iBytesNeeded > 0)
      {
        xiiTemporaryHybridArray<char, 1024> directory;
        directory.SetCountUninitialized(iBytesNeeded);
        WideCharToMultiByte(CP_UTF8, 0, pDirectory.GetPtr(), pDirectory.GetCount(), directory.GetData(), directory.GetCount(), nullptr, nullptr);

        Change& currentChange                = ref_changes.ExpandAndGetRef();
        currentChange.m_uiAction             = pInfo->Action;
        currentChange.m_LastModificationTime = {};
        currentChange.m_bIsFile              = true; // Pretend it's a file for now.
        currentChange.m_sEventFilePath       = sDirectoryPath;
        currentChange.m_sEventFilePath.AppendPath(xiiStringView(directory.GetData(), directory.GetCount()));
        currentChange.m_sEventFilePath.MakeCleanPath();
      }

      if (pInfo->NextEntryOffset == 0)
      {
        break;
      }
      else
      {
        pInfo = (const FILE_NOTIFY_INFORMATION*)(((xiiUInt8*)pInfo) + pInfo->NextEntryOffset);
      }
    }
  }

  void PostProcessNonNTFSChanges(xiiDynamicArray<Change>& ref_changes, xiiFileSystemMirrorType* pMirror, const MoveEvent& pendingRename, bool bResolveCasing)
  {
    xiiTemporaryHybridArray<xiiInt32, 4> nextOp;
    xiiInt32                             pendingNewName = -1;
    // Figure what changes belong to the same object by creating a linked list of changes. This part is tricky as we basically have to handle all the oddities that xiiDirectoryWatcher::EnumerateChanges already does again to figure out which operations belong to the same object.
    {
      xiiMap<xiiStringView, xiiUInt32> lastChangeAtPath;
      nextOp.SetCount(ref_changes.GetCount(), -1);

      xiiInt32 uiPendingRemoveOrRename = -1;
      xiiInt32 uiLastMoveFrom          = -1;
      bool     bPendingRenameAvailable = !pendingRename.m_sPath.IsEmpty();

      for (xiiUInt32 i = 0; i < ref_changes.GetCount(); i++)
      {
        const Change& currentChange = ref_changes[i];

        if (uiPendingRemoveOrRename != -1 && (currentChange.m_uiAction == FILE_ACTION_RENAMED_OLD_NAME) && (ref_changes[uiPendingRemoveOrRename].m_sEventFilePath == currentChange.m_sEventFilePath))
        {
          // This is the bogus removed event because we changed the casing of a file / directory, ignore.
          lastChangeAtPath.Insert(ref_changes[uiPendingRemoveOrRename].m_sEventFilePath, uiPendingRemoveOrRename);

          uiPendingRemoveOrRename = -1;
        }

        if (uiPendingRemoveOrRename != -1)
        {
          // An actual remove: Stop tracking the change.
          lastChangeAtPath.Remove(ref_changes[uiPendingRemoveOrRename].m_sEventFilePath);

          uiPendingRemoveOrRename = -1;
        }

        xiiUInt32* uiUniqueItemIndex = nullptr;
        switch (currentChange.m_uiAction)
        {
          case FILE_ACTION_ADDED:
          {
            lastChangeAtPath.Insert(currentChange.m_sEventFilePath, i);
          }
          break;
          case FILE_ACTION_REMOVED:
          {
            if (lastChangeAtPath.TryGetValue(currentChange.m_sEventFilePath, uiUniqueItemIndex))
            {
              nextOp[*uiUniqueItemIndex] = i;
              *uiUniqueItemIndex         = i;
            }
            uiPendingRemoveOrRename = i;
          }
          break;
          case FILE_ACTION_MODIFIED:
          {
            if (lastChangeAtPath.TryGetValue(currentChange.m_sEventFilePath, uiUniqueItemIndex))
            {
              nextOp[*uiUniqueItemIndex] = i;
              *uiUniqueItemIndex         = i;
            }
            else
            {
              lastChangeAtPath[currentChange.m_sEventFilePath] = i;
            }
          }
          break;
          case FILE_ACTION_RENAMED_OLD_NAME:
          {
            if (lastChangeAtPath.TryGetValue(currentChange.m_sEventFilePath, uiUniqueItemIndex))
            {
              nextOp[*uiUniqueItemIndex] = i;
              *uiUniqueItemIndex         = i;
            }
            else
            {
              lastChangeAtPath[currentChange.m_sEventFilePath] = i;
            }
            uiLastMoveFrom          = i;
            bPendingRenameAvailable = false;
          }
          break;
          case FILE_ACTION_RENAMED_NEW_NAME:
          {
            if (uiLastMoveFrom != -1)
            {
              nextOp[uiLastMoveFrom] = i;

              lastChangeAtPath.Remove(ref_changes[uiLastMoveFrom].m_sEventFilePath);
            }
            if (uiLastMoveFrom == -1 && bPendingRenameAvailable)
            {
              pendingNewName = i;
            }
            bPendingRenameAvailable = false;
            uiLastMoveFrom          = -1;

            lastChangeAtPath.Insert(currentChange.m_sEventFilePath, i);
          }
          break;
        }
      }
    }

    // Anything that is chained via the nextOp linked list must be given the same type.
    // Instead of building arrays of arrays, we create a bit field of all changes and then flatten the linked list at the first set bit. While iterating we remove everything we reached via the linked list so on the next call to get the first bit we will find another object that needs processing. As the operations are ordered, the first bit will always point to the very first operation of an object (nextOp can never point to a previous element).
    xiiHybridBitfield<128> pendingChanges;
    pendingChanges.SetCount(ref_changes.GetCount(), true);

    // Get start of first object.
    xiiTemporaryHybridArray<Change*, 4> objectChanges;
    auto                                it = pendingChanges.GetIterator();
    while (it.IsValid())
    {
      // Flatten the changes for one object into a list for easier processing.
      {
        objectChanges.Clear();

        xiiUInt32 uiCurrentIndex = it.Value();
        objectChanges.PushBack(&ref_changes[uiCurrentIndex]);

        pendingChanges.ClearBit(uiCurrentIndex);

        while (nextOp[uiCurrentIndex] != -1)
        {
          uiCurrentIndex = nextOp[uiCurrentIndex];

          pendingChanges.ClearBit(uiCurrentIndex);
          objectChanges.PushBack(&ref_changes[uiCurrentIndex]);
        }
      }

      // Figure out what type the object is. There is no correct way of doing this, which is the reason why ReadDirectoryChangesExW exists. There are however some good heuristics we can use:
      // 1. If the change is on an existing object, we should know it's type from the mirror. This is 100% correct.
      // 2. If object still exists, we can query its stats on disk to determine its type. In rare cases, this can be wrong but you would need to create a case where a file is replaced with a folder of the same name or something.
      // 3. If the object was created and deleted in the same enumeration, we cannot know what type it was, so we take a guess.
      {
        bool                          bIsFile = true;
        xiiFileSystemMirrorType::Type type;
        xiiStringBuilder              cachedPath;

        if (pendingNewName >= 0 && objectChanges[0] == &ref_changes[pendingNewName])
        {
          bIsFile = !pendingRename.m_bIsDirectory;
        }
        else if (pMirror->GetType(objectChanges[0]->m_sEventFilePath, type).Succeeded() || (bResolveCasing && pMirror->GetPathCasing(objectChanges[0]->m_sEventFilePath, cachedPath, type).Succeeded()))
        {
          bIsFile = type == xiiFileSystemMirrorType::Type::File;
        }
        else
        {
          bool bIsFileFound = false;
          for (Change* pCurrentChange : objectChanges)
          {
            xiiFileStats stats;
            if (xiiOSFile::GetFileStats(pCurrentChange->m_sEventFilePath, stats).Succeeded())
            {
              bIsFile      = !stats.m_bIsDirectory;
              bIsFileFound = true;
              break;
            }
          }
          if (!bIsFileFound)
          {
            // No stats and no entry in mirror: It's guessing time!
            bIsFile = objectChanges[0]->m_sEventFilePath.FindSubString(".") != nullptr;
          }
        }

        // Apply type to all objects in the chain.
        for (Change* pCurrentChange : objectChanges)
        {
          pCurrentChange->m_bIsFile = bIsFile;
        }
      }

      // Find start of next object.
      it = pendingChanges.GetIterator();
    }
  }

} // namespace

struct xiiDirectoryWatcherImpl
{
  void DoRead();
  void EnumerateChangesImpl(xiiStringView sDirectoryPath, xiiTime waitUpTo, const xiiDelegate<void(const Change&)>& callback);

  bool                                    m_bNTFS               = false;
  bool                                    m_bValidatePathCasing = false;
  xiiTime                                 m_LastCasingCheck;
  xiiUInt32                               m_uiNextCasingEntry = 0;
  MoveEvent                               m_PendingRename;
  xiiString                               m_sReconciledRename;
  HANDLE                                  m_DirectoryHandle;
  DWORD                                   m_uiFilter;
  OVERLAPPED                              m_Overlapped;
  HANDLE                                  m_OverlappedEvent;
  xiiDynamicArray<xiiUInt8>               m_Buffer;
  xiiBitflags<xiiDirectoryWatcher::Watch> m_WatchFlags;
  xiiUniquePtr<xiiFileSystemMirrorType>   m_FileSystemMirror; // store the last modification timestamp alongside each file
};

xiiDirectoryWatcher::xiiDirectoryWatcher() :
  m_pImpl(XII_DEFAULT_NEW(xiiDirectoryWatcherImpl))
{
  m_pImpl->m_Buffer.SetCountUninitialized(1024 * 1024);
}

xiiResult xiiDirectoryWatcher::OpenDirectory(xiiStringView sAbsolutePath, xiiBitflags<Watch> whatToWatch)
{
  m_pImpl->m_bNTFS               = false;
  m_pImpl->m_bValidatePathCasing = false;
  m_pImpl->m_LastCasingCheck     = xiiTime::MakeZero();
  m_pImpl->m_uiNextCasingEntry   = 0;
  m_pImpl->m_PendingRename.Clear();
  m_pImpl->m_sReconciledRename.Clear();
  {
    // Get drive root:
    xiiStringBuilder sTemp = sAbsolutePath;
    sTemp.MakeCleanPath();

    const char* szFirst = sTemp.FindSubString("/");
    XII_ASSERT_DEV(szFirst != nullptr, "The path '{}' is not absolute", sTemp);
    xiiStringView sRoot = sAbsolutePath.GetSubString(0, static_cast<xiiUInt32>(szFirst - sTemp.GetData()) + 1);

    WCHAR szFileSystemName[8];
    BOOL  result                   = GetVolumeInformationW(xiiStringWChar(sRoot), nullptr, 0, nullptr, nullptr, nullptr, szFileSystemName, XII_ARRAY_SIZE(szFileSystemName));
    m_pImpl->m_bNTFS               = result == TRUE && xiiStringUtf8(szFileSystemName).GetView() == "NTFS" && !cvar_ForceNonNTFS.GetValue();
    m_pImpl->m_bValidatePathCasing = result == TRUE && xiiStringUtf8(szFileSystemName).GetView() == "ReFS";
  }

  XII_ASSERT_DEV(m_sDirectoryPath.IsEmpty(), "Directory already open, call CloseDirectory first!");

  xiiStringBuilder sPath(sAbsolutePath);
  sPath.MakeCleanPath();
  sPath.Trim("/");

  m_pImpl->m_WatchFlags      = whatToWatch;
  m_pImpl->m_uiFilter        = FILE_NOTIFY_CHANGE_FILE_NAME;
  const bool bRequiresMirror = whatToWatch.IsSet(Watch::Writes) || whatToWatch.AreAllSet(Watch::Deletes | Watch::Subdirectories);
  if (bRequiresMirror)
  {
    m_pImpl->m_uiFilter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
  }

  if (!m_pImpl->m_bNTFS || bRequiresMirror)
  {
    m_pImpl->m_FileSystemMirror = XII_DEFAULT_NEW(xiiFileSystemMirrorType);
    m_pImpl->m_FileSystemMirror->AddDirectory(sPath).AssertSuccess();
  }

  if (whatToWatch.IsAnySet(Watch::Deletes | Watch::Creates | Watch::Renames))
  {
    m_pImpl->m_uiFilter |= FILE_NOTIFY_CHANGE_DIR_NAME;
  }

  m_pImpl->m_DirectoryHandle = CreateFileW(xiiDosDevicePath(sPath), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);

  if (m_pImpl->m_DirectoryHandle == INVALID_HANDLE_VALUE)
  {
    return XII_FAILURE;
  }

  m_pImpl->m_OverlappedEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
  if (m_pImpl->m_OverlappedEvent == nullptr)
  {
    CloseHandle(m_pImpl->m_DirectoryHandle);
    m_pImpl->m_FileSystemMirror.Clear();
    return XII_FAILURE;
  }

  m_pImpl->DoRead();
  m_sDirectoryPath = sPath;

  return XII_SUCCESS;
}

void xiiDirectoryWatcher::CloseDirectory()
{
  if (!m_sDirectoryPath.IsEmpty())
  {
    // Reads can be issued by a worker thread. CancelIo only cancels reads from this thread, and cancellation itself does not wait for buffer writes to end.
    CancelIoEx(m_pImpl->m_DirectoryHandle, &m_pImpl->m_Overlapped);

    DWORD uiTransferred = 0;
    GetOverlappedResult(m_pImpl->m_DirectoryHandle, &m_pImpl->m_Overlapped, &uiTransferred, TRUE);

    CloseHandle(m_pImpl->m_OverlappedEvent);
    CloseHandle(m_pImpl->m_DirectoryHandle);

    m_sDirectoryPath.Clear();
    m_pImpl->m_FileSystemMirror.Clear();
    m_pImpl->m_PendingRename.Clear();
    m_pImpl->m_sReconciledRename.Clear();
  }
}

xiiDirectoryWatcher::~xiiDirectoryWatcher()
{
  CloseDirectory();

  XII_DEFAULT_DELETE(m_pImpl);
}

void xiiDirectoryWatcherImpl::DoRead()
{
  ResetEvent(m_OverlappedEvent);
  memset(&m_Overlapped, 0, sizeof(m_Overlapped));
  m_Overlapped.hEvent = m_OverlappedEvent;

  if (m_bNTFS)
  {
    BOOL bSucceeded = ReadDirectoryChangesExW(m_DirectoryHandle, m_Buffer.GetData(), m_Buffer.GetCount(), m_WatchFlags.IsSet(xiiDirectoryWatcher::Watch::Subdirectories), m_uiFilter, nullptr, &m_Overlapped, nullptr, ReadDirectoryNotifyExtendedInformation);
    XII_ASSERT_DEV(bSucceeded, "ReadDirectoryChangesExW failed.");
    XII_IGNORE_UNUSED(bSucceeded);
  }
  else
  {
    BOOL bSucceeded = ReadDirectoryChangesW(m_DirectoryHandle, m_Buffer.GetData(), m_Buffer.GetCount(), m_WatchFlags.IsSet(xiiDirectoryWatcher::Watch::Subdirectories), m_uiFilter, nullptr, &m_Overlapped, nullptr);
    XII_ASSERT_DEV(bSucceeded, "ReadDirectoryChangesW failed.");
    XII_IGNORE_UNUSED(bSucceeded);
  }
}

void xiiDirectoryWatcherImpl::EnumerateChangesImpl(xiiStringView sDirectoryPath, xiiTime waitUpTo, const xiiDelegate<void(const Change&)>& callback)
{
  xiiTemporaryHybridArray<Change, 6> changes;

  xiiTemporaryHybridArray<xiiUInt8, 4096> buffer;
  while (WaitForSingleObject(m_OverlappedEvent, static_cast<DWORD>(waitUpTo.GetMilliseconds())) == WAIT_OBJECT_0)
  {
    waitUpTo = xiiTime::MakeZero(); // only wait on the first call to GetQueuedCompletionStatus

    DWORD numberOfBytes = 0;
    GetOverlappedResult(m_DirectoryHandle, &m_Overlapped, &numberOfBytes, FALSE);

    // Copy the buffer
    buffer.SetCountUninitialized(numberOfBytes);
    buffer.GetArrayPtr().CopyFrom(m_Buffer.GetArrayPtr().GetSubArray(0, numberOfBytes));

    // Reissue the read request
    DoRead();

    if (numberOfBytes == 0)
    {
      break; // Preserve changes collected before an empty completion.
    }

    // We can fire NTFS events right away as they don't need post processing which prevents us from resizing the changes array unnecessarily.
    if (m_bNTFS)
    {
      GetChangesNTFS(sDirectoryPath, buffer, changes);

      for (const Change& change : changes)
      {
        callback(change);
      }
      changes.Clear();
    }
    else
    {
      GetChangesNonNTFS(sDirectoryPath, buffer, changes);
    }
  }

  // Non-NTFS changes need to be collected and processed in one go to be able to reconstruct the type of the change.
  if (!m_bNTFS)
  {
    PostProcessNonNTFSChanges(changes, m_FileSystemMirror.Borrow(), m_PendingRename, m_bValidatePathCasing);

    for (const Change& change : changes)
    {
      callback(change);
    }
  }
}

void xiiDirectoryWatcher::EnumerateChanges(EnumerateChangesFunction func, xiiTime waitUpTo)
{
  xiiFileSystemMirrorType* pMirror = m_pImpl->m_FileSystemMirror.Borrow();
  XII_ASSERT_DEV(!m_sDirectoryPath.IsEmpty(), "No directory opened!");

  MoveEvent                                     pendingRemoveOrRename;
  const xiiBitflags<xiiDirectoryWatcher::Watch> whatToWatch = m_pImpl->m_WatchFlags;
  // Renaming a file to the same filename with different casing triggers the events REMOVED (old casing) -> RENAMED_OLD_NAME -> _RENAMED_NEW_NAME.
  // Thus, we need to cache every remove event to make sure the very next event is not a rename of the exact same file.
  auto FirePendingRemove = [&]() -> void {
    if (!pendingRemoveOrRename.IsEmpty())
    {
      if (pendingRemoveOrRename.m_bIsDirectory)
      {
        if (whatToWatch.IsSet(Watch::Deletes))
        {
          if (pMirror && whatToWatch.IsSet(Watch::Subdirectories))
          {
            pMirror->Enumerate(pendingRemoveOrRename.m_sPath, [&](const xiiStringBuilder& sPath, xiiFileSystemMirrorType::Type type) { func(sPath, xiiDirectoryWatcherAction::Removed, (type == xiiFileSystemMirrorType::Type::File) ? xiiDirectoryWatcherType::File : xiiDirectoryWatcherType::Directory); }).AssertSuccess();
          }
          func(pendingRemoveOrRename.m_sPath, xiiDirectoryWatcherAction::Removed, xiiDirectoryWatcherType::Directory);
        }
        if (pMirror)
        {
          pMirror->RemoveDirectory(pendingRemoveOrRename.m_sPath).AssertSuccess();
        }
      }
      else
      {
        if (pMirror)
        {
          pMirror->RemoveFile(pendingRemoveOrRename.m_sPath).AssertSuccess();
        }
        if (whatToWatch.IsSet(xiiDirectoryWatcher::Watch::Deletes))
        {
          func(pendingRemoveOrRename.m_sPath, xiiDirectoryWatcherAction::Removed, xiiDirectoryWatcherType::File);
        }
      }
      pendingRemoveOrRename.Clear();
    }
  };

  XII_SCOPE_EXIT(FirePendingRemove());

  MoveEvent& lastMoveFrom = m_pImpl->m_PendingRename;

  // Process the messages
  m_pImpl->EnumerateChangesImpl(m_sDirectoryPath, waitUpTo, [&](const Change& nativeInfo) -> void {
    Change        normalizedInfo;
    const Change* pInfo = &nativeInfo;

    if (m_pImpl->m_bValidatePathCasing && pMirror && nativeInfo.m_uiAction == FILE_ACTION_REMOVED)
    {
      normalizedInfo = nativeInfo;
      pInfo          = &normalizedInfo;
    }

    const Change& info = *pInfo;
    if (m_pImpl->m_bValidatePathCasing && pMirror && info.m_uiAction == FILE_ACTION_REMOVED)
    {
      xiiStringBuilder              sCachedPath;
      xiiFileSystemMirrorType::Type type;

      if (!lastMoveFrom.IsEmpty() && lastMoveFrom.m_sPath.IsEqual_NoCase(info.m_sEventFilePath))
      {
        lastMoveFrom.Clear();
      }

      if (pMirror->GetPathCasing(info.m_sEventFilePath, sCachedPath, type).Failed())
        return; // A delayed native removal already reconciled from the mirror.

      normalizedInfo.m_sEventFilePath = sCachedPath;
    }

    xiiDirectoryWatcherAction action    = xiiDirectoryWatcherAction::None;
    bool                      fireEvent = false;

    if (!pendingRemoveOrRename.IsEmpty() && info.m_bIsFile == !pendingRemoveOrRename.m_bIsDirectory && info.m_uiAction == FILE_ACTION_RENAMED_OLD_NAME && pendingRemoveOrRename.m_sPath == info.m_sEventFilePath)
    {
      // This is the bogus removed event because we changed the casing of a file / directory, ignore.
      pendingRemoveOrRename.Clear();
    }
    FirePendingRemove();

    if (info.m_uiAction == FILE_ACTION_RENAMED_OLD_NAME && m_pImpl->m_bValidatePathCasing && pMirror)
    {
      // ReFS may omit NEW_NAME. A later OLD_NAME starts another rename, so the earlier unpaired record cannot supply its source (including after deletion).
      lastMoveFrom.Clear();

      xiiStringBuilder              sCachedPath;
      xiiFileSystemMirrorType::Type cachedType;

      if (pMirror->GetPathCasing(info.m_sEventFilePath, sCachedPath, cachedType).Failed())
        return; // The source was already reconciled and removed.

      if (ReconcileCaseRename(*pMirror, info.m_sEventFilePath, m_pImpl->m_sReconciledRename, [&](xiiStringView path, xiiDirectoryWatcherAction action, xiiDirectoryWatcherType type) -> void {
            if (whatToWatch.IsSet(Watch::Renames))
            {
              func(path, action, type);
            }
          }))
        return;
    }

    if (info.m_uiAction == FILE_ACTION_RENAMED_NEW_NAME && !m_pImpl->m_sReconciledRename.IsEmpty() && info.m_sEventFilePath.IsEqual_NoCase(m_pImpl->m_sReconciledRename))
    {
      m_pImpl->m_sReconciledRename.Clear();
      return;
    }

    if (info.m_uiAction == FILE_ACTION_RENAMED_OLD_NAME)
    {
      m_pImpl->m_sReconciledRename.Clear();
    }

    if (info.m_bIsFile)
    {
      switch (info.m_uiAction)
      {
        case FILE_ACTION_ADDED:
        {
          DEBUG_LOG("FILE_ACTION_ADDED {} ({})", info.m_sEventFilePath, info.m_LastModificationTime.QuadPart);

          action    = xiiDirectoryWatcherAction::Added;
          fireEvent = whatToWatch.IsSet(xiiDirectoryWatcher::Watch::Creates);

          if (pMirror)
          {
            bool bFileAlreadyExists = false;
            pMirror->AddFile(info.m_sEventFilePath, true, &bFileAlreadyExists, nullptr).AssertSuccess();

            if (bFileAlreadyExists)
            {
              fireEvent = false;
            }
          }
        }
        break;
        case FILE_ACTION_REMOVED:
        {
          DEBUG_LOG("FILE_ACTION_REMOVED {} ({})", info.m_sEventFilePath, info.m_LastModificationTime.QuadPart);

          action                = xiiDirectoryWatcherAction::Removed;
          fireEvent             = false;
          pendingRemoveOrRename = {info.m_sEventFilePath, false};
        }
        break;
        case FILE_ACTION_MODIFIED:
        {
          DEBUG_LOG("FILE_ACTION_MODIFIED {} ({})", info.m_sEventFilePath, info.m_LastModificationTime.QuadPart);

          action                   = xiiDirectoryWatcherAction::Modified;
          fireEvent                = whatToWatch.IsAnySet(xiiDirectoryWatcher::Watch::Writes);
          bool bIsFileAlreadyKnown = false;
          bool bAddPending         = false;

          if (pMirror)
          {
            pMirror->AddFile(info.m_sEventFilePath, false, &bIsFileAlreadyKnown, &bAddPending).AssertSuccess();
          }
          if (bIsFileAlreadyKnown && bAddPending)
          {
            fireEvent = false;
          }
        }
        break;
        case FILE_ACTION_RENAMED_OLD_NAME:
        {
          DEBUG_LOG("FILE_ACTION_RENAMED_OLD_NAME {} ({})", info.m_sEventFilePath, info.m_LastModificationTime.QuadPart);

          XII_ASSERT_DEV(lastMoveFrom.IsEmpty(), "Pending rename from '{0}' before old name '{1}'", lastMoveFrom.m_sPath, info.m_sEventFilePath);

          action    = xiiDirectoryWatcherAction::RenamedOldName;
          fireEvent = whatToWatch.IsAnySet(xiiDirectoryWatcher::Watch::Renames);

          XII_ASSERT_DEV(lastMoveFrom.IsEmpty(), "there should be no pending last move from");
          lastMoveFrom = {info.m_sEventFilePath, false};
        }
        break;
        case FILE_ACTION_RENAMED_NEW_NAME:
        {
          DEBUG_LOG("FILE_ACTION_RENAMED_NEW_NAME {} ({})", info.m_sEventFilePath, info.m_LastModificationTime.QuadPart);

          action    = xiiDirectoryWatcherAction::RenamedNewName;
          fireEvent = whatToWatch.IsAnySet(xiiDirectoryWatcher::Watch::Renames);

          XII_ASSERT_DEV(!lastMoveFrom.IsEmpty() && !lastMoveFrom.m_bIsDirectory, "last move from doesn't match");

          if (pMirror)
          {
            pMirror->RemoveFile(lastMoveFrom.m_sPath).AssertSuccess();
            pMirror->AddFile(info.m_sEventFilePath, false, nullptr, nullptr).AssertSuccess();
          }
          lastMoveFrom.Clear();
        }
        break;
      }

      if (fireEvent)
      {
        func(info.m_sEventFilePath, action, xiiDirectoryWatcherType::File);
      }
    }
    else
    {
      switch (info.m_uiAction)
      {
        case FILE_ACTION_ADDED:
        {
          DEBUG_LOG("DIR_ACTION_ADDED {}", info.m_sEventFilePath);

          bool bIsDirectoryAlreadyKnown = false;
          if (pMirror)
          {
            pMirror->AddDirectory(info.m_sEventFilePath, &bIsDirectoryAlreadyKnown).AssertSuccess();
          }

          if (whatToWatch.IsSet(Watch::Creates) && !bIsDirectoryAlreadyKnown)
          {
            func(info.m_sEventFilePath, xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::Directory);
          }

          // Whenever we add a directory we might be "to late" to see changes inside it.
          // So iterate the file system and make sure we track all files / subdirectories.
          xiiFileSystemIterator subdirIt;

          subdirIt.StartSearch(info.m_sEventFilePath.GetView(), whatToWatch.IsSet(xiiDirectoryWatcher::Watch::Subdirectories) ? xiiFileSystemIteratorFlags::ReportFilesAndFoldersRecursive : xiiFileSystemIteratorFlags::ReportFiles);

          xiiStringBuilder sTmpPath2;
          for (; subdirIt.IsValid(); subdirIt.Next())
          {
            const xiiFileStats& stats = subdirIt.GetStats();
            stats.GetFullPath(sTmpPath2);

            if (stats.m_bIsDirectory)
            {
              bIsDirectoryAlreadyKnown = false;

              if (pMirror)
              {
                pMirror->AddDirectory(sTmpPath2, &bIsDirectoryAlreadyKnown).AssertSuccess();
              }
              if (whatToWatch.IsSet(xiiDirectoryWatcher::Watch::Creates) && !bIsDirectoryAlreadyKnown)
              {
                func(sTmpPath2, xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::Directory);
              }
            }
            else
            {
              bool bFileExistsAlready = false;

              if (pMirror)
              {
                pMirror->AddFile(sTmpPath2, false, &bFileExistsAlready, nullptr).AssertSuccess();
              }
              if (whatToWatch.IsSet(xiiDirectoryWatcher::Watch::Creates) && !bFileExistsAlready)
              {
                func(sTmpPath2, xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::File);
              }
            }
          }
        }
        break;
        case FILE_ACTION_REMOVED:
        {
          DEBUG_LOG("DIR_ACTION_REMOVED {}", info.m_sEventFilePath);

          pendingRemoveOrRename = {info.m_sEventFilePath, true};
        }
        break;
        case FILE_ACTION_RENAMED_OLD_NAME:
        {
          DEBUG_LOG("DIR_ACTION_OLD_NAME {}", info.m_sEventFilePath);

          XII_ASSERT_DEV(lastMoveFrom.IsEmpty(), "Pending rename from '{0}' before old name '{1}'", lastMoveFrom.m_sPath, info.m_sEventFilePath);

          lastMoveFrom = {info.m_sEventFilePath, true};
        }
        break;
        case FILE_ACTION_RENAMED_NEW_NAME:
        {
          DEBUG_LOG("DIR_ACTION_NEW_NAME {}", info.m_sEventFilePath);

          XII_ASSERT_DEV(!lastMoveFrom.IsEmpty(), "rename old name and rename new name should always appear in pairs");

          if (pMirror)
          {
            pMirror->MoveDirectory(lastMoveFrom.m_sPath, info.m_sEventFilePath).AssertSuccess();
          }
          if (whatToWatch.IsSet(Watch::Renames))
          {
            func(lastMoveFrom.m_sPath, xiiDirectoryWatcherAction::RenamedOldName, xiiDirectoryWatcherType::Directory);
            func(info.m_sEventFilePath, xiiDirectoryWatcherAction::RenamedNewName, xiiDirectoryWatcherType::Directory);
          }
          lastMoveFrom.Clear();
        }
        break;
        default:
          break;
      }
    } //
  });

  FirePendingRemove();

  const xiiTime now = xiiTime::Now();
  if (m_pImpl->m_bValidatePathCasing && pMirror && now - m_pImpl->m_LastCasingCheck >= xiiTime::MakeFromMilliseconds(100))
  {
    m_pImpl->m_LastCasingCheck = now;

    ReconcileMirrorPaths(*pMirror, m_sDirectoryPath, m_pImpl->m_uiNextCasingEntry, whatToWatch.IsSet(Watch::Subdirectories), [&](xiiStringView path, xiiDirectoryWatcherAction action, xiiDirectoryWatcherType type) -> void {
      if (whatToWatch.IsSet(action == xiiDirectoryWatcherAction::Removed ? Watch::Deletes : Watch::Renames))
      {
        func(path, action, type);
      }
    });
  }
}

void xiiDirectoryWatcher::EnumerateChanges(xiiArrayPtr<xiiDirectoryWatcher*> watchers, EnumerateChangesFunction func, xiiTime waitUpTo)
{
  xiiTemporaryHybridArray<HANDLE, 16> events;
  events.SetCount(watchers.GetCount());

  for (xiiUInt32 i = 0; i < watchers.GetCount(); ++i)
  {
    events[i] = watchers[i]->m_pImpl->m_OverlappedEvent;
  }

  // Wait for any of the watchers to have some data ready
  if (!events.IsEmpty())
  {
    WaitForMultipleObjects(events.GetCount(), events.GetData(), FALSE, static_cast<DWORD>(waitUpTo.GetMilliseconds()));
  }

  // Iterate all of them to make sure we report all changes up to this point.
  for (xiiDirectoryWatcher* pWatcher : watchers)
  {
    pWatcher->EnumerateChanges(func);
  }
}
