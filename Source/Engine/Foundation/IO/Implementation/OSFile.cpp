/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OSFile.h>

xiiString64        xiiOSFile::s_sApplicationPath;
xiiString64        xiiOSFile::s_sUserDataPath;
xiiString64        xiiOSFile::s_sTempDataPath;
xiiString64        xiiOSFile::s_sUserDocumentsPath;
xiiAtomicInteger32 xiiOSFile::s_iFileCounter;

xiiOSFile::Event xiiOSFile::s_FileEvents;

xiiFileStats::xiiFileStats()  = default;
xiiFileStats::~xiiFileStats() = default;

void xiiFileStats::GetFullPath(xiiStringBuilder& ref_sPath) const
{
  ref_sPath.Set(m_sParentPath, "/", m_sName);
  ref_sPath.MakeCleanPath();
}

xiiOSFile::xiiOSFile()
{
  m_FileMode = xiiFileOpenMode::None;
  m_iFileID  = s_iFileCounter.Increment();
}

xiiOSFile::~xiiOSFile()
{
  Close();
}

xiiResult xiiOSFile::Open(xiiStringView sFile, xiiFileOpenMode::Enum openMode, xiiFileShareMode::Enum fileShareMode)
{
  m_iFileID = s_iFileCounter.Increment();

  XII_ASSERT_DEV(openMode >= xiiFileOpenMode::Read && openMode <= xiiFileOpenMode::Append, "Invalid Mode");
  XII_ASSERT_DEV(!IsOpen(), "The file has already been opened.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiTime t0 = xiiTime::Now();
#endif

  m_sFileName = sFile;
  m_sFileName.MakeCleanPath();
  m_sFileName.MakePathSeparatorsNative();

  xiiResult Res = XII_FAILURE;

  if (!m_sFileName.IsAbsolutePath())
    goto Completed;

  {
    xiiStringBuilder sFolder = m_sFileName.GetFileDirectory();

    if (openMode == xiiFileOpenMode::Write || openMode == xiiFileOpenMode::Append)
    {
      XII_SUCCEED_OR_RETURN(CreateDirectoryStructure(sFolder.GetData()));
    }
  }

  if (InternalOpen(m_sFileName.GetData(), openMode, fileShareMode) == XII_SUCCESS)
  {
    m_FileMode = openMode;
    Res        = XII_SUCCESS;
    goto Completed;
  }

  m_sFileName.Clear();
  m_FileMode = xiiFileOpenMode::None;
  goto Completed;

Completed:

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiTime tdiff = xiiTime::Now() - t0;
#else
  const xiiTime tdiff = xiiTime::MakeZero();
#endif

  EventData e;
  e.m_bSuccess  = Res == XII_SUCCESS;
  e.m_Duration  = tdiff;
  e.m_FileMode  = openMode;
  e.m_iFileID   = m_iFileID;
  e.m_sFile     = m_sFileName;
  e.m_EventType = EventType::FileOpen;

  s_FileEvents.Broadcast(e);

  return Res;
}

bool xiiOSFile::IsOpen() const
{
  return m_FileMode != xiiFileOpenMode::None;
}

void xiiOSFile::Close()
{
  if (!IsOpen())
    return;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiTime t0 = xiiTime::Now();
#endif

  InternalClose();

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiTime tdiff = xiiTime::Now() - t0;
#else
  const xiiTime tdiff = xiiTime::MakeZero();
#endif

  EventData e;
  e.m_bSuccess  = true;
  e.m_Duration  = tdiff;
  e.m_iFileID   = m_iFileID;
  e.m_sFile     = m_sFileName;
  e.m_EventType = EventType::FileClose;

  s_FileEvents.Broadcast(e);

  m_sFileName.Clear();
  m_FileMode = xiiFileOpenMode::None;
}

xiiResult xiiOSFile::Write(const void* pBuffer, xiiUInt64 uiBytes)
{
  if (uiBytes == 0)
    return XII_SUCCESS;

  XII_ASSERT_DEV((m_FileMode == xiiFileOpenMode::Write) || (m_FileMode == xiiFileOpenMode::Append), "The file is not opened for writing.");
  XII_ASSERT_DEV(pBuffer != nullptr, "pBuffer must not be nullptr.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiTime t0 = xiiTime::Now();
#endif

  const xiiResult Res = InternalWrite(pBuffer, uiBytes);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiTime tdiff = xiiTime::Now() - t0;
#else
  const xiiTime tdiff = xiiTime::MakeZero();
#endif

  EventData e;
  e.m_bSuccess        = Res == XII_SUCCESS;
  e.m_Duration        = tdiff;
  e.m_iFileID         = m_iFileID;
  e.m_sFile           = m_sFileName;
  e.m_EventType       = EventType::FileWrite;
  e.m_uiBytesAccessed = uiBytes;

  s_FileEvents.Broadcast(e);

  return Res;
}

xiiUInt64 xiiOSFile::Read(void* pBuffer, xiiUInt64 uiBytes)
{
  XII_ASSERT_DEV(m_FileMode == xiiFileOpenMode::Read, "The file is not opened for reading.");
  XII_ASSERT_DEV(pBuffer != nullptr, "pBuffer must not be nullptr.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiTime t0 = xiiTime::Now();
#endif

  const xiiUInt64 Res = InternalRead(pBuffer, uiBytes);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiTime tdiff = xiiTime::Now() - t0;
#else
  const xiiTime tdiff = xiiTime::MakeZero();
#endif

  EventData e;
  e.m_bSuccess        = (Res == uiBytes);
  e.m_Duration        = tdiff;
  e.m_iFileID         = m_iFileID;
  e.m_sFile           = m_sFileName;
  e.m_EventType       = EventType::FileRead;
  e.m_uiBytesAccessed = Res;

  s_FileEvents.Broadcast(e);

  return Res;
}

xiiUInt64 xiiOSFile::ReadAll(xiiDynamicArray<xiiUInt8>& out_fileContent)
{
  XII_ASSERT_DEV(m_FileMode == xiiFileOpenMode::Read, "The file is not opened for reading.");

  out_fileContent.Clear();
  out_fileContent.SetCountUninitialized((xiiUInt32)GetFileSize());

  if (!out_fileContent.IsEmpty())
  {
    Read(out_fileContent.GetData(), out_fileContent.GetCount());
  }

  return out_fileContent.GetCount();
}

xiiUInt64 xiiOSFile::GetFilePosition() const
{
  XII_ASSERT_DEV(IsOpen(), "The file must be open to tell the file pointer position.");

  return InternalGetFilePosition();
}

void xiiOSFile::SetFilePosition(xiiInt64 iDistance, xiiFileSeekMode::Enum pos) const
{
  XII_ASSERT_DEV(IsOpen(), "The file must be open to tell the file pointer position.");
  XII_ASSERT_DEV(m_FileMode != xiiFileOpenMode::Append, "SetFilePosition is not possible on files that were opened for appending.");

  return InternalSetFilePosition(iDistance, pos);
}

xiiUInt64 xiiOSFile::GetFileSize() const
{
  XII_ASSERT_DEV(IsOpen(), "The file must be open to tell the file size.");

  const xiiInt64 iCurPos = static_cast<xiiInt64>(GetFilePosition());

  // to circumvent the 'append does not support SetFilePosition' assert, we use the internal function directly
  InternalSetFilePosition(0, xiiFileSeekMode::FromEnd);

  const xiiUInt64 uiCurSize = static_cast<xiiInt64>(GetFilePosition());

  // to circumvent the 'append does not support SetFilePosition' assert, we use the internal function directly
  InternalSetFilePosition(iCurPos, xiiFileSeekMode::FromStart);

  return uiCurSize;
}

const xiiString xiiOSFile::MakePathAbsoluteWithCWD(xiiStringView sPath)
{
  xiiStringBuilder tmp = sPath;
  tmp.MakeCleanPath();

  if (tmp.IsRelativePath())
  {
    tmp.PrependFormat("{}/", GetCurrentWorkingDirectory());
    tmp.MakeCleanPath();
  }

  return tmp;
}

bool xiiOSFile::ExistsFile(xiiStringView sFile)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiTime t0 = xiiTime::Now();
#endif

  xiiStringBuilder s(sFile);
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  const bool bRes = InternalExistsFile(s);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiTime tdiff = xiiTime::Now() - t0;
#else
  const xiiTime tdiff = xiiTime::MakeZero();
#endif

  EventData e;
  e.m_bSuccess  = bRes;
  e.m_Duration  = tdiff;
  e.m_iFileID   = s_iFileCounter.Increment();
  e.m_sFile     = s;
  e.m_EventType = EventType::FileExists;

  s_FileEvents.Broadcast(e);

  return bRes;
}

bool xiiOSFile::ExistsDirectory(xiiStringView sDirectory)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiTime t0 = xiiTime::Now();
#endif

  xiiStringBuilder s(sDirectory);
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  XII_ASSERT_DEV(s.IsAbsolutePath(), "Path must be absolute");

  const bool bRes = InternalExistsDirectory(s);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiTime tdiff = xiiTime::Now() - t0;
#else
  const xiiTime tdiff = xiiTime::MakeZero();
#endif

  EventData e;
  e.m_bSuccess  = bRes;
  e.m_Duration  = tdiff;
  e.m_iFileID   = s_iFileCounter.Increment();
  e.m_sFile     = s;
  e.m_EventType = EventType::DirectoryExists;

  s_FileEvents.Broadcast(e);

  return bRes;
}

void xiiOSFile::FindFreeFilename(xiiStringBuilder& inout_sPath, xiiStringView sSuffix)
{
  XII_ASSERT_DEV(!inout_sPath.IsEmpty() && inout_sPath.IsAbsolutePath(), "Invalid input path.");

  if (!xiiOSFile::ExistsFile(inout_sPath))
    return;

  const xiiString sName = inout_sPath.GetFileName();

  xiiStringBuilder sNewName;

  for (xiiUInt32 i = 1; i < 100000; ++i)
  {
    sNewName.SetFormat("{}{}{}", sName, sSuffix, i);

    inout_sPath.ChangeFileName(sNewName);

    if (!xiiOSFile::ExistsFile(inout_sPath))
      return;
  }

  XII_REPORT_FAILURE("Something went wrong.");
}

xiiResult xiiOSFile::DeleteFile(xiiStringView sFile)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiTime t0 = xiiTime::Now();
#endif

  xiiStringBuilder s(sFile);
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  const xiiResult Res = InternalDeleteFile(s.GetData());

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiTime tdiff = xiiTime::Now() - t0;
#else
  const xiiTime tdiff = xiiTime::MakeZero();
#endif

  EventData e;
  e.m_bSuccess  = Res == XII_SUCCESS;
  e.m_Duration  = tdiff;
  e.m_iFileID   = s_iFileCounter.Increment();
  e.m_sFile     = sFile;
  e.m_EventType = EventType::FileDelete;

  s_FileEvents.Broadcast(e);

  return Res;
}

