/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/UniquePtr.h>

// A general problem when implementing a directory watcher is, that moving a folder out of the watched directory only communicates the fact that a folder was moved (but not to where, nor its contents).
// This means when a folder is moved out of view, this needs to be treated as a delete. At the point of the move, it is no longer possible to query the contents of the folder.
// So an in-memory copy of the file system is required in order to correctly implement a directory watcher.
template <typename T>
class xiiFileSystemMirror
{
public:
  enum class Type
  {
    File,
    Directory
  };

  struct DirectoryEntry
  {
    xiiMap<xiiString, DirectoryEntry> m_SubDirectories;
    xiiMap<xiiString, T>              m_Files;
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

  /// Moves a file, preserving its associated value.
  xiiResult MoveFile(xiiStringView sFromPath, xiiStringView sToPath);

  /// Moves a directory. Any files & folders inside are moved with it.
  xiiResult MoveDirectory(xiiStringView sFromPath, xiiStringView sToPath);

  using EnumerateFunc = xiiDelegate<void(const xiiStringBuilder& path, Type type)>;

  /// Enumerates files and directories under the given path in post-order.
  xiiResult Enumerate(xiiStringView sPath, EnumerateFunc callbackFunc);

  /// On success, out_Type will contains the type of the object (file or folder).
  xiiResult GetType(xiiStringView sPath, Type& out_type);

  /// Resolves an existing cached path using case-insensitive component matching.
  /// Intended for filesystems whose notifications may use obsolete casing.
  xiiResult GetPathCasing(xiiStringView sPath, xiiStringBuilder& out_sPath, Type& out_type) const;

private:
  DirectoryEntry* FindDirectory(xiiStringBuilder& path);

private:
  DirectoryEntry m_TopLevelDirectory;
  xiiString      m_sTopLevelDirectoryPath;
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
  xiiStringBuilder sCurrentDirectoryAbsolutePath = sPath;
  sCurrentDirectoryAbsolutePath.MakeCleanPath();
  EnsureTrailingSlash(sCurrentDirectoryAbsolutePath);

  if (m_sTopLevelDirectoryPath.IsEmpty())
  {
    m_sTopLevelDirectoryPath = sCurrentDirectoryAbsolutePath;
    sCurrentDirectoryAbsolutePath.Shrink(0, 1); // Remove trailing '/'

    DirectoryEntry* pCurrentDirectory = &m_TopLevelDirectory;

    xiiTemporaryHybridArray<DirectoryEntry*, 16> directoryEntryStack;

    xiiFileSystemIterator files;
    files.StartSearch(sCurrentDirectoryAbsolutePath, xiiFileSystemIteratorFlags::ReportFilesAndFoldersRecursive);
    for (; files.IsValid(); files.Next())
    {
      const xiiFileStats& stats = files.GetStats();

      // In case we are done with a directory, move back up
      while (sCurrentDirectoryAbsolutePath != stats.m_sParentPath)
      {
        XII_ASSERT_DEV(directoryEntryStack.GetCount() > 0, "Unexpected file iteration order");

        pCurrentDirectory = directoryEntryStack.PeekBack();
        directoryEntryStack.PopBack();

        sCurrentDirectoryAbsolutePath.PathParentDirectory();

        RemoveTrailingSlash(sCurrentDirectoryAbsolutePath);
      }

      if (stats.m_bIsDirectory)
      {
        directoryEntryStack.PushBack(pCurrentDirectory);

        xiiStringBuilder sSubDirectoryName = stats.m_sName;
        EnsureTrailingSlash(sSubDirectoryName);

        auto insertIt     = pCurrentDirectory->m_SubDirectories.Insert(sSubDirectoryName, DirectoryEntry());
        pCurrentDirectory = &insertIt.Value();
        sCurrentDirectoryAbsolutePath.AppendPath(stats.m_sName);
      }
      else
      {
        pCurrentDirectory->m_Files.Insert(std::move(stats.m_sName), T{});
      }
    }
    if (out_pDirectoryExistsAlready != nullptr)
    {
      *out_pDirectoryExistsAlready = false;
    }
  }
  else
  {
    DirectoryEntry* pParentDirectory = FindDirectory(sCurrentDirectoryAbsolutePath);
    if (pParentDirectory == nullptr)
    {
      return XII_FAILURE;
    }

    if (out_pDirectoryExistsAlready != nullptr)
    {
      *out_pDirectoryExistsAlready = sCurrentDirectoryAbsolutePath.IsEmpty();
    }

    while (!sCurrentDirectoryAbsolutePath.IsEmpty())
    {
      const char*   pDirectoryNameEnd = sCurrentDirectoryAbsolutePath.FindSubString("/");
      xiiStringView sSubDirectoryName(sCurrentDirectoryAbsolutePath.GetData(), pDirectoryNameEnd + 1);

      auto insertIt    = pParentDirectory->m_SubDirectories.Insert(sSubDirectoryName, DirectoryEntry());
      pParentDirectory = &insertIt.Value();

      sCurrentDirectoryAbsolutePath.Shrink(xiiStringUtils::GetCharacterCount(sSubDirectoryName.GetStartPointer(), sSubDirectoryName.GetEndPointer()), 0);
    }
  }

