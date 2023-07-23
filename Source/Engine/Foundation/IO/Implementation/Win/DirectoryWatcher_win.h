#pragma once

#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/IO/Implementation/Shared/FileSystemMirror.h>
#include <Foundation/IO/Implementation/Win/DosDevicePath_win.h>
#include <Foundation/Logging/Log.h>

// Comment in to get verbose output on the function of the directory watcher
// #define DEBUG_FILE_WATCHER

#ifdef DEBUG_FILE_WATCHER
#  define DEBUG_LOG(...) xiiLog::Debug(__VA_ARGS__)
#else
#  define DEBUG_LOG(...)
#endif

namespace
{
  struct MoveEvent
  {
    xiiString path;
    bool      isDirectory = false;

    void Clear()
    {
      path.Clear();
    }

    bool IsEmpty()
    {
      return path.IsEmpty();
    }
  };

  using xiiFileSystemMirrorType = xiiFileSystemMirror<bool>;
} // namespace

struct xiiDirectoryWatcherImpl
{
  void DoRead();

  HANDLE                                  m_directoryHandle;
  DWORD                                   m_filter;
  OVERLAPPED                              m_overlapped;
  HANDLE                                  m_overlappedEvent;
  xiiDynamicArray<xiiUInt8>               m_buffer;
  xiiBitflags<xiiDirectoryWatcher::Watch> m_whatToWatch;
  xiiUniquePtr<xiiFileSystemMirrorType>   m_mirror; // store the last modification timestamp alongside each file
};

xiiDirectoryWatcher::xiiDirectoryWatcher() :
  m_pImpl(XII_DEFAULT_NEW(xiiDirectoryWatcherImpl))
{
  m_pImpl->m_buffer.SetCountUninitialized(1024 * 1024);
}