xiiStringView xiiOSFile::GetApplicationDirectory()
{
  if (s_sApplicationPath.IsEmpty())
  {
    // s_sApplicationPath is filled out and cached by GetApplicationPath(), so call that first, if necessary
    GetApplicationPath();

    XII_ASSERT_ALWAYS(!s_sApplicationPath.IsEmpty(), "Invalid application directory");
  }

  return s_sApplicationPath.GetFileDirectory();
}

xiiResult xiiOSFile::CreateDirectoryStructure(xiiStringView sDirectory)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiTime t0 = xiiTime::Now();
#endif

  xiiStringBuilder s(sDirectory);
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  XII_ASSERT_DEV(s.IsAbsolutePath(), "The path '{0}' is not absolute.", s);

  xiiStringBuilder sCurPath;

  auto it = s.GetIteratorFront();

  xiiResult Res = XII_SUCCESS;

  while (it.IsValid())
  {
    while ((it.GetCharacter() != '\0') && (!xiiPathUtils::IsPathSeparator(it.GetCharacter())))
    {
      sCurPath.Append(it.GetCharacter());
      ++it;
    }

    sCurPath.Append(it.GetCharacter());
    ++it;

    if (InternalCreateDirectory(sCurPath.GetData()) == XII_FAILURE)
    {
      Res = XII_FAILURE;
      break;
    }
  }

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiTime tdiff = xiiTime::Now() - t0;
#else
  const xiiTime tdiff = xiiTime::MakeZero();