  return XII_SUCCESS;
}

template <typename T>
xiiResult xiiFileSystemMirror<T>::AddFile(xiiStringView sPath, const T& value, bool* out_pFileExistsAlready, T* out_pOldValue)
{
  xiiStringBuilder sPathBuilder = sPath;
  DirectoryEntry*  pDirectory   = FindDirectory(sPathBuilder);
  if (pDirectory == nullptr)
  {
    return XII_FAILURE; // file not under top level directory
  }

  const char* szSlashPos = sPathBuilder.FindSubString("/");

  while (szSlashPos != nullptr)
  {
    xiiStringView sSubDirectoryName(sPathBuilder.GetData(), szSlashPos + 1);

    auto insertIt = pDirectory->m_SubDirectories.Insert(sSubDirectoryName, DirectoryEntry());
    pDirectory    = &insertIt.Value();

    sPathBuilder.Shrink(xiiStringUtils::GetCharacterCount(sSubDirectoryName.GetStartPointer(), sSubDirectoryName.GetEndPointer()), 0);

    szSlashPos = sPathBuilder.FindSubString("/");
  }

  auto it = pDirectory->m_Files.Find(sPathBuilder);
  // Do not add the file twice
  if (!it.IsValid())
  {
    pDirectory->m_Files.Insert(sPathBuilder, value);

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
  DirectoryEntry*  pDirectory   = FindDirectory(sPathBuilder);
  if (pDirectory == nullptr)
  {
    return XII_FAILURE; // file not under top level directory
  }

  if (sPathBuilder.FindSubString("/") != nullptr)
  {
    return XII_FAILURE; // file does not exist
  }

  if (pDirectory->m_Files.GetCount() == 0)
  {
    return XII_FAILURE; // there are no files in this directory
  }

  auto it = pDirectory->m_Files.Find(sPathBuilder);
  if (!it.IsValid())
  {
    return XII_FAILURE; // file does not exist
  }

  pDirectory->m_Files.Remove(it);
  return XII_SUCCESS;
}

template <typename T>
xiiResult xiiFileSystemMirror<T>::RemoveDirectory(xiiStringView sPath)
{
  xiiStringBuilder sParentPath    = sPath;
  xiiStringBuilder sDirectoryName = sPath;

  sParentPath.PathParentDirectory();
  EnsureTrailingSlash(sParentPath);

  sDirectoryName.Shrink(sParentPath.GetCharacterCount(), 0);
  EnsureTrailingSlash(sDirectoryName);

  DirectoryEntry* pParentDirectory = FindDirectory(sParentPath);
  if (pParentDirectory == nullptr || !sParentPath.IsEmpty())
  {
    return XII_FAILURE;
  }

  if (!pParentDirectory->m_SubDirectories.Remove(sDirectoryName))
  {
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

template <typename T>
xiiResult xiiFileSystemMirror<T>::MoveFile(xiiStringView sFromPath, xiiStringView sToPath)
{
  xiiStringBuilder sRelativePath = sFromPath;
  DirectoryEntry*  pDirectory    = FindDirectory(sRelativePath);

  if (!pDirectory)
    return XII_FAILURE;

  auto file = pDirectory->m_Files.Find(sRelativePath);
  if (!file.IsValid())
    return XII_FAILURE;
  if (sFromPath == sToPath)
    return XII_SUCCESS;

  XII_SUCCEED_OR_RETURN(AddFile(sToPath, file.Value(), nullptr, nullptr));

  return RemoveFile(sFromPath);
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

  DirectoryEntry* pMoveFromDirectory = FindDirectory(sFromPathBuilder);
  if (!pMoveFromDirectory)
  {
    return XII_FAILURE;
  }
  XII_ASSERT_DEV(sFromPathBuilder.IsEmpty(), "move from directory should fully exist");

  DirectoryEntry* pMoveToDirectory = FindDirectory(sToPathBuilder);
  if (!pMoveToDirectory)
  {
    return XII_FAILURE;
  }

  if (!sToPathBuilder.IsEmpty())
  {
    do
    {
      const char*   pDirectoryNameEnd = sToPathBuilder.FindSubString("/");
      xiiStringView sSubDirectoryName(sToPathBuilder.GetData(), pDirectoryNameEnd + 1);

      auto insertIt    = pMoveToDirectory->m_SubDirectories.Insert(sSubDirectoryName, DirectoryEntry());
      pMoveToDirectory = &insertIt.Value();

      sToPathBuilder.Shrink(0, xiiStringUtils::GetCharacterCount(sSubDirectoryName.GetStartPointer(), sSubDirectoryName.GetEndPointer()));
    } while (!sToPathBuilder.IsEmpty());
  }

  DirectoryEntry movedDirectory;
  {
    auto fromIt = pMoveFromDirectory->m_SubDirectories.Find(sFromName);
    if (!fromIt.IsValid())
    {
      return XII_FAILURE;
    }

    movedDirectory = std::move(fromIt.Value());
    pMoveFromDirectory->m_SubDirectories.Remove(fromIt);
  }

  pMoveToDirectory->m_SubDirectories.Insert(sToName, std::move(movedDirectory));

  return XII_SUCCESS;
}

namespace
{
  template <typename T>
  struct xiiDirEnumerateState
  {
    typename xiiFileSystemMirror<T>::DirectoryEntry*                                      m_Directory;
    typename xiiMap<xiiString, typename xiiFileSystemMirror<T>::DirectoryEntry>::Iterator m_SubDirectoryIterator;
  };
} // namespace

template <typename T>
xiiResult xiiFileSystemMirror<T>::Enumerate(xiiStringView sPath, EnumerateFunc callbackFunc)
{
  xiiTemporaryHybridArray<xiiDirEnumerateState<T>, 16> directoryStack;

  xiiStringBuilder sPathBuilder = sPath;
  if (!sPathBuilder.EndsWith("/"))
  {
    sPathBuilder.Append("/");
  }
  DirectoryEntry* pDirectoryToEnumerate = FindDirectory(sPathBuilder);
  if (pDirectoryToEnumerate == nullptr)
  {
    return XII_FAILURE;
  }
  if (!sPathBuilder.IsEmpty())
  {
    return XII_FAILURE; // requested folder to enumerate doesn't exist
  }

  DirectoryEntry*                                                           pCurrentDirectory = pDirectoryToEnumerate;
  typename xiiMap<xiiString, xiiFileSystemMirror::DirectoryEntry>::Iterator currentSubDirIt   = pCurrentDirectory->m_SubDirectories.GetIterator();
  sPathBuilder                                                                                = sPath;

  while (pCurrentDirectory != nullptr)
  {
    if (currentSubDirIt.IsValid())
    {
      DirectoryEntry* pNextDirectory = &currentSubDirIt.Value();

      sPathBuilder.AppendPath(currentSubDirIt.Key());

      currentSubDirIt.Next();

      directoryStack.PushBack({pCurrentDirectory, currentSubDirIt});

      pCurrentDirectory = pNextDirectory;
      currentSubDirIt   = pCurrentDirectory->m_SubDirectories.GetIterator();
    }
    else
    {
      xiiStringBuilder sFilePath;

      for (auto& file : pCurrentDirectory->m_Files)
      {
        sFilePath = sPathBuilder;
        sFilePath.AppendPath(file.Key());

        callbackFunc(sFilePath, Type::File);
      }

      if (pCurrentDirectory != pDirectoryToEnumerate)
      {
        if (sPathBuilder.EndsWith("/") && sPathBuilder.GetElementCount() > 1)
        {
          sPathBuilder.Shrink(0, 1);
        }
        callbackFunc(sPathBuilder, Type::Directory);
      }

      if (directoryStack.IsEmpty())
      {
        pCurrentDirectory = nullptr;
      }
      else
      {
        pCurrentDirectory = directoryStack.PeekBack().m_Directory;
        currentSubDirIt   = directoryStack.PeekBack().m_SubDirectoryIterator;

        directoryStack.PopBack();

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
  DirectoryEntry*  pDirectory   = FindDirectory(sPathBuilder);

  if (pDirectory == nullptr)
  {
    return XII_FAILURE; // file not under top level directory
  }

  auto it = pDirectory->m_Files.Find(sPathBuilder);
  if (it.IsValid())
  {
    out_type = xiiFileSystemMirror::Type::File;
    return XII_SUCCESS;
  }

  EnsureTrailingSlash(sPathBuilder);

  auto itDir = pDirectory->m_SubDirectories.Find(sPathBuilder);
  if (itDir.IsValid())
  {
    out_type = xiiFileSystemMirror::Type::Directory;
    return XII_SUCCESS;
  }
  return XII_FAILURE;
}

template <typename T>
typename xiiFileSystemMirror<T>::DirectoryEntry* xiiFileSystemMirror<T>::FindDirectory(xiiStringBuilder& path)
{
  if (!path.StartsWith(m_sTopLevelDirectoryPath))
  {
    return nullptr;
  }
  path.TrimWordStart(m_sTopLevelDirectoryPath);

  DirectoryEntry* pCurrentDirectory = &m_TopLevelDirectory;

  bool bFound = false;
  do
  {
    bFound = false;

    for (auto& directory : pCurrentDirectory->m_SubDirectories)
    {
      if (path.StartsWith(directory.Key()))
      {
        pCurrentDirectory = &directory.Value();

        path.TrimWordStart(directory.Key());
        path.TrimWordStart("/");

        bFound = true;
        break;
      }
    }
  } while (bFound);

  return pCurrentDirectory;
}

template <typename T>
xiiResult xiiFileSystemMirror<T>::GetPathCasing(xiiStringView sPath, xiiStringBuilder& out_sPath, Type& out_type) const
{
  if (m_sTopLevelDirectoryPath.IsEmpty())
    return XII_FAILURE;

  xiiStringBuilder sRemainingPath = sPath;
  sRemainingPath.MakeCleanPath();
  RemoveTrailingSlash(sRemainingPath);

  xiiStringBuilder sResolvedPath = m_sTopLevelDirectoryPath;
  RemoveTrailingSlash(sResolvedPath);

  if (sRemainingPath.IsEqual_NoCase(sResolvedPath))
  {
    out_sPath = sResolvedPath;
    out_type  = Type::Directory;
    return XII_SUCCESS;
  }

  if (!sRemainingPath.StartsWith_NoCase(m_sTopLevelDirectoryPath))
    return XII_FAILURE;

  sRemainingPath.Shrink(m_sTopLevelDirectoryPath.GetCharacterCount(), 0);

  const DirectoryEntry* pDirectory = &m_TopLevelDirectory;
  while (!sRemainingPath.IsEmpty())
  {
    const char*         pSeparator = sRemainingPath.FindSubString("/");
    const xiiStringView sComponent(sRemainingPath.GetData(), pSeparator ? pSeparator : sRemainingPath.GetData() + sRemainingPath.GetElementCount());
    xiiStringBuilder    sKey(sComponent, "/");

    auto childIt = pDirectory->m_SubDirectories.Find(sKey);
    if (!childIt.IsValid())
    {
      for (childIt = pDirectory->m_SubDirectories.GetIterator(); childIt.IsValid(); ++childIt)
      {
        if (childIt.Key().IsEqual_NoCase(sKey))
          break;
      }
    }
    if (childIt.IsValid())
    {
      sResolvedPath.AppendPath(childIt.Key());

      pDirectory = &childIt.Value();

      if (!pSeparator)
      {
        RemoveTrailingSlash(sResolvedPath);

        out_sPath = sResolvedPath;
        out_type  = Type::Directory;
        return XII_SUCCESS;
      }

      sRemainingPath.Shrink(xiiStringUtils::GetCharacterCount(sComponent.GetStartPointer(), sComponent.GetEndPointer()) + 1, 0);
      continue;
    }

    if (pSeparator)
      return XII_FAILURE;

    auto fileIt = pDirectory->m_Files.Find(sComponent);
    if (!fileIt.IsValid())
    {
      for (fileIt = pDirectory->m_Files.GetIterator(); fileIt.IsValid(); ++fileIt)
      {
        if (fileIt.Key().IsEqual_NoCase(sComponent))
          break;
      }
    }

    if (!fileIt.IsValid())
      return XII_FAILURE;

    sResolvedPath.AppendPath(fileIt.Key());

    out_sPath = sResolvedPath;
    out_type  = Type::File;
    return XII_SUCCESS;
  }
  return XII_FAILURE;
}
