#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OSFile.h>

xiiString64        xiiOSFile::s_sApplicationPath;
xiiString64        xiiOSFile::s_sUserDataPath;
xiiString64        xiiOSFile::s_sTempDataPath;
xiiAtomicInteger32 xiiOSFile::s_iFileCounter;
xiiOSFile::Event   xiiOSFile::s_FileEvents;

xiiFileStats::xiiFileStats() = default;

void xiiFileStats::GetFullPath(xiiStringBuilder& path) const
{
  path.Set(m_sParentPath, "/", m_sName);
  path.MakeCleanPath();
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

xiiResult xiiOSFile::Open(const char* szFile, xiiFileOpenMode::Enum OpenMode, xiiFileShareMode::Enum FileShareMode)
{
  m_iFileID = s_iFileCounter.Increment();

  XII_ASSERT_DEV(OpenMode >= xiiFileOpenMode::Read && OpenMode <= xiiFileOpenMode::Append, "Invalid Mode");
  XII_ASSERT_DEV(!IsOpen(), "The file has already been opened.");

  const xiiTime t0 = xiiTime::Now();

  m_sFileName = szFile;
  m_sFileName.MakeCleanPath();
  m_sFileName.MakePathSeparatorsNative();

  xiiResult Res = XII_FAILURE;

  if (!m_sFileName.IsAbsolutePath())
    goto done;

  {
    xiiStringBuilder sFolder = m_sFileName.GetFileDirectory();

    if (OpenMode == xiiFileOpenMode::Write || OpenMode == xiiFileOpenMode::Append)
    {
      XII_SUCCEED_OR_RETURN(CreateDirectoryStructure(sFolder.GetData()));
    }
  }

  if (InternalOpen(m_sFileName.GetData(), OpenMode, FileShareMode) == XII_SUCCESS)
  {
    m_FileMode = OpenMode;
    Res        = XII_SUCCESS;
    goto done;
  }

  m_sFileName.Clear();
  m_FileMode = xiiFileOpenMode::None;
  goto done;

done:
  const xiiTime t1    = xiiTime::Now();
  const xiiTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess  = Res == XII_SUCCESS;
  e.m_Duration  = tdiff;
  e.m_FileMode  = OpenMode;
  e.m_iFileID   = m_iFileID;
  e.m_szFile    = m_sFileName.GetData();
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

  const xiiTime t0 = xiiTime::Now();

  InternalClose();

  const xiiTime t1    = xiiTime::Now();
  const xiiTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess  = true;
  e.m_Duration  = tdiff;
  e.m_iFileID   = m_iFileID;
  e.m_szFile    = m_sFileName.GetData();
  e.m_EventType = EventType::FileClose;

  s_FileEvents.Broadcast(e);

  m_sFileName.Clear();
  m_FileMode = xiiFileOpenMode::None;
}

xiiResult xiiOSFile::Write(const void* pBuffer, xiiUInt64 uiBytes)
{
  XII_ASSERT_DEV((m_FileMode == xiiFileOpenMode::Write) || (m_FileMode == xiiFileOpenMode::Append), "The file is not opened for writing.");
  XII_ASSERT_DEV(pBuffer != nullptr, "pBuffer must not be nullptr.");

  const xiiTime t0 = xiiTime::Now();

  const xiiResult Res = InternalWrite(pBuffer, uiBytes);

  const xiiTime t1    = xiiTime::Now();
  const xiiTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess        = Res == XII_SUCCESS;
  e.m_Duration        = tdiff;
  e.m_iFileID         = m_iFileID;
  e.m_szFile          = m_sFileName.GetData();
  e.m_EventType       = EventType::FileWrite;
  e.m_uiBytesAccessed = uiBytes;

  s_FileEvents.Broadcast(e);

  return Res;
}

xiiUInt64 xiiOSFile::Read(void* pBuffer, xiiUInt64 uiBytes)
{
  XII_ASSERT_DEV(m_FileMode == xiiFileOpenMode::Read, "The file is not opened for reading.");
  XII_ASSERT_DEV(pBuffer != nullptr, "pBuffer must not be nullptr.");

  const xiiTime t0 = xiiTime::Now();

  const xiiUInt64 Res = InternalRead(pBuffer, uiBytes);

  const xiiTime t1    = xiiTime::Now();
  const xiiTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess        = (Res == uiBytes);
  e.m_Duration        = tdiff;
  e.m_iFileID         = m_iFileID;
  e.m_szFile          = m_sFileName.GetData();
  e.m_EventType       = EventType::FileRead;
  e.m_uiBytesAccessed = Res;

  s_FileEvents.Broadcast(e);

  return Res;
}

xiiUInt64 xiiOSFile::ReadAll(xiiDynamicArray<xiiUInt8>& out_FileContent)
{
  XII_ASSERT_DEV(m_FileMode == xiiFileOpenMode::Read, "The file is not opened for reading.");

  out_FileContent.Clear();
  out_FileContent.SetCountUninitialized((xiiUInt32)GetFileSize());

  if (!out_FileContent.IsEmpty())
  {
    Read(out_FileContent.GetData(), out_FileContent.GetCount());
  }

  return out_FileContent.GetCount();
}

xiiUInt64 xiiOSFile::GetFilePosition() const
{
  XII_ASSERT_DEV(IsOpen(), "The file must be open to tell the file pointer position.");

  return InternalGetFilePosition();
}

void xiiOSFile::SetFilePosition(xiiInt64 iDistance, xiiFileSeekMode::Enum Pos) const
{
  XII_ASSERT_DEV(IsOpen(), "The file must be open to tell the file pointer position.");
  XII_ASSERT_DEV(m_FileMode != xiiFileOpenMode::Append, "SetFilePosition is not possible on files that were opened for appending.");

  return InternalSetFilePosition(iDistance, Pos);
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

const xiiString xiiOSFile::MakePathAbsoluteWithCWD(const char* szPath)
{
  xiiStringBuilder tmp = szPath;
  tmp.MakeCleanPath();

  if (tmp.IsRelativePath())
  {
    tmp.PrependFormat("{}/", GetCurrentWorkingDirectory());
    tmp.MakeCleanPath();
  }

  return tmp;
}

bool xiiOSFile::ExistsFile(const char* szFile)
{
  const xiiTime t0 = xiiTime::Now();

  xiiStringBuilder s(szFile);
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  const bool bRes = InternalExistsFile(s);

  const xiiTime t1    = xiiTime::Now();
  const xiiTime tdiff = t1 - t0;


  EventData e;
  e.m_bSuccess  = bRes;
  e.m_Duration  = tdiff;
  e.m_iFileID   = s_iFileCounter.Increment();
  e.m_szFile    = s;
  e.m_EventType = EventType::FileExists;

  s_FileEvents.Broadcast(e);

  return bRes;
}

bool xiiOSFile::ExistsDirectory(const char* szDirectory)
{
  const xiiTime t0 = xiiTime::Now();

  xiiStringBuilder s(szDirectory);
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  XII_ASSERT_DEV(s.IsAbsolutePath(), "Path must be absolute");

  const bool bRes = InternalExistsDirectory(s);

  const xiiTime t1    = xiiTime::Now();
  const xiiTime tdiff = t1 - t0;


  EventData e;
  e.m_bSuccess  = bRes;
  e.m_Duration  = tdiff;
  e.m_iFileID   = s_iFileCounter.Increment();
  e.m_szFile    = s;
  e.m_EventType = EventType::DirectoryExists;

  s_FileEvents.Broadcast(e);

  return bRes;
}

xiiResult xiiOSFile::DeleteFile(const char* szFile)
{
  const xiiTime t0 = xiiTime::Now();

  xiiStringBuilder s(szFile);
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  const xiiResult Res = InternalDeleteFile(s.GetData());

  const xiiTime t1    = xiiTime::Now();
  const xiiTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess  = Res == XII_SUCCESS;
  e.m_Duration  = tdiff;
  e.m_iFileID   = s_iFileCounter.Increment();
  e.m_szFile    = szFile;
  e.m_EventType = EventType::FileDelete;

  s_FileEvents.Broadcast(e);

  return Res;
}

xiiResult xiiOSFile::CreateDirectoryStructure(const char* szDirectory)
{
  const xiiTime t0 = xiiTime::Now();

  xiiStringBuilder s(szDirectory);
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

  const xiiTime t1    = xiiTime::Now();
  const xiiTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess  = Res == XII_SUCCESS;
  e.m_Duration  = tdiff;
  e.m_iFileID   = s_iFileCounter.Increment();
  e.m_szFile    = szDirectory;
  e.m_EventType = EventType::MakeDir;

  s_FileEvents.Broadcast(e);

  return Res;
}

xiiResult xiiOSFile::MoveFileOrDirectory(const char* szDirectoryFrom, const char* szDirectoryTo)
{
  xiiStringBuilder sFrom(szDirectoryFrom);
  sFrom.MakeCleanPath();
  sFrom.MakePathSeparatorsNative();

  xiiStringBuilder sTo(szDirectoryTo);
  sTo.MakeCleanPath();
  sTo.MakePathSeparatorsNative();

  return InternalMoveFileOrDirectory(sFrom, sTo);
}

xiiResult xiiOSFile::CopyFile(const char* szSource, const char* szDestination)
{
  const xiiTime t0 = xiiTime::Now();

  xiiOSFile SrcFile, DstFile;

  xiiResult Res = XII_FAILURE;

  if (SrcFile.Open(szSource, xiiFileOpenMode::Read) == XII_FAILURE)
    goto done;

  DstFile.m_bRetryOnSharingViolation = false;
  if (DstFile.Open(szDestination, xiiFileOpenMode::Write) == XII_FAILURE)
    goto done;

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
        goto done;
    }
  }

  Res = XII_SUCCESS;

done:

  const xiiTime t1    = xiiTime::Now();
  const xiiTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess  = Res == XII_SUCCESS;
  e.m_Duration  = tdiff;
  e.m_iFileID   = s_iFileCounter.Increment();
  e.m_szFile    = szSource;
  e.m_szFile2   = szDestination;
  e.m_EventType = EventType::FileCopy;

  s_FileEvents.Broadcast(e);

  return Res;
}