#endif

  EventData e;
  e.m_bSuccess  = Res == XII_SUCCESS;
  e.m_Duration  = tdiff;
  e.m_iFileID   = s_iFileCounter.Increment();
  e.m_sFile     = sDirectory;
  e.m_EventType = EventType::MakeDir;

  s_FileEvents.Broadcast(e);

  return Res;
}

xiiResult xiiOSFile::MoveFileOrDirectory(xiiStringView sDirectoryFrom, xiiStringView sDirectoryTo)
{
  xiiStringBuilder sFrom(sDirectoryFrom);
  sFrom.MakeCleanPath();
  sFrom.MakePathSeparatorsNative();

  xiiStringBuilder sTo(sDirectoryTo);
  sTo.MakeCleanPath();
  sTo.MakePathSeparatorsNative();

  return InternalMoveFileOrDirectory(sFrom, sTo);
}

xiiResult xiiOSFile::CopyFile(xiiStringView sSource, xiiStringView sDestination)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiTime t0 = xiiTime::Now();
#endif

  xiiOSFile SrcFile, DstFile;

  xiiResult Res = XII_FAILURE;

  if (SrcFile.Open(sSource, xiiFileOpenMode::Read) == XII_FAILURE)
    goto Completed;

  DstFile.m_bRetryOnSharingViolation = false;
  if (DstFile.Open(sDestination, xiiFileOpenMode::Write) == XII_FAILURE)
    goto Completed;

  {
    const xiiUInt32 uiTempSize = 1024 * 1024 * 8; // 8 MB

    // can't allocate that much data on the stack
    xiiDynamicArray<xiiUInt8> TempBuffer;
    TempBuffer.SetCountUninitialized(uiTempSize);

    while (true)
    {
      const xiiUInt64 uiRead = SrcFile.Read(&TempBuffer[0], uiTempSize);

      if (uiRead == 0)
        break;

      if (DstFile.Write(&TempBuffer[0], uiRead) == XII_FAILURE)
        goto Completed;
    }
  }

  Res = XII_SUCCESS;

