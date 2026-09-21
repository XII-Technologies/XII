/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/UniquePtr.h>

// A general problem when implementing a directory watcher is, that moving a folder out of the watched directory only communicates
// Which folder was moved (but not to where, nor its contents). This means when a folder is moved out of view,
// this needs to be treated as a delete. At the point of the move, it is no longer possible to query the contents of the folder.
// So a in memory copy of the file system is required in order to correctly implement a directory watcher.
template <typename T>
class xiiFileSystemMirror
{
public:
  enum class Type
  {
    File,
    Directory
  };

  struct DirEntry
  {
    xiiMap<xiiString, DirEntry> m_subDirectories;
    xiiMap<xiiString, T>        m_files;
  };

  xiiFileSystemMirror();
  ~xiiFileSystemMirror();

  /// Adds the directory, and all files in it recursively.
  xiiResult AddDirectory(xiiStringView sPath, bool* out_pDirectoryExistsAlready = nullptr);

  /// Adds a file. Creates directories if they do not exist.
  xiiResult AddFile(xiiStringView sPath, const T& value, bool* out_pFileExistsAlready, T* out_pOldValue);

  /// Removes a file.
  xiiResult RemoveFile(xiiStringView sPath);

  /// Removes a directory. Deletes any files & directories inside.
  xiiResult RemoveDirectory(xiiStringView sPath);

  /// Moves a directory. Any files & folders inside are moved with it.
  xiiResult MoveDirectory(xiiStringView sFromPath, xiiStringView sToPath);

  using EnumerateFunc = xiiDelegate<void(const xiiStringBuilder& path, Type type)>;

  /// Enumerates the files & directories under the given path
  xiiResult Enumerate(xiiStringView sPath, EnumerateFunc callbackFunc);

  /// On success, out_Type will contains the type of the object (file or folder).
  xiiResult GetType(xiiStringView sPath, Type& out_type);

private:
  DirEntry* FindDirectory(xiiStringBuilder& path);

private:
  DirEntry  m_TopLevelDir;
  xiiString m_sTopLevelDirPath;
};

namespace
{
  void EnsureTrailingSlash(xiiStringBuilder& ref_sBuilder)
  {
    if (!ref_sBuilder.EndsWith("/"))
    {
      ref_sBuilder.Append("/");
    }
  }

  void RemoveTrailingSlash(xiiStringBuilder& ref_sBuilder)
  {
    if (ref_sBuilder.EndsWith("/"))
    {
      ref_sBuilder.Shrink(0, 1);
    }
  }
} // namespace

template <typename T>
xiiFileSystemMirror<T>::xiiFileSystemMirror() = default;

template <typename T>
xiiFileSystemMirror<T>::~xiiFileSystemMirror() = default;

template <typename T>
xiiResult xiiFileSystemMirror<T>::AddDirectory(xiiStringView sPath, bool* out_pDirectoryExistsAlready)
{
  xiiStringBuilder currentDirAbsPath = sPath;
  currentDirAbsPath.MakeCleanPath();
  EnsureTrailingSlash(currentDirAbsPath);

  if (m_sTopLevelDirPath.IsEmpty())
  {
    m_sTopLevelDirPath = currentDirAbsPath;
    currentDirAbsPath.Shrink(0, 1); // remove trailing /

    DirEntry* currentDir = &m_TopLevelDir;

    xiiHybridArray<DirEntry*, 16> m_dirStack;

    xiiFileSystemIterator files;
    files.StartSearch(currentDirAbsPath.GetData(), xiiFileSystemIteratorFlags::ReportFilesAndFoldersRecursive);
    for (; files.IsValid(); files.Next())
    {
      const xiiFileStats& stats = files.GetStats();

      // In case we are done with a directory, move back up
      while (currentDirAbsPath != stats.m_sParentPath)
      {
        XII_ASSERT_DEV(m_dirStack.GetCount() > 0, "Unexpected file iteration order");
        currentDir = m_dirStack.PeekBack();
        m_dirStack.PopBack();
        currentDirAbsPath.PathParentDirectory();
        RemoveTrailingSlash(currentDirAbsPath);
      }

      if (stats.m_bIsDirectory)
      {
        m_dirStack.PushBack(currentDir);
        xiiStringBuilder subdirName = stats.m_sName;
        EnsureTrailingSlash(subdirName);
        auto insertIt = currentDir->m_subDirectories.Insert(subdirName, DirEntry());
        currentDir    = &insertIt.Value();
        currentDirAbsPath.AppendPath(stats.m_sName);
      }
      else
      {
        currentDir->m_files.Insert(std::move(stats.m_sName), T{});
      }
    }
    if (out_pDirectoryExistsAlready != nullptr)
    {
      *out_pDirectoryExistsAlready = false;
    }
  }
  else
  {
    DirEntry* parentDir = FindDirectory(currentDirAbsPath);
    if (parentDir == nullptr)
    {
      return XII_FAILURE;
    }

    if (out_pDirectoryExistsAlready != nullptr)
    {
      *out_pDirectoryExistsAlready = currentDirAbsPath.IsEmpty();
    }

    while (!currentDirAbsPath.IsEmpty())
    {
      const char*   dirEnd = currentDirAbsPath.FindSubString("/");
      xiiStringView subdirName(currentDirAbsPath.GetData(), dirEnd + 1);
      auto          insertIt = parentDir->m_subDirectories.Insert(subdirName, DirEntry());
      parentDir              = &insertIt.Value();
      currentDirAbsPath.Shrink(xiiStringUtils::GetCharacterCount(subdirName.GetStartPointer(), subdirName.GetEndPointer()), 0);
    }
  }

  return XII_SUCCESS;
}