#if XII_ENABLED(XII_SUPPORTS_FILE_STATS)

xiiResult xiiOSFile::GetFileStats(const char* szFileOrFolder, xiiFileStats& out_Stats)
{
  const xiiTime t0 = xiiTime::Now();

  xiiStringBuilder s = szFileOrFolder;
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  XII_ASSERT_DEV(s.IsAbsolutePath(), "The path '{0}' is not absolute.", s);

  const xiiResult Res = InternalGetFileStats(s.GetData(), out_Stats);

  const xiiTime t1    = xiiTime::Now();
  const xiiTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess  = Res == XII_SUCCESS;
  e.m_Duration  = tdiff;
  e.m_iFileID   = s_iFileCounter.Increment();
  e.m_szFile    = szFileOrFolder;
  e.m_EventType = EventType::FileStat;

  s_FileEvents.Broadcast(e);

  return Res;
}

#  if XII_ENABLED(XII_SUPPORTS_CASE_INSENSITIVE_PATHS) && XII_ENABLED(XII_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
xiiResult xiiOSFile::GetFileCasing(const char* szFileOrFolder, xiiStringBuilder& out_sCorrectSpelling)
{
  /// \todo We should implement this also on xiiFileSystem, to be able to support stats through virtual filesystems

  const xiiTime t0 = xiiTime::Now();

  xiiStringBuilder s(szFileOrFolder);
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

  const xiiTime t1    = xiiTime::Now();
  const xiiTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess  = Res == XII_SUCCESS;
  e.m_Duration  = tdiff;
  e.m_iFileID   = s_iFileCounter.Increment();
  e.m_szFile    = szFileOrFolder;
  e.m_EventType = EventType::FileCasing;

  s_FileEvents.Broadcast(e);

  return Res;
}

