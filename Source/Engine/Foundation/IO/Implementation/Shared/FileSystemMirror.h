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

  // \brief Adds the directory, and all files in it recursively.
  xiiResult AddDirectory(const char* path, bool* outDirectoryExistsAlready = nullptr);

  // \brief Adds a file. Creates directories if they do not exist.
  xiiResult AddFile(const char* path, const T& value, bool* outFileExistsAlready, T* outOldValue);

  // \brief Removes a file.
  xiiResult RemoveFile(const char* path);

  // \brief Removes a directory. Deletes any files & directories inside.
  xiiResult RemoveDirectory(const char* path);

  // \brief Moves a directory. Any files & folders inside are moved with it.
  xiiResult MoveDirectory(const char* fromPath, const char* toPath);

  using EnumerateFunc = xiiDelegate<void(const xiiStringBuilder& path, Type type)>;

  // \brief Enumerates the files & directories under the given path
  xiiResult Enumerate(const char* path, EnumerateFunc callbackFunc);

private:
  DirEntry* FindDirectory(xiiStringBuilder& path);
  DirEntry* AddDirectoryImpl(DirEntry* startDir, xiiStringBuilder& path);

private:
  DirEntry  m_TopLevelDir;
  xiiString m_sTopLevelDirPath;
};

namespace
{
  void EnsureTrailingSlash(xiiStringBuilder& builder)
  {
    if (!builder.EndsWith("/"))
    {
      builder.Append("/");
    }
  }

  void RemoveTrailingSlash(xiiStringBuilder& builder)
  {
    if (builder.EndsWith("/"))
    {
      builder.Shrink(0, 1);
    }
  }
} // namespace

template <typename T>
xiiFileSystemMirror<T>::xiiFileSystemMirror() = default;

template <typename T>
xiiFileSystemMirror<T>::~xiiFileSystemMirror() = default;