xiiResult xiiDirectoryWatcher::OpenDirectory(xiiStringView sAbsolutePath, xiiBitflags<Watch> whatToWatch)
{
  XII_ASSERT_DEV(m_sDirectoryPath.IsEmpty(), "Directory already open, call CloseDirectory first!");
  xiiStringBuilder sPath(sAbsolutePath);
  sPath.MakeCleanPath();
  sPath.Trim("/");

  m_pImpl->m_whatToWatch = whatToWatch;
  m_pImpl->m_filter      = FILE_NOTIFY_CHANGE_FILE_NAME;
  if (whatToWatch.IsSet(Watch::Writes) || whatToWatch.AreAllSet(Watch::Deletes | Watch::Subdirectories))
  {
    m_pImpl->m_filter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
    m_pImpl->m_mirror = XII_DEFAULT_NEW(xiiFileSystemMirrorType);
    m_pImpl->m_mirror->AddDirectory(sPath).AssertSuccess();
  }

  if (whatToWatch.IsAnySet(Watch::Deletes | Watch::Creates | Watch::Renames))
  {
    m_pImpl->m_filter |= FILE_NOTIFY_CHANGE_DIR_NAME;
  }

  m_pImpl->m_directoryHandle = CreateFileW(xiiDosDevicePath(sPath), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                           nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
  if (m_pImpl->m_directoryHandle == INVALID_HANDLE_VALUE)
  {
    return XII_FAILURE;
  }

  m_pImpl->m_overlappedEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
  if (m_pImpl->m_overlappedEvent == INVALID_HANDLE_VALUE)
  {
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
    CancelIo(m_pImpl->m_directoryHandle);
    CloseHandle(m_pImpl->m_overlappedEvent);
    CloseHandle(m_pImpl->m_directoryHandle);
    m_sDirectoryPath.Clear();
  }
}

xiiDirectoryWatcher::~xiiDirectoryWatcher()
{
  CloseDirectory();
  XII_DEFAULT_DELETE(m_pImpl);
}

void xiiDirectoryWatcherImpl::DoRead()
{
  ResetEvent(m_overlappedEvent);
  memset(&m_overlapped, 0, sizeof(m_overlapped));
  m_overlapped.hEvent = m_overlappedEvent;
  BOOL success =
    ReadDirectoryChangesExW(m_directoryHandle, m_buffer.GetData(), m_buffer.GetCount(), m_whatToWatch.IsSet(xiiDirectoryWatcher::Watch::Subdirectories), m_filter, nullptr, &m_overlapped, nullptr, ReadDirectoryNotifyExtendedInformation);
  XII_ASSERT_DEV(success, "ReadDirectoryChangesW failed.");
}

void xiiDirectoryWatcher::EnumerateChanges(EnumerateChangesFunction func, xiiTime waitUpTo)
{
  XII_ASSERT_DEV(!m_sDirectoryPath.IsEmpty(), "No directory opened!");
  while (WaitForSingleObject(m_pImpl->m_overlappedEvent, static_cast<DWORD>(waitUpTo.GetMilliseconds())) == WAIT_OBJECT_0)
  {
    waitUpTo = xiiTime::Zero(); // only wait on the first call to GetQueuedCompletionStatus

    DWORD numberOfBytes = 0;
    GetOverlappedResult(m_pImpl->m_directoryHandle, &m_pImpl->m_overlapped, &numberOfBytes, FALSE);

    // Copy the buffer
    xiiHybridArray<xiiUInt8, 4096> buffer;
    buffer.SetCountUninitialized(numberOfBytes);
    buffer.GetArrayPtr().CopyFrom(m_pImpl->m_buffer.GetArrayPtr().GetSubArray(0, numberOfBytes));

    // Reissue the read request
    m_pImpl->DoRead();

    if (numberOfBytes == 0)
    {
      return;
    }

    const xiiBitflags<xiiDirectoryWatcher::Watch> whatToWatch = m_pImpl->m_whatToWatch;

    xiiFileSystemMirrorType* mirror = m_pImpl->m_mirror.Borrow();

    MoveEvent lastMoveFrom;

    // Progress the messages
    auto info = (const FILE_NOTIFY_EXTENDED_INFORMATION*)buffer.GetData();
    while (true)
    {
      auto     directory   = xiiArrayPtr<const WCHAR>(info->FileName, info->FileNameLength / sizeof(WCHAR));
      xiiInt32 bytesNeeded = WideCharToMultiByte(CP_UTF8, 0, directory.GetPtr(), directory.GetCount(), nullptr, 0, nullptr, nullptr);
      if (bytesNeeded > 0)
      {
        xiiHybridArray<char, 1024> dir;
        dir.SetCountUninitialized(bytesNeeded);
        WideCharToMultiByte(CP_UTF8, 0, directory.GetPtr(), directory.GetCount(), dir.GetData(), dir.GetCount(), nullptr, nullptr);
        xiiDirectoryWatcherAction action    = xiiDirectoryWatcherAction::None;
        bool                      fireEvent = false;

        xiiStringBuilder eventFilePath = m_sDirectoryPath;
        eventFilePath.AppendPath(xiiStringView(dir.GetData(), dir.GetCount()));
        eventFilePath.MakeCleanPath();

        if ((info->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
          switch (info->Action)
          {
            case FILE_ACTION_ADDED:
              DEBUG_LOG("FILE_ACTION_ADDED {} ({})", eventFilePath, info->LastModificationTime.QuadPart);
              action    = xiiDirectoryWatcherAction::Added;
              fireEvent = whatToWatch.IsSet(xiiDirectoryWatcher::Watch::Creates);
              if (mirror)
              {
                bool fileAlreadyExists = false;
                mirror->AddFile(eventFilePath.GetData(), true, &fileAlreadyExists, nullptr).AssertSuccess();
                if (fileAlreadyExists)
                {
                  fireEvent = false;
                }
              }
              break;
            case FILE_ACTION_REMOVED:
              DEBUG_LOG("FILE_ACTION_REMOVED {} ({})", eventFilePath, info->LastModificationTime.QuadPart);
              action    = xiiDirectoryWatcherAction::Removed;
              fireEvent = whatToWatch.IsSet(xiiDirectoryWatcher::Watch::Deletes);
              if (mirror)
              {
                mirror->RemoveFile(eventFilePath.GetData()).AssertSuccess();
              }
              break;
            case FILE_ACTION_MODIFIED:
            {
              DEBUG_LOG("FILE_ACTION_MODIFIED {} ({})", eventFilePath, info->LastModificationTime.QuadPart);
              action               = xiiDirectoryWatcherAction::Modified;
              fireEvent            = whatToWatch.IsAnySet(xiiDirectoryWatcher::Watch::Writes);
              bool fileAreadyKnown = false;
              bool addPending      = false;
              if (mirror)
              {
                mirror->AddFile(eventFilePath.GetData(), false, &fileAreadyKnown, &addPending).AssertSuccess();
              }
              if (fileAreadyKnown && addPending)
              {
                fireEvent = false;
              }
            }
            break;
            case FILE_ACTION_RENAMED_OLD_NAME:
              DEBUG_LOG("FILE_ACTION_RENAMED_OLD_NAME {} ({})", eventFilePath, info->LastModificationTime.QuadPart);
              XII_ASSERT_DEV(lastMoveFrom.IsEmpty(), "there should be no pending move from");
              action    = xiiDirectoryWatcherAction::RenamedOldName;
              fireEvent = whatToWatch.IsAnySet(xiiDirectoryWatcher::Watch::Renames);
              XII_ASSERT_DEV(lastMoveFrom.IsEmpty(), "there should be no pending last move from");
              lastMoveFrom = {eventFilePath, false};
              break;
            case FILE_ACTION_RENAMED_NEW_NAME:
              DEBUG_LOG("FILE_ACTION_RENAMED_NEW_NAME {} ({})", eventFilePath, info->LastModificationTime.QuadPart);
              action    = xiiDirectoryWatcherAction::RenamedNewName;
              fireEvent = whatToWatch.IsAnySet(xiiDirectoryWatcher::Watch::Renames);
              XII_ASSERT_DEV(!lastMoveFrom.IsEmpty() && !lastMoveFrom.isDirectory, "last move from doesn't match");
              if (mirror)
              {
                mirror->RemoveFile(lastMoveFrom.path).AssertSuccess();
                mirror->AddFile(eventFilePath, false, nullptr, nullptr).AssertSuccess();
              }
              lastMoveFrom.Clear();
              break;
          }

          if (fireEvent)
          {
            func(eventFilePath, action, xiiDirectoryWatcherType::File);
          }
        }
        else
        {
          switch (info->Action)
          {
            case FILE_ACTION_ADDED:
            {
              DEBUG_LOG("DIR_ACTION_ADDED {}", eventFilePath);
              bool directoryAlreadyKnown = false;
              if (mirror)
              {
                mirror->AddDirectory(eventFilePath, &directoryAlreadyKnown).AssertSuccess();
              }

              if (whatToWatch.IsSet(Watch::Creates) && !directoryAlreadyKnown)
              {
                func(eventFilePath, xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::Directory);
              }

              // Whenever we add a directory we might be "to late" to see changes inside it.
              // So iterate the file system and make sure we track all files / subdirectories
              xiiFileSystemIterator subdirIt;

              subdirIt.StartSearch(eventFilePath.GetData(),
                                   whatToWatch.IsSet(xiiDirectoryWatcher::Watch::Subdirectories) ? xiiFileSystemIteratorFlags::ReportFilesAndFoldersRecursive : xiiFileSystemIteratorFlags::ReportFiles);

              xiiStringBuilder tmpPath2;
              for (; subdirIt.IsValid(); subdirIt.Next())
              {
                const xiiFileStats& stats = subdirIt.GetStats();
                stats.GetFullPath(tmpPath2);
                if (stats.m_bIsDirectory)
                {
                  directoryAlreadyKnown = false;
                  if (mirror)
                  {
                    mirror->AddDirectory(tmpPath2, &directoryAlreadyKnown).AssertSuccess();
                  }
                  if (whatToWatch.IsSet(xiiDirectoryWatcher::Watch::Creates) && !directoryAlreadyKnown)
                  {
                    func(tmpPath2, xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::Directory);
                  }
                }
                else
                {
                  bool fileExistsAlready = false;
                  if (mirror)
                  {
                    mirror->AddFile(tmpPath2, false, &fileExistsAlready, nullptr).AssertSuccess();
                  }
                  if (whatToWatch.IsSet(xiiDirectoryWatcher::Watch::Creates) && !fileExistsAlready)
                  {
                    func(tmpPath2, xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::File);
                  }
                }
              }
            }
            break;
            case FILE_ACTION_REMOVED:
              DEBUG_LOG("DIR_ACTION_REMOVED {}", eventFilePath);
              if (whatToWatch.IsSet(Watch::Deletes))
              {
                if (mirror && whatToWatch.IsSet(Watch::Subdirectories))
                {
                  mirror->Enumerate(eventFilePath, [&](const xiiStringBuilder& sPath, xiiFileSystemMirrorType::Type type) {
                          func(sPath, xiiDirectoryWatcherAction::Removed, (type == xiiFileSystemMirrorType::Type::File) ? xiiDirectoryWatcherType::File : xiiDirectoryWatcherType::Directory);
                        })
                    .AssertSuccess();
                }
                func(eventFilePath, xiiDirectoryWatcherAction::Removed, xiiDirectoryWatcherType::Directory);
              }
              if (mirror)
              {
                mirror->RemoveDirectory(eventFilePath).AssertSuccess();
              }
              break;
            case FILE_ACTION_RENAMED_OLD_NAME:
              DEBUG_LOG("DIR_ACTION_OLD_NAME {}", eventFilePath);
              XII_ASSERT_DEV(lastMoveFrom.IsEmpty(), "there should be no pending move from");
              lastMoveFrom = {eventFilePath, true};
              break;
            case FILE_ACTION_RENAMED_NEW_NAME:
              DEBUG_LOG("DIR_ACTION_NEW_NAME {}", eventFilePath);
              XII_ASSERT_DEV(!lastMoveFrom.IsEmpty(), "rename old name and rename new name should always appear in pairs");
              if (mirror)
              {
                mirror->MoveDirectory(lastMoveFrom.path, eventFilePath).AssertSuccess();
              }
              if (whatToWatch.IsSet(Watch::Renames))
              {
                func(lastMoveFrom.path, xiiDirectoryWatcherAction::RenamedOldName, xiiDirectoryWatcherType::Directory);
                func(eventFilePath, xiiDirectoryWatcherAction::RenamedNewName, xiiDirectoryWatcherType::Directory);
              }
              lastMoveFrom.Clear();
              break;
            default:
              break;
          }
        }
      }
      if (info->NextEntryOffset == 0)
        break;
      else
        info = (const FILE_NOTIFY_EXTENDED_INFORMATION*)(((xiiUInt8*)info) + info->NextEntryOffset);
    }
  }
}


void xiiDirectoryWatcher::EnumerateChanges(xiiArrayPtr<xiiDirectoryWatcher*> watchers, EnumerateChangesFunction func, xiiTime waitUpTo)
{
  xiiHybridArray<HANDLE, 16> events;
  events.SetCount(watchers.GetCount());

  for (xiiUInt32 i = 0; i < watchers.GetCount(); ++i)
  {
    events[i] = watchers[i]->m_pImpl->m_overlappedEvent;
  }

  // Wait for any of the watchers to have some data ready
  if (WaitForMultipleObjects(events.GetCount(), events.GetData(), FALSE, static_cast<DWORD>(waitUpTo.GetMilliseconds())) == WAIT_TIMEOUT)
  {
    return;
  }

  // Iterate all of them to make sure we report all changes up to this point.
  for (xiiDirectoryWatcher* watcher : watchers)
  {
    watcher->EnumerateChanges(func);
  }
}