#  endif // XII_SUPPORTS_CASE_INSENSITIVE_PATHS && XII_SUPPORTS_UNRESTRICTED_FILE_ACCESS

#endif // XII_SUPPORTS_FILE_STATS

#if XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS) && XII_ENABLED(XII_SUPPORTS_FILE_STATS)

void xiiOSFile::GatherAllItemsInFolder(xiiDynamicArray<xiiFileStats>& out_ItemList, const char* szFolder, xiiBitflags<xiiFileSystemIteratorFlags> flags /*= xiiFileSystemIteratorFlags::All*/)
{
  out_ItemList.Clear();

  xiiFileSystemIterator iterator;
  iterator.StartSearch(szFolder, flags);

  if (!iterator.IsValid())
    return;

  out_ItemList.Reserve(128);

  while (iterator.IsValid())
  {
    out_ItemList.PushBack(iterator.GetStats());

    iterator.Next();
  }
}

xiiResult xiiOSFile::CopyFolder(const char* szSourceFolder, const char* szDestinationFolder, xiiDynamicArray<xiiString>* out_FilesCopied /*= nullptr*/)
{
  xiiDynamicArray<xiiFileStats> items;
  GatherAllItemsInFolder(items, szSourceFolder);

  xiiStringBuilder srcPath;
  xiiStringBuilder dstPath;
  xiiStringBuilder relPath;

  for (const auto& item : items)
  {
    srcPath = item.m_sParentPath;
    srcPath.AppendPath(item.m_sName);

    relPath = srcPath;

    if (relPath.MakeRelativeTo(szSourceFolder).Failed())
      return XII_FAILURE; // unexpected to ever fail, but don't want to assert on it

    dstPath = szDestinationFolder;
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

      if (out_FilesCopied)
      {
        out_FilesCopied->PushBack(dstPath);
      }
    }

    // TODO: make sure to remove read-only flags of copied files ?
  }

  return XII_SUCCESS;
}

