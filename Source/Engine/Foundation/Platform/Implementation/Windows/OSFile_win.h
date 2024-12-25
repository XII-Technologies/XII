#pragma once

#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Logging/Log.h>
#include <Foundation/Platform/Implementation/Windows/DosDevicePath_win.h>
#include <Foundation/Strings/StringConversion.h>
#include <Foundation/Threading/ThreadUtils.h>

// Defined in Timestamp_win.h
xiiInt64 FileTimeToEpoch(FILETIME fileTime);

static xiiUInt64 HighLowToUInt64(xiiUInt32 uiHigh32, xiiUInt32 uiLow32)
{
  xiiUInt64 uiHigh64 = uiHigh32;
  xiiUInt64 uiLow64  = uiLow32;

  return (uiHigh64 << 32) | uiLow64;
}

#if XII_DISABLED(XII_USE_POSIX_FILE_API)

#  include <Shlobj.h>

xiiResult xiiOSFile::InternalOpen(xiiStringView sFile, xiiFileOpenMode::Enum OpenMode, xiiFileShareMode::Enum FileShareMode)
{
  const xiiTime sleepTime = xiiTime::MakeFromMilliseconds(20);
  xiiInt32      iRetries  = 20;

  if (FileShareMode == xiiFileShareMode::Default)
  {
    // when 'default' share mode is requested, use 'share reads' when opening a file for reading
    // and use 'exclusive' when opening a file for writing

    if (OpenMode == xiiFileOpenMode::Read)
    {
      FileShareMode = xiiFileShareMode::SharedReads;
    }
    else
    {
      FileShareMode = xiiFileShareMode::Exclusive;
    }
  }

  DWORD dwSharedMode = 0; // exclusive access
  if (FileShareMode == xiiFileShareMode::SharedReads)
  {
    dwSharedMode = FILE_SHARE_READ;
  }

  while (iRetries > 0)
  {
    SetLastError(ERROR_SUCCESS);
    DWORD error = 0;

    switch (OpenMode)
    {
      case xiiFileOpenMode::Read:
        m_FileData.m_pFileHandle = CreateFileW(xiiDosDevicePath(sFile), GENERIC_READ, dwSharedMode, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        break;

      case xiiFileOpenMode::Write:
        m_FileData.m_pFileHandle = CreateFileW(xiiDosDevicePath(sFile), GENERIC_WRITE, dwSharedMode, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        break;

      case xiiFileOpenMode::Append:
        m_FileData.m_pFileHandle = CreateFileW(xiiDosDevicePath(sFile), FILE_APPEND_DATA, dwSharedMode, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

        // in append mode we need to set the file pointer to the end explicitly, otherwise GetFilePosition might return 0 the first time
        if ((m_FileData.m_pFileHandle != nullptr) && (m_FileData.m_pFileHandle != INVALID_HANDLE_VALUE))
          InternalSetFilePosition(0, xiiFileSeekMode::FromEnd);

        break;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    const xiiResult res = ((m_FileData.m_pFileHandle != nullptr) && (m_FileData.m_pFileHandle != INVALID_HANDLE_VALUE)) ? XII_SUCCESS : XII_FAILURE;

    if (res.Failed())
    {
      if (xiiOSFile::ExistsDirectory(sFile))
      {
        // trying to 'open' a directory fails with little useful error codes such as 'access denied'
        return XII_FAILURE;
      }

      error = GetLastError();

      // file does not exist
      if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
        return res;
      // badly formed path, happens when two absolute paths are concatenated
      if (error == ERROR_INVALID_NAME)
        return res;

      if (error == ERROR_SHARING_VIOLATION
          // these two situations happen when the xiiInspector is connected
          // for some reason, the networking blocks file reading (when run on the same machine)
          // retrying fixes the problem, but can introduce very long stalls
          || error == WSAEWOULDBLOCK || error == ERROR_SUCCESS)
      {
        if (m_bRetryOnSharingViolation)
        {
          --iRetries;
          xiiThreadUtils::Sleep(sleepTime);
          continue; // try again
        }
        else
        {
          return res;
        }
      }

      // anything else, print an error (for now)
      xiiLog::Error("CreateFile failed with error {0}", xiiArgErrorCode(error));
    }

    return res;
  }

  return XII_FAILURE;
}

void xiiOSFile::InternalClose()
{
  CloseHandle(m_FileData.m_pFileHandle);
  m_FileData.m_pFileHandle = INVALID_HANDLE_VALUE;
}

xiiResult xiiOSFile::InternalWrite(const void* pBuffer, xiiUInt64 uiBytes)
{
  const xiiUInt32 uiBatchBytes = 1024 * 1024 * 1024; // 1 GB

  // first write out all the data in 1GB batches
  while (uiBytes > uiBatchBytes)
  {
    DWORD uiBytesWritten = 0;
    if ((!WriteFile(m_FileData.m_pFileHandle, pBuffer, uiBatchBytes, &uiBytesWritten, nullptr)) || (uiBytesWritten != uiBatchBytes))
      return XII_FAILURE;

    uiBytes -= uiBatchBytes;
    pBuffer = xiiMemoryUtils::AddByteOffset(pBuffer, uiBatchBytes);
  }

  if (uiBytes > 0)
  {
    const xiiUInt32 uiBytes32 = static_cast<xiiUInt32>(uiBytes);

    DWORD uiBytesWritten = 0;
    if ((!WriteFile(m_FileData.m_pFileHandle, pBuffer, uiBytes32, &uiBytesWritten, nullptr)) || (uiBytesWritten != uiBytes32))
      return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiUInt64 xiiOSFile::InternalRead(void* pBuffer, xiiUInt64 uiBytes)
{
  xiiUInt64 uiBytesRead = 0;

  const xiiUInt32 uiBatchBytes = 1024 * 1024 * 1024; // 1 GB

  // first write out all the data in 1GB batches
  while (uiBytes > uiBatchBytes)
  {
    DWORD uiBytesReadThisTime = 0;
    if (!ReadFile(m_FileData.m_pFileHandle, pBuffer, uiBatchBytes, &uiBytesReadThisTime, nullptr))
      return uiBytesRead + uiBytesReadThisTime;

    uiBytesRead += uiBytesReadThisTime;

    if (uiBytesReadThisTime != uiBatchBytes)
      return uiBytesRead;

    uiBytes -= uiBatchBytes;
    pBuffer = xiiMemoryUtils::AddByteOffset(pBuffer, uiBatchBytes);
  }

  if (uiBytes > 0)
  {
    const xiiUInt32 uiBytes32 = static_cast<xiiUInt32>(uiBytes);

    DWORD uiBytesReadThisTime = 0;
    if (!ReadFile(m_FileData.m_pFileHandle, pBuffer, uiBytes32, &uiBytesReadThisTime, nullptr))
      return uiBytesRead + uiBytesReadThisTime;

    uiBytesRead += uiBytesReadThisTime;
  }

  return uiBytesRead;
}

xiiUInt64 xiiOSFile::InternalGetFilePosition() const
{
  long int  uiHigh32 = 0;
  xiiUInt32 uiLow32  = SetFilePointer(m_FileData.m_pFileHandle, 0, &uiHigh32, FILE_CURRENT);

  return HighLowToUInt64(uiHigh32, uiLow32);
}

void xiiOSFile::InternalSetFilePosition(xiiInt64 iDistance, xiiFileSeekMode::Enum Pos) const
{
  LARGE_INTEGER pos;
  LARGE_INTEGER newpos;
  pos.QuadPart = static_cast<LONGLONG>(iDistance);

  switch (Pos)
  {
    case xiiFileSeekMode::FromStart:
      XII_VERIFY(SetFilePointerEx(m_FileData.m_pFileHandle, pos, &newpos, FILE_BEGIN), "Seek Failed.");
      break;
    case xiiFileSeekMode::FromEnd:
      XII_VERIFY(SetFilePointerEx(m_FileData.m_pFileHandle, pos, &newpos, FILE_END), "Seek Failed.");
      break;
    case xiiFileSeekMode::FromCurrent:
      XII_VERIFY(SetFilePointerEx(m_FileData.m_pFileHandle, pos, &newpos, FILE_CURRENT), "Seek Failed.");
      break;
  }
}

bool xiiOSFile::InternalExistsFile(xiiStringView sFile)
{
  const DWORD dwAttrib = GetFileAttributesW(xiiDosDevicePath(sFile).GetData());

  return ((dwAttrib != INVALID_FILE_ATTRIBUTES) && ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == 0));
}

bool xiiOSFile::InternalExistsDirectory(xiiStringView sDirectory)
{
  const DWORD dwAttrib = GetFileAttributesW(xiiDosDevicePath(sDirectory));

  return ((dwAttrib != INVALID_FILE_ATTRIBUTES) && ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0));
}

xiiResult xiiOSFile::InternalDeleteFile(xiiStringView sFile)
{
  if (DeleteFileW(xiiDosDevicePath(sFile)) == FALSE)
  {
    DWORD error = GetLastError();
    if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
      return XII_SUCCESS;

    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiOSFile::InternalDeleteDirectory(xiiStringView sDirectory)
{
  if (RemoveDirectoryW(xiiDosDevicePath(sDirectory)) == FALSE)
  {
    DWORD error = GetLastError();
    if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
      return XII_SUCCESS;

    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiOSFile::InternalCreateDirectory(xiiStringView sDirectory)
{
  // handle drive letters as always successful
  if (xiiStringUtils::GetCharacterCount(sDirectory.GetStartPointer(), sDirectory.GetEndPointer()) <= 3) // 'C:\'
    return XII_SUCCESS;

  if (CreateDirectoryW(xiiDosDevicePath(sDirectory), nullptr) == FALSE)
  {
    const DWORD uiError = GetLastError();
    if (uiError == ERROR_ALREADY_EXISTS)
      return XII_SUCCESS;

    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiOSFile::InternalMoveFileOrDirectory(xiiStringView sDirectoryFrom, xiiStringView sDirectoryTo)
{
  if (MoveFileW(xiiDosDevicePath(sDirectoryFrom), xiiDosDevicePath(sDirectoryTo)) == 0)
  {
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

#endif // not XII_USE_POSIX_FILE_API

xiiResult xiiOSFile::InternalGetFileStats(xiiStringView sFileOrFolder, xiiFileStats& out_Stats)
{
  xiiStringBuilder s = sFileOrFolder;

  // FindFirstFile does not like paths that end with a separator, so remove them all
  s.Trim(nullptr, "/\\");

  // handle the case that this query is done on the 'device part' of a path
  if (s.GetCharacterCount() <= 2) // 'C:', 'D:', 'E' etc.
  {
    s.ToUpper();

    out_Stats.m_uiFileSize   = 0;
    out_Stats.m_bIsDirectory = true;
    out_Stats.m_sParentPath.Clear();
    out_Stats.m_sName                = s;
    out_Stats.m_LastModificationTime = xiiTimestamp::MakeInvalid();
    return XII_SUCCESS;
  }

  WIN32_FIND_DATAW data;
  HANDLE           hSearch = FindFirstFileW(xiiDosDevicePath(s), &data);

  if ((hSearch == nullptr) || (hSearch == INVALID_HANDLE_VALUE))
    return XII_FAILURE;

  out_Stats.m_uiFileSize   = HighLowToUInt64(data.nFileSizeHigh, data.nFileSizeLow);
  out_Stats.m_bIsDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
  out_Stats.m_sParentPath  = sFileOrFolder;
  out_Stats.m_sParentPath.PathParentDirectory();
  out_Stats.m_sName                = data.cFileName;
  out_Stats.m_LastModificationTime = xiiTimestamp::MakeFromInt(FileTimeToEpoch(data.ftLastWriteTime), xiiSIUnitOfTime::Microsecond);

  FindClose(hSearch);
  return XII_SUCCESS;
}

#if XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS)

xiiFileSystemIterator::xiiFileSystemIterator() = default;

xiiFileSystemIterator::~xiiFileSystemIterator()
{
  while (!m_Data.m_Handles.IsEmpty())
  {
    FindClose(m_Data.m_Handles.PeekBack());
    m_Data.m_Handles.PopBack();
  }
}

bool xiiFileSystemIterator::IsValid() const
{
  return !m_Data.m_Handles.IsEmpty();
}

void xiiFileSystemIterator::StartSearch(xiiStringView sSearchStart, xiiBitflags<xiiFileSystemIteratorFlags> flags /*= xiiFileSystemIteratorFlags::All*/)
{
  XII_ASSERT_DEV(m_Data.m_Handles.IsEmpty(), "Cannot start another search.");

  m_sSearchTerm = sSearchStart;

  xiiStringBuilder sSearch = sSearchStart;
  sSearch.MakeCleanPath();

  // same as just passing in the folder path, so remove this
  if (sSearch.EndsWith("/*"))
    sSearch.Shrink(0, 2);

  // The Windows documentation disallows trailing (back)slashes.
  sSearch.Trim(nullptr, "/");

  // Since the use of wildcard-ed file names will disable recursion, we ensure both are not used simultaneously.
  const bool bHasWildcard = sSearch.FindLastSubString("*") || sSearch.FindLastSubString("?");
  XII_ASSERT_DEV(flags.IsSet(xiiFileSystemIteratorFlags::Recursive) == false || bHasWildcard == false, "Recursive file iteration does not support wildcards. Either don't use recursion, or filter the filenames manually.");

  m_sCurPath = sSearch.GetFileDirectory();

  XII_ASSERT_DEV(sSearch.IsAbsolutePath(), "The path '{0}' is not absolute.", m_sCurPath);

  m_Flags = flags;

  WIN32_FIND_DATAW data;
  HANDLE           hSearch = FindFirstFileW(xiiDosDevicePath(sSearch), &data);

  if ((hSearch == nullptr) || (hSearch == INVALID_HANDLE_VALUE))
    return;

  m_CurFile.m_uiFileSize           = HighLowToUInt64(data.nFileSizeHigh, data.nFileSizeLow);
  m_CurFile.m_bIsDirectory         = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
  m_CurFile.m_sParentPath          = m_sCurPath;
  m_CurFile.m_sName                = data.cFileName;
  m_CurFile.m_LastModificationTime = xiiTimestamp::MakeFromInt(FileTimeToEpoch(data.ftLastWriteTime), xiiSIUnitOfTime::Microsecond);

  m_Data.m_Handles.PushBack(hSearch);

  if (xiiOSFile::ExistsDirectory(sSearch))
  {
    // when calling FindFirstFileW with a path to a folder (e.g. "C:/test") it will report "test" as the very first item
    // which is typically NOT what one wants, instead you want items INSIDE that folder to be reported
    // this is especially annoying when 'Recursion' is disabled, as "C:/test" would result in "C:/test" being reported
    // but no items inside it
    // therefore, when the start search points to a directory, we enable recursion for one call to 'Next', thus enter
    // the directory, and then switch it back again; all following calls to 'Next' will then iterate through the sub directory

    const bool bRecursive = m_Flags.IsSet(xiiFileSystemIteratorFlags::Recursive);
    m_Flags.Add(xiiFileSystemIteratorFlags::Recursive);

    Next();

    m_Flags.AddOrRemove(xiiFileSystemIteratorFlags::Recursive, bRecursive);
    return;
  }

  if ((m_CurFile.m_sName == "..") || (m_CurFile.m_sName == "."))
  {
    Next(); // will search for the next file or folder that is not ".." or "." ; might return false though
    return;
  }

  if (m_CurFile.m_bIsDirectory)
  {
    if (!m_Flags.IsSet(xiiFileSystemIteratorFlags::ReportFolders))
    {
      Next();
      return;
    }
  }
  else
  {
    if (!m_Flags.IsSet(xiiFileSystemIteratorFlags::ReportFiles))
    {
      Next();
      return;
    }
  }
}

xiiInt32 xiiFileSystemIterator::InternalNext()
{
  constexpr xiiInt32 ReturnFailure          = 0;
  constexpr xiiInt32 ReturnSuccess          = 1;
  constexpr xiiInt32 ReturnCallInternalNext = 2;

  if (m_Data.m_Handles.IsEmpty())
    return ReturnFailure;

  if (m_Flags.IsSet(xiiFileSystemIteratorFlags::Recursive) && m_CurFile.m_bIsDirectory && (m_CurFile.m_sName != "..") && (m_CurFile.m_sName != "."))
  {
    m_sCurPath.AppendPath(m_CurFile.m_sName);

    xiiStringBuilder sNewSearch = m_sCurPath;
    sNewSearch.AppendPath("*");

    WIN32_FIND_DATAW data;
    HANDLE           hSearch = FindFirstFileW(xiiDosDevicePath(sNewSearch), &data);

    if ((hSearch != nullptr) && (hSearch != INVALID_HANDLE_VALUE))
    {
      m_CurFile.m_uiFileSize           = HighLowToUInt64(data.nFileSizeHigh, data.nFileSizeLow);
      m_CurFile.m_bIsDirectory         = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
      m_CurFile.m_sParentPath          = m_sCurPath;
      m_CurFile.m_sName                = data.cFileName;
      m_CurFile.m_LastModificationTime = xiiTimestamp::MakeFromInt(FileTimeToEpoch(data.ftLastWriteTime), xiiSIUnitOfTime::Microsecond);

      m_Data.m_Handles.PushBack(hSearch);

      if ((m_CurFile.m_sName == "..") || (m_CurFile.m_sName == "."))
        return ReturnCallInternalNext; // will search for the next file or folder that is not ".." or "." ; might return false though

      if (m_CurFile.m_bIsDirectory)
      {
        if (!m_Flags.IsSet(xiiFileSystemIteratorFlags::ReportFolders))
          return ReturnCallInternalNext;
      }
      else
      {
        if (!m_Flags.IsSet(xiiFileSystemIteratorFlags::ReportFiles))
          return ReturnCallInternalNext;
      }

      return ReturnSuccess;
    }

    // if the recursion did not work, just iterate in this folder further
  }

  WIN32_FIND_DATAW data;
  if (!FindNextFileW(m_Data.m_Handles.PeekBack(), &data))
  {
    // nothing found in this directory anymore
    FindClose(m_Data.m_Handles.PeekBack());
    m_Data.m_Handles.PopBack();

    if (m_Data.m_Handles.IsEmpty())
      return ReturnFailure;

    m_sCurPath.PathParentDirectory();
    if (m_sCurPath.EndsWith("/"))
    {
      m_sCurPath.Shrink(0, 1); // Remove trailing /
    }

    return ReturnCallInternalNext;
  }

  m_CurFile.m_uiFileSize           = HighLowToUInt64(data.nFileSizeHigh, data.nFileSizeLow);
  m_CurFile.m_bIsDirectory         = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
  m_CurFile.m_sParentPath          = m_sCurPath;
  m_CurFile.m_sName                = data.cFileName;
  m_CurFile.m_LastModificationTime = xiiTimestamp::MakeFromInt(FileTimeToEpoch(data.ftLastWriteTime), xiiSIUnitOfTime::Microsecond);

  if ((m_CurFile.m_sName == "..") || (m_CurFile.m_sName == "."))
    return ReturnCallInternalNext;

  if (m_CurFile.m_bIsDirectory)
  {
    if (!m_Flags.IsSet(xiiFileSystemIteratorFlags::ReportFolders))
      return ReturnCallInternalNext;
  }
  else
  {
    if (!m_Flags.IsSet(xiiFileSystemIteratorFlags::ReportFiles))
      return ReturnCallInternalNext;
  }

  return ReturnSuccess;
}

#endif

xiiStringView xiiOSFile::GetApplicationPath()
{
  if (s_sApplicationPath.IsEmpty())
  {
    xiiUInt32                     uiRequiredLength = 512;
    xiiHybridArray<wchar_t, 1024> tmp;

    while (true)
    {
      tmp.SetCountUninitialized(uiRequiredLength);

      // reset last error code
      SetLastError(ERROR_SUCCESS);

      const xiiUInt32 uiLength = GetModuleFileNameW(nullptr, tmp.GetData(), tmp.GetCount() - 1);
      const DWORD     error    = GetLastError();

      if (error == ERROR_SUCCESS)
      {
        tmp[uiLength] = L'\0';
        break;
      }

      if (error == ERROR_INSUFFICIENT_BUFFER)
      {
        uiRequiredLength += 512;
        continue;
      }

      XII_REPORT_FAILURE("GetModuleFileNameW failed: {0}", xiiArgErrorCode(error));
    }

    s_sApplicationPath = xiiStringUtf8(tmp.GetData()).GetData();
  }

  return s_sApplicationPath;
}

xiiString xiiOSFile::GetUserDataFolder(xiiStringView sSubFolder)
{
  if (s_sUserDataPath.IsEmpty())
  {
    wchar_t* pPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, nullptr, &pPath)))
    {
      s_sUserDataPath = xiiStringWChar(pPath);
    }

    if (pPath != nullptr)
    {
      CoTaskMemFree(pPath);
    }
  }

  xiiStringBuilder s = s_sUserDataPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

xiiString xiiOSFile::GetTempDataFolder(xiiStringView sSubFolder /*= nullptr*/)
{
  xiiStringBuilder s;

  if (s_sTempDataPath.IsEmpty())
  {
    wchar_t* pPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &pPath)))
    {
      s = xiiStringWChar(pPath);
      s.AppendPath("Temp");
      s_sTempDataPath = s;
    }

    if (pPath != nullptr)
    {
      CoTaskMemFree(pPath);
    }
  }

  s = s_sTempDataPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

xiiString xiiOSFile::GetUserDocumentsFolder(xiiStringView sSubFolder /*= {}*/)
{
  if (s_sUserDocumentsPath.IsEmpty())
  {
    wchar_t* pPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_PublicDocuments, KF_FLAG_DEFAULT, nullptr, &pPath)))
    {
      s_sUserDocumentsPath = xiiStringWChar(pPath);
    }

    if (pPath != nullptr)
    {
      CoTaskMemFree(pPath);
    }
  }

  xiiStringBuilder s = s_sUserDocumentsPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

const xiiString xiiOSFile::GetCurrentWorkingDirectory()
{
  const xiiUInt32 uiRequiredLength = GetCurrentDirectoryW(0, nullptr);

  xiiHybridArray<wchar_t, 1024> tmp;
  tmp.SetCountUninitialized(uiRequiredLength + 16);

  if (GetCurrentDirectoryW(tmp.GetCount() - 1, tmp.GetData()) == 0)
  {
    XII_REPORT_FAILURE("GetCurrentDirectoryW failed: {}", xiiArgErrorCode(GetLastError()));
    return xiiString();
  }

  tmp[uiRequiredLength] = L'\0';

  xiiStringBuilder clean = xiiStringUtf8(tmp.GetData()).GetData();
  clean.MakeCleanPath();

  return clean;
}
