/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/IO/Implementation/FileSystemMirror.h>
#include <Foundation/Platform/Implementation/Windows/DosDevicePath_win.h>

namespace
{
  // Resolve OLD_NAME-only case changes while the object exists, or recognize a
  // delayed record against the mirror even if the renamed object was deleted.
  bool ReconcileCaseRename(xiiFileSystemMirror<bool>& mirror, xiiStringView oldPath,
                           xiiString& out_sNewPath, xiiDirectoryWatcher::EnumerateChangesFunction callback)
  {
    xiiStringBuilder                cached, corrected;
    xiiFileSystemMirror<bool>::Type type;
    if (mirror.GetPathCasing(oldPath, cached, type).Failed())
      return false;
    xiiFileStats stats;
    if (xiiOSFile::GetFileStats(oldPath, stats).Failed())
    {
      if (cached.GetFileNameAndExtension() == oldPath.GetFileNameAndExtension())
        return false;
      out_sNewPath = cached;
      return true;
    }
    if (!stats.m_sName.IsEqual_NoCase(oldPath.GetFileNameAndExtension()))
      return false;
    // Repair the leaf in its cached parent. An ancestor may itself still need
    // repair; creating a second parent here would split the cached subtree.
    corrected = cached.GetFileDirectory();
    corrected.AppendPath(stats.m_sName);
    if (cached != corrected)
    {
      const bool directory = type == xiiFileSystemMirror<bool>::Type::Directory;
      if (directory)
      {
        if (mirror.MoveDirectory(cached, corrected).Failed())
          return false;
      }
      else
      {
        if (mirror.MoveFile(cached, corrected).Failed())
          return false;
      }
      const auto eventType = directory ? xiiDirectoryWatcherType::Directory : xiiDirectoryWatcherType::File;
      callback(cached, xiiDirectoryWatcherAction::RenamedOldName, eventType);
      callback(corrected, xiiDirectoryWatcherAction::RenamedNewName, eventType);
    }
    out_sNewPath = corrected;
    return true;
  }

  // ReFS can withhold case-only notifications indefinitely. Bound metadata queries
  // per poll; file contents are never read. Each batch walks the cached tree,
  // O(N), and checks at most 128 paths. The caller limits batches to 100 ms.
  void ReconcileMirrorPaths(xiiFileSystemMirror<bool>& mirror, xiiStringView root, xiiUInt32& nextEntry, bool recursive,
                            xiiDirectoryWatcher::EnumerateChangesFunction callback)
  {
    struct Mutation
    {
      xiiString               m_sOldPath;
      xiiString               m_sNewPath;
      xiiDirectoryWatcherType m_Type;
      bool                    m_bRemoved = false;
    };
    xiiTemporaryHybridArray<Mutation, 4> changes;
    xiiUInt32                            ordinal   = 0;
    xiiUInt32                            checked   = 0;
    constexpr xiiUInt32                  batchSize = 128;
    const auto                           result    = mirror.Enumerate(root, [&](const xiiStringBuilder& path, xiiFileSystemMirror<bool>::Type type) {
      if (!recursive && path.GetFileDirectory().GetElementCount() != root.GetElementCount() + 1)
        return;
      const xiiUInt32 current = ordinal++;
      if (current < nextEntry || checked >= batchSize)
        return;
      ++checked;
      xiiFileStats stats;
      if (xiiOSFile::GetFileStats(path, stats).Failed())
      {
        // Do not interpret inaccessible objects as deleted. Confirm a missing
        // path using the native error, which the portable stats result omits.
        const DWORD attributes = GetFileAttributesW(xiiDosDevicePath(path));
        const DWORD error = GetLastError();
        if (attributes == INVALID_FILE_ATTRIBUTES && (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND))
          changes.PushBack({path, {}, type == xiiFileSystemMirror<bool>::Type::Directory ? xiiDirectoryWatcherType::Directory : xiiDirectoryWatcherType::File, true});
        return;
      }
      if (stats.m_bIsDirectory != (type == xiiFileSystemMirror<bool>::Type::Directory) ||
          path.GetFileNameAndExtension() == stats.m_sName || !path.GetFileNameAndExtension().IsEqual_NoCase(stats.m_sName))
        return;
      xiiStringBuilder corrected = path;
      corrected.PathParentDirectory();
      corrected.AppendPath(stats.m_sName);
      changes.PushBack({path, corrected, stats.m_bIsDirectory ? xiiDirectoryWatcherType::Directory : xiiDirectoryWatcherType::File}); });
    nextEntry                                      = nextEntry + checked >= ordinal ? 0 : nextEntry + checked;
    if (result.Failed())
      return;

    // Enumeration is read-only; apply child changes before parent changes after it ends.
    for (const auto& rename : changes)
    {
      if (rename.m_bRemoved)
      {
        // A parent can disappear after its children were checked in an earlier
        // batch. Report any remaining descendants before removing the subtree.
        if (recursive && rename.m_Type == xiiDirectoryWatcherType::Directory)
          mirror.Enumerate(rename.m_sOldPath, [&](const xiiStringBuilder& path, xiiFileSystemMirror<bool>::Type type) {
                  callback(path, xiiDirectoryWatcherAction::Removed, type == xiiFileSystemMirror<bool>::Type::Directory ? xiiDirectoryWatcherType::Directory : xiiDirectoryWatcherType::File);
                })
            .IgnoreResult();
        const auto removeResult = rename.m_Type == xiiDirectoryWatcherType::Directory ?
          mirror.RemoveDirectory(rename.m_sOldPath) :
          mirror.RemoveFile(rename.m_sOldPath);
        if (removeResult.Succeeded())
          callback(rename.m_sOldPath, xiiDirectoryWatcherAction::Removed, rename.m_Type);
        continue;
      }
      const auto moveResult = rename.m_Type == xiiDirectoryWatcherType::Directory ?
        mirror.MoveDirectory(rename.m_sOldPath, rename.m_sNewPath) :
        mirror.MoveFile(rename.m_sOldPath, rename.m_sNewPath);
      if (moveResult.Succeeded())
      {
        callback(rename.m_sOldPath, xiiDirectoryWatcherAction::RenamedOldName, rename.m_Type);
        callback(rename.m_sNewPath, xiiDirectoryWatcherAction::RenamedNewName, rename.m_Type);
      }
    }
  }
} // namespace