Completed:

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiTime tdiff = xiiTime::Now() - t0;
#else
  const xiiTime tdiff = xiiTime::MakeZero();
#endif

  EventData e;
  e.m_bSuccess  = Res == XII_SUCCESS;
  e.m_Duration  = tdiff;
  e.m_iFileID   = s_iFileCounter.Increment();
  e.m_sFile     = sSource;
  e.m_sFile2    = sDestination;
  e.m_EventType = EventType::FileCopy;

  s_FileEvents.Broadcast(e);

  return Res;
}

#if XII_ENABLED(XII_SUPPORTS_FILE_STATS)

xiiResult xiiOSFile::GetFileStats(xiiStringView sFileOrFolder, xiiFileStats& out_stats)
{
#  if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiTime t0 = xiiTime::Now();
#  endif

  xiiStringBuilder s = sFileOrFolder;
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  XII_ASSERT_DEV(s.IsAbsolutePath(), "The path '{0}' is not absolute.", s);

  const xiiResult Res = InternalGetFileStats(s.GetData(), out_stats);

#  if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiTime tdiff = xiiTime::Now() - t0;
#  else
  const xiiTime tdiff = xiiTime::MakeZero();
#  endif

  EventData e;
  e.m_bSuccess  = Res == XII_SUCCESS;
  e.m_Duration  = tdiff;
  e.m_iFileID   = s_iFileCounter.Increment();
  e.m_sFile     = sFileOrFolder;
  e.m_EventType = EventType::FileStat;

  s_FileEvents.Broadcast(e);

  return Res;
}