template <typename T>
xiiResult xiiFileSystemMirror<T>::AddFile(xiiStringView sPath, const T& value, bool* out_pFileExistsAlready, T* out_pOldValue)
{
  xiiStringBuilder sPathBuilder = sPath;
  DirEntry*        dir          = FindDirectory(sPathBuilder);
  if (dir == nullptr)
  {
    return XII_FAILURE; // file not under top level directory
  }

  const char* szSlashPos = sPathBuilder.FindSubString("/");

  while (szSlashPos != nullptr)
  {
    xiiStringView subdirName(sPathBuilder.GetData(), szSlashPos + 1);
    auto          insertIt = dir->m_subDirectories.Insert(subdirName, DirEntry());
    dir                    = &insertIt.Value();
    sPathBuilder.Shrink(xiiStringUtils::GetCharacterCount(subdirName.GetStartPointer(), subdirName.GetEndPointer()), 0);
    szSlashPos = sPathBuilder.FindSubString("/");
  }

  auto it = dir->m_files.Find(sPathBuilder);
  // Do not add the file twice
  if (!it.IsValid())
  {
    dir->m_files.Insert(sPathBuilder, value);
    if (out_pFileExistsAlready != nullptr)
    {
      *out_pFileExistsAlready = false;
    }
  }
  else
  {
    if (out_pFileExistsAlready != nullptr)
    {
      *out_pFileExistsAlready = true;
    }
    if (out_pOldValue != nullptr)
    {
      *out_pOldValue = it.Value();
    }
    it.Value() = value;
  }
  return XII_SUCCESS;
}

template <typename T>
xiiResult xiiFileSystemMirror<T>::RemoveFile(xiiStringView sPath)
{
  xiiStringBuilder sPathBuilder = sPath;
  DirEntry*        dir          = FindDirectory(sPathBuilder);
  if (dir == nullptr)
  {
    return XII_FAILURE; // file not under top level directory
  }

  if (sPathBuilder.FindSubString("/") != nullptr)
  {
    return XII_FAILURE; // file does not exist
  }

  if (dir->m_files.GetCount() == 0)
  {
    return XII_FAILURE; // there are no files in this directory
  }

  auto it = dir->m_files.Find(sPathBuilder);
  if (!it.IsValid())
  {
    return XII_FAILURE; // file does not exist
  }

  dir->m_files.Remove(it);
  return XII_SUCCESS;
}