template <typename T>
xiiResult xiiFileSystemMirror<T>::AddDirectory(const char* path, bool* outDirectoryExistsAlready)
{
  xiiStringBuilder currentDirAbsPath = path;
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
    if (outDirectoryExistsAlready != nullptr)
    {
      *outDirectoryExistsAlready = false;
    }
  }
  else
  {
    DirEntry* parentDir = FindDirectory(currentDirAbsPath);
    if (parentDir == nullptr)
    {
      return XII_FAILURE;
    }

    if (outDirectoryExistsAlready != nullptr)
    {
      *outDirectoryExistsAlready = currentDirAbsPath.IsEmpty();
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
xiiResult xiiFileSystemMirror<T>::AddFile(const char* path, const T& value, bool* outFileExistsAlready, T* outOldValue)
{
  xiiStringBuilder sPath = path;
  DirEntry*        dir   = FindDirectory(sPath);
  if (dir == nullptr)
  {
    return XII_FAILURE; // file not under top level directory
  }

  const char* szSlashPos = sPath.FindSubString("/");

  while (szSlashPos != nullptr)
  {
    xiiStringView subdirName(sPath.GetData(), szSlashPos + 1);
    auto          insertIt = dir->m_subDirectories.Insert(subdirName, DirEntry());
    dir                    = &insertIt.Value();
    sPath.Shrink(xiiStringUtils::GetCharacterCount(subdirName.GetStartPointer(), subdirName.GetEndPointer()), 0);
    szSlashPos = sPath.FindSubString("/");
  }

  auto it = dir->m_files.Find(sPath);
  // Do not add the file twice
  if (!it.IsValid())
  {
    dir->m_files.Insert(sPath, value);
    if (outFileExistsAlready != nullptr)
    {
      *outFileExistsAlready = false;
    }
  }
  else
  {
    if (outFileExistsAlready != nullptr)
    {
      *outFileExistsAlready = true;
    }
    if (outOldValue != nullptr)
    {
      *outOldValue = it.Value();
    }
    it.Value() = value;
  }
  return XII_SUCCESS;
}

template <typename T>
xiiResult xiiFileSystemMirror<T>::RemoveFile(const char* path)
{
  xiiStringBuilder sPath = path;
  DirEntry*        dir   = FindDirectory(sPath);
  if (dir == nullptr)
  {
    return XII_FAILURE; // file not under top level directory
  }

  if (sPath.FindSubString("/") != nullptr)
  {
    return XII_FAILURE; // file does not exist
  }

  if (dir->m_files.GetCount() == 0)
  {
    return XII_FAILURE; // there are no files in this directory
  }

  auto it = dir->m_files.Find(sPath);
  if (!it.IsValid())
  {
    return XII_FAILURE; // file does not exist
  }

  dir->m_files.Remove(it);
  return XII_SUCCESS;
}

template <typename T>
xiiResult xiiFileSystemMirror<T>::RemoveDirectory(const char* path)
{
  xiiStringBuilder parentPath = path;
  xiiStringBuilder dirName    = path;
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
xiiResult xiiFileSystemMirror<T>::MoveDirectory(const char* fromPath, const char* toPath)
{
  xiiStringBuilder sFromPath = fromPath;
  xiiStringBuilder sFromName = fromPath;
  sFromPath.PathParentDirectory();
  EnsureTrailingSlash(sFromPath);
  sFromName.Shrink(sFromPath.GetCharacterCount(), 0);
  EnsureTrailingSlash(sFromName);


  xiiStringBuilder sToPath = toPath;
  xiiStringBuilder sToName = toPath;
  sToPath.PathParentDirectory();
  EnsureTrailingSlash(sToPath);
  sToName.Shrink(sToPath.GetCharacterCount(), 0);
  EnsureTrailingSlash(sToName);

  DirEntry* moveFromDir = FindDirectory(sFromPath);
  if (!moveFromDir)
  {
    return XII_FAILURE;
  }
  XII_ASSERT_DEV(sFromPath.IsEmpty(), "move from directory should fully exist");

  DirEntry* moveToDir = FindDirectory(sToPath);
  if (!moveToDir)
  {
    return XII_FAILURE;
  }

  if (!sToPath.IsEmpty())
  {
    do
    {
      const char*   dirEnd = sToPath.FindSubString("/");
      xiiStringView subdirName(sToPath.GetData(), dirEnd + 1);
      auto          insertIt = moveToDir->m_subDirectories.Insert(subdirName, DirEntry());
      moveToDir              = &insertIt.Value();
      sToPath.Shrink(0, xiiStringUtils::GetCharacterCount(subdirName.GetStartPointer(), subdirName.GetEndPointer()));
    } while (!sToPath.IsEmpty());
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
xiiResult xiiFileSystemMirror<T>::Enumerate(const char* path, EnumerateFunc callbackFunc)
{
  xiiHybridArray<xiiDirEnumerateState<T>, 16> dirStack;
  xiiStringBuilder                            sPath = path;
  if (!sPath.EndsWith("/"))
  {
    sPath.Append("/");
  }
  DirEntry* dirToEnumerate = FindDirectory(sPath);
  if (dirToEnumerate == nullptr)
  {
    return XII_FAILURE;
  }
  if (!sPath.IsEmpty())
  {
    return XII_FAILURE; // requested folder to enumerate doesn't exist
  }
  DirEntry*                                                           currentDir      = dirToEnumerate;
  typename xiiMap<xiiString, xiiFileSystemMirror::DirEntry>::Iterator currentSubDirIt = currentDir->m_subDirectories.GetIterator();
  sPath                                                                               = path;

  while (currentDir != nullptr)
  {
    if (currentSubDirIt.IsValid())
    {
      DirEntry* nextDir = &currentSubDirIt.Value();
      sPath.AppendPath(currentSubDirIt.Key());
      currentSubDirIt.Next();
      dirStack.PushBack({currentDir, currentSubDirIt});
      currentDir = nextDir;
    }
    else
    {
      xiiStringBuilder sFilePath;
      for (auto& file : currentDir->m_files)
      {
        sFilePath = sPath;
        sFilePath.AppendPath(file.Key());
        callbackFunc(sFilePath, Type::File);
      }

      if (currentDir != dirToEnumerate)
      {
        if (sPath.EndsWith("/") && sPath.GetElementCount() > 1)
        {
          sPath.Shrink(0, 1);
        }
        callbackFunc(sPath, Type::Directory);
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
        sPath.PathParentDirectory();
        if (sPath.GetElementCount() > 1 && sPath.EndsWith("/"))
        {
          sPath.Shrink(0, 1);
        }
      }
    }
  }

  return XII_SUCCESS;
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