#  if XII_ENABLED(XII_SUPPORTS_CASE_INSENSITIVE_PATHS) && XII_ENABLED(XII_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
xiiResult xiiOSFile::GetFileCasing(xiiStringView sFileOrFolder, xiiStringBuilder& out_sCorrectSpelling)
{
  /// \todo Core: We should implement this also on xiiFileSystem, to be able to support stats through virtual filesystems.

#    if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiTime t0 = xiiTime::Now();
#    endif

  xiiStringBuilder s(sFileOrFolder);
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  XII_ASSERT_DEV(s.IsAbsolutePath(), "The path '{0}' is not absolute.", s);

  xiiStringBuilder sCurPath;

  auto it = s.GetIteratorFront();

  out_sCorrectSpelling.Clear();

  xiiResult Res = XII_SUCCESS;

  while (it.IsValid())
  {
    while ((it.GetCharacter() != '\0') && (!xiiPathUtils::IsPathSeparator(it.GetCharacter())))
    {
      sCurPath.Append(it.GetCharacter());
      ++it;
    }

    if (!sCurPath.IsEmpty())
    {
      xiiFileStats stats;
      if (GetFileStats(sCurPath.GetData(), stats) == XII_FAILURE)
      {
        Res = XII_FAILURE;
        break;
      }

      out_sCorrectSpelling.AppendPath(stats.m_sName);
    }
    sCurPath.Append(it.GetCharacter());
    ++it;
  }

#    if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiTime tdiff = xiiTime::Now() - t0;
#    else
  const xiiTime tdiff = xiiTime::MakeZero();
#    endif

  EventData e;
  e.m_bSuccess  = Res == XII_SUCCESS;
  e.m_Duration  = tdiff;
  e.m_iFileID   = s_iFileCounter.Increment();
  e.m_sFile     = sFileOrFolder;
  e.m_EventType = EventType::FileCasing;

  s_FileEvents.Broadcast(e);

  return Res;
}

#  endif // XII_SUPPORTS_CASE_INSENSITIVE_PATHS && XII_SUPPORTS_UNRESTRICTED_FILE_ACCESS

#endif // XII_SUPPORTS_FILE_STATS

#if XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS) && XII_ENABLED(XII_SUPPORTS_FILE_STATS)

void xiiOSFile::GatherAllItemsInFolder(xiiDynamicArray<xiiFileStats>& out_itemList, xiiStringView sFolder, xiiBitflags<xiiFileSystemIteratorFlags> flags /*= xiiFileSystemIteratorFlags::All*/)
{
  out_itemList.Clear();

  xiiFileSystemIterator iterator;
  iterator.StartSearch(sFolder, flags);

  if (!iterator.IsValid())
    return;

  out_itemList.Reserve(128);

  while (iterator.IsValid())
  {
    out_itemList.PushBack(iterator.GetStats());

    iterator.Next();
  }
}

xiiResult xiiOSFile::CopyFolder(xiiStringView sSourceFolder, xiiStringView sDestinationFolder, xiiDynamicArray<xiiString>* out_pFilesCopied /*= nullptr*/)
{
  xiiDynamicArray<xiiFileStats> items;
  GatherAllItemsInFolder(items, sSourceFolder);

  xiiStringBuilder srcPath;
  xiiStringBuilder dstPath;
  xiiStringBuilder relPath;

  for (const auto& item : items)
  {
    srcPath = item.m_sParentPath;
    srcPath.AppendPath(item.m_sName);

    relPath = srcPath;

    if (relPath.MakeRelativeTo(sSourceFolder).Failed())
      return XII_FAILURE; // unexpected to ever fail, but don't want to assert on it

    dstPath = sDestinationFolder;
    dstPath.AppendPath(relPath);

    if (item.m_bIsDirectory)
    {
      if (xiiOSFile::CreateDirectoryStructure(dstPath).Failed())
        return XII_FAILURE;
    }
    else
    {
      if (xiiOSFile::CopyFile(srcPath, dstPath).Failed())
        return XII_FAILURE;

      if (out_pFilesCopied)
      {
        out_pFilesCopied->PushBack(dstPath);
      }
    }

    // TODO: make sure to remove read-only flags of copied files ?
  }

  return XII_SUCCESS;
}