template <typename T>
xiiResult xiiFileSystemMirror<T>::RemoveDirectory(xiiStringView sPath)
{
  xiiStringBuilder parentPath = sPath;
  xiiStringBuilder dirName    = sPath;
  parentPath.PathParentDirectory();
  EnsureTrailingSlash(parentPath);
  dirName.Shrink(parentPath.GetCharacterCount(), 0);
  EnsureTrailingSlash(dirName);

  DirEntry* parentDir = FindDirectory(parentPath);
  if (parentDir == nullptr || !parentPath.IsEmpty())
  {
    return XII_FAILURE;
  }

  if (!parentDir->m_subDirectories.Remove(dirName))
  {
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

template <typename T>
xiiResult xiiFileSystemMirror<T>::MoveDirectory(xiiStringView sFromPath, xiiStringView sToPath)
{
  xiiStringBuilder sFromPathBuilder = sFromPath;
  xiiStringBuilder sFromName        = sFromPath;
  sFromPathBuilder.PathParentDirectory();
  EnsureTrailingSlash(sFromPathBuilder);
  sFromName.Shrink(sFromPathBuilder.GetCharacterCount(), 0);
  EnsureTrailingSlash(sFromName);


  xiiStringBuilder sToPathBuilder = sToPath;
  xiiStringBuilder sToName        = sToPath;
  sToPathBuilder.PathParentDirectory();
  EnsureTrailingSlash(sToPathBuilder);
  sToName.Shrink(sToPathBuilder.GetCharacterCount(), 0);
  EnsureTrailingSlash(sToName);

  DirEntry* moveFromDir = FindDirectory(sFromPathBuilder);
  if (!moveFromDir)
  {
    return XII_FAILURE;
  }
  XII_ASSERT_DEV(sFromPathBuilder.IsEmpty(), "move from directory should fully exist");

  DirEntry* moveToDir = FindDirectory(sToPathBuilder);
  if (!moveToDir)
  {
    return XII_FAILURE;
  }

  if (!sToPathBuilder.IsEmpty())
  {
    do
    {
      const char*   dirEnd = sToPathBuilder.FindSubString("/");
      xiiStringView subdirName(sToPathBuilder.GetData(), dirEnd + 1);
      auto          insertIt = moveToDir->m_subDirectories.Insert(subdirName, DirEntry());
      moveToDir              = &insertIt.Value();
      sToPathBuilder.Shrink(0, xiiStringUtils::GetCharacterCount(subdirName.GetStartPointer(), subdirName.GetEndPointer()));
    } while (!sToPathBuilder.IsEmpty());
  }

  DirEntry movedDir;
  {
    auto fromIt = moveFromDir->m_subDirectories.Find(sFromName);
    if (!fromIt.IsValid())
    {
      return XII_FAILURE;
    }

    movedDir = std::move(fromIt.Value());
    moveFromDir->m_subDirectories.Remove(fromIt);
  }

  moveToDir->m_subDirectories.Insert(sToName, std::move(movedDir));

  return XII_SUCCESS;
}

namespace
{
  template <typename T>
  struct xiiDirEnumerateState
  {
    typename xiiFileSystemMirror<T>::DirEntry*                                      dir;
    typename xiiMap<xiiString, typename xiiFileSystemMirror<T>::DirEntry>::Iterator subDirIt;
  };
} // namespace

template <typename T>
xiiResult xiiFileSystemMirror<T>::Enumerate(xiiStringView sPath, EnumerateFunc callbackFunc)
{
  xiiHybridArray<xiiDirEnumerateState<T>, 16> dirStack;
  xiiStringBuilder                            sPathBuilder = sPath;
  if (!sPathBuilder.EndsWith("/"))
  {
    sPathBuilder.Append("/");
  }
  DirEntry* dirToEnumerate = FindDirectory(sPathBuilder);
  if (dirToEnumerate == nullptr)
  {
    return XII_FAILURE;
  }
  if (!sPathBuilder.IsEmpty())
  {
    return XII_FAILURE; // requested folder to enumerate doesn't exist
  }
  DirEntry*                                                           currentDir      = dirToEnumerate;
  typename xiiMap<xiiString, xiiFileSystemMirror::DirEntry>::Iterator currentSubDirIt = currentDir->m_subDirectories.GetIterator();
  sPathBuilder                                                                        = sPath;

  while (currentDir != nullptr)
  {
    if (currentSubDirIt.IsValid())
    {
      DirEntry* nextDir = &currentSubDirIt.Value();
      sPathBuilder.AppendPath(currentSubDirIt.Key());
      currentSubDirIt.Next();
      dirStack.PushBack({currentDir, currentSubDirIt});
      currentDir = nextDir;
    }
    else
    {
      xiiStringBuilder sFilePath;
      for (auto& file : currentDir->m_files)
      {
        sFilePath = sPathBuilder;
        sFilePath.AppendPath(file.Key());
        callbackFunc(sFilePath, Type::File);
      }

      if (currentDir != dirToEnumerate)
      {
        if (sPathBuilder.EndsWith("/") && sPathBuilder.GetElementCount() > 1)
        {
          sPathBuilder.Shrink(0, 1);
        }
        callbackFunc(sPathBuilder, Type::Directory);
      }

      if (dirStack.IsEmpty())
      {
        currentDir = nullptr;
      }
      else
      {
        currentDir      = dirStack.PeekBack().dir;
        currentSubDirIt = dirStack.PeekBack().subDirIt;
        dirStack.PopBack();
        sPathBuilder.PathParentDirectory();
        if (sPathBuilder.GetElementCount() > 1 && sPathBuilder.EndsWith("/"))
        {
          sPathBuilder.Shrink(0, 1);
        }
      }
    }
  }

  return XII_SUCCESS;
}

template <typename T>
xiiResult xiiFileSystemMirror<T>::GetType(xiiStringView sPath, Type& out_type)
{
  xiiStringBuilder sPathBuilder = sPath;
  DirEntry*        dir          = FindDirectory(sPathBuilder);
  if (dir == nullptr)
  {
    return XII_FAILURE; // file not under top level directory
  }

  auto it = dir->m_files.Find(sPathBuilder);
  if (it.IsValid())
  {
    out_type = xiiFileSystemMirror::Type::File;
    return XII_SUCCESS;
  }

  auto itDir = dir->m_subDirectories.Find(sPathBuilder);
  if (itDir.IsValid())
  {
    out_type = xiiFileSystemMirror::Type::Directory;
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

template <typename T>
typename xiiFileSystemMirror<T>::DirEntry* xiiFileSystemMirror<T>::FindDirectory(xiiStringBuilder& path)
{
  if (!path.StartsWith(m_sTopLevelDirPath))
  {
    return nullptr;
  }
  path.TrimWordStart(m_sTopLevelDirPath);

  DirEntry* currentDir = &m_TopLevelDir;

  bool found = false;
  do
  {
    found = false;
    for (auto& dir : currentDir->m_subDirectories)
    {
      if (path.StartsWith(dir.Key()))
      {
        currentDir = &dir.Value();
        path.TrimWordStart(dir.Key());
        path.TrimWordStart("/");
        found = true;
        break;
      }
    }
  } while (found);

  return currentDir;
}