xiiResult xiiOSFile::DeleteFolder(const char* szFolder)
{
  xiiDynamicArray<xiiFileStats> items;
  GatherAllItemsInFolder(items, szFolder);

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

  if (xiiOSFile::InternalDeleteDirectory(szFolder).Failed())
    return XII_FAILURE;

  return XII_SUCCESS;
}

#endif // XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS) && XII_ENABLED(XII_SUPPORTS_FILE_STATS)

#if XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS)

void xiiFileSystemIterator::StartMultiFolderSearch(xiiArrayPtr<xiiString> startFolders, const char* szSearchTerm, xiiBitflags<xiiFileSystemIteratorFlags> flags /*= xiiFileSystemIteratorFlags::Default*/)
{
  if (startFolders.IsEmpty())
    return;

  m_sMultiSearchTerm     = szSearchTerm;
  m_Flags                = flags;
  m_uiCurrentStartFolder = 0;
  m_StartFolders         = startFolders;

  xiiStringBuilder search = startFolders[m_uiCurrentStartFolder];
  search.AppendPath(szSearchTerm);

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

        StartSearch(search, m_Flags);
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
#  include <Foundation/IO/Implementation/Win/OSFile_win.h>

// For UWP we're currently using a mix of WinRT functions and posix.
#  if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
#    include <Foundation/IO/Implementation/Posix/OSFile_posix.h>
#  endif
#elif XII_ENABLED(XII_USE_POSIX_FILE_API)
#  include <Foundation/IO/Implementation/Posix/OSFile_posix.h>
#else
#  error "Unknown Platform."
#endif

XII_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_OSFile);