xiiResult xiiOSFile::DeleteFolder(xiiStringView sFolder)
{
  xiiDynamicArray<xiiFileStats> items;
  GatherAllItemsInFolder(items, sFolder);

  xiiStringBuilder fullPath;

  for (const auto& item : items)
  {
    if (item.m_bIsDirectory)
      continue;

    fullPath = item.m_sParentPath;
    fullPath.AppendPath(item.m_sName);

    if (xiiOSFile::DeleteFile(fullPath).Failed())
      return XII_FAILURE;
  }

  for (xiiUInt32 i = items.GetCount(); i > 0; --i)
  {
    const auto& item = items[i - 1];

    if (!item.m_bIsDirectory)
      continue;

    fullPath = item.m_sParentPath;
    fullPath.AppendPath(item.m_sName);

    if (xiiOSFile::InternalDeleteDirectory(fullPath).Failed())
      return XII_FAILURE;
  }

  if (xiiOSFile::InternalDeleteDirectory(sFolder).Failed())
    return XII_FAILURE;

  return XII_SUCCESS;
}

#endif // XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS) && XII_ENABLED(XII_SUPPORTS_FILE_STATS)

#if XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS)

void xiiFileSystemIterator::StartMultiFolderSearch(xiiArrayPtr<xiiString> startFolders, xiiStringView sSearchTerm, xiiBitflags<xiiFileSystemIteratorFlags> flags /*= xiiFileSystemIteratorFlags::Default*/)
{
  if (startFolders.IsEmpty())
    return;

  m_sMultiSearchTerm     = sSearchTerm;
  m_Flags                = flags;
  m_uiCurrentStartFolder = 0;
  m_StartFolders         = startFolders;

  xiiStringBuilder search = startFolders[m_uiCurrentStartFolder];
  search.AppendPath(sSearchTerm);

  StartSearch(search, m_Flags);

  if (!IsValid())
  {
    Next();
  }
}

void xiiFileSystemIterator::Next()
{
  while (true)
  {
    const xiiInt32 res = InternalNext();

    if (res == 1) // success
    {
      return;
    }
    else if (res == 0) // failure
    {
      ++m_uiCurrentStartFolder;

      if (m_uiCurrentStartFolder < m_StartFolders.GetCount())
      {
        xiiStringBuilder search = m_StartFolders[m_uiCurrentStartFolder];
        search.AppendPath(m_sMultiSearchTerm);

        if (search.IsAbsolutePath())
        {
          StartSearch(search, m_Flags);
        }
      }
      else
      {
        return;
      }

      if (IsValid())
      {
        return;
      }
    }
    else
    {
      // call InternalNext() again
    }
  }
}

void xiiFileSystemIterator::SkipFolder()
{
  XII_ASSERT_DEBUG(m_Flags.IsSet(xiiFileSystemIteratorFlags::Recursive), "SkipFolder has no meaning when the iterator is not set to be recursive.");
  XII_ASSERT_DEBUG(m_CurFile.m_bIsDirectory, "SkipFolder can only be called when the current object is a folder.");

  m_Flags.Remove(xiiFileSystemIteratorFlags::Recursive);

  Next();

  m_Flags.Add(xiiFileSystemIteratorFlags::Recursive);
}

#endif

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Platform/Implementation/Windows/OSFile_win.h>
#elif XII_ENABLED(XII_USE_POSIX_FILE_API)
#  include <Foundation/Platform/Implementation/Posix/OSFile_posix.h>
#else
#  error "Unknown Platform."
#endif

XII_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_OSFile);
