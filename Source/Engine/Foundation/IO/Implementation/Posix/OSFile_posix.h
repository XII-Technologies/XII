#pragma once

#include <Foundation/FoundationPCH.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <direct.h>
#  define XII_USE_OLD_POSIX_FUNCTIONS XII_ON
#else
#  include <dirent.h>
#  include <fnmatch.h>
#  include <pwd.h>
#  include <sys/file.h>
#  include <sys/types.h>
#  include <unistd.h>
#  define XII_USE_OLD_POSIX_FUNCTIONS XII_OFF
#endif

#if XII_ENABLED(XII_PLATFORM_OSX)
#  include <CoreFoundation/CoreFoundation.h>
#endif

#if XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <Foundation/Basics/Platform/Android/AndroidJni.h>
#  include <Foundation/Basics/Platform/Android/AndroidUtils.h>
#  include <android_native_app_glue.h>
#endif

#ifndef PATH_MAX
#  define PATH_MAX 1024
#endif

xiiResult xiiOSFile::InternalOpen(xiiStringView sFile, xiiFileOpenMode::Enum OpenMode, xiiFileShareMode::Enum FileShareMode)
{
  xiiStringBuilder sFileCopy = sFile;
  const char*      szFile    = sFileCopy;

#if XII_DISABLED(XII_PLATFORM_WINDOWS_UWP) // UWP does not support these functions
  xiiInt32 fd = -1;
  switch (OpenMode)
  {
    // O_CLOEXEC = don't forward to child processes
    case xiiFileOpenMode::Read:
      fd = open(szFile, O_RDONLY | O_CLOEXEC);
      break;
    case xiiFileOpenMode::Write:
    case xiiFileOpenMode::Append:
      fd = open(szFile, O_CREAT | O_WRONLY | O_CLOEXEC, 0644);
      break;
    default:
      break;
  }

  if (FileShareMode == xiiFileShareMode::Default)
  {
    if (OpenMode == xiiFileOpenMode::Read)
    {
      FileShareMode = xiiFileShareMode::SharedReads;
    }
    else
    {
      FileShareMode = xiiFileShareMode::Exclusive;
    }
  }

  if (fd == -1)
  {
    return XII_FAILURE;
  }

  const xiiInt32 iSharedMode = (FileShareMode == xiiFileShareMode::Exclusive) ? LOCK_EX : LOCK_SH;
  const xiiTime  sleepTime   = xiiTime::Milliseconds(20);
  xiiInt32       iRetries    = m_bRetryOnSharingViolation ? 20 : 1;

  while (flock(fd, iSharedMode | LOCK_NB /* do not block */) != 0)
  {
    xiiInt32 errorCode = errno;
    iRetries--;
    if (iRetries == 0 || errorCode != EWOULDBLOCK)
    {
      // error, could not get a lock
      xiiLog::Error("Failed to get a {} lock for file {}, error {}", (FileShareMode == xiiFileShareMode::Exclusive) ? "Exculsive" : "Shared", szFile, errno);
      close(fd);
      return XII_FAILURE;
    }
    xiiThreadUtils::Sleep(sleepTime);
  }

  switch (OpenMode)
  {
    case xiiFileOpenMode::Read:
      m_FileData.m_pFileHandle = fdopen(fd, "rb");
      break;
    case xiiFileOpenMode::Write:
      if (ftruncate(fd, 0) < 0)
      {
        close(fd);
        return XII_FAILURE;
      }
      m_FileData.m_pFileHandle = fdopen(fd, "wb");
      break;
    case xiiFileOpenMode::Append:
      m_FileData.m_pFileHandle = fdopen(fd, "ab");

      // in append mode we need to set the file pointer to the end explicitly, otherwise GetFilePosition might return 0 the first time
      if (m_FileData.m_pFileHandle != nullptr)
        InternalSetFilePosition(0, xiiFileSeekMode::FromEnd);

      break;
    default:
      break;
  }

  if (m_FileData.m_pFileHandle == nullptr)
  {
    close(fd);
  }

#else

  switch (OpenMode)
  {
    case xiiFileOpenMode::Read:
      m_FileData.m_pFileHandle = fopen(szFile, "rb");
      break;
    case xiiFileOpenMode::Write:
      m_FileData.m_pFileHandle = fopen(szFile, "wb");
      break;
    case xiiFileOpenMode::Append:
      m_FileData.m_pFileHandle = fopen(szFile, "ab");

      // in append mode we need to set the file pointer to the end explicitly, otherwise GetFilePosition might return 0 the first time
      if (m_FileData.m_pFileHandle != nullptr)
        InternalSetFilePosition(0, xiiFileSeekMode::FromEnd);

      break;
    default:
      break;
  }
#endif

  if (m_FileData.m_pFileHandle == nullptr)
  {
    return XII_FAILURE;
  }

  // lock will be released automatically when the file is closed
  return XII_SUCCESS;
}

void xiiOSFile::InternalClose()
{
  fclose(m_FileData.m_pFileHandle);
}

xiiResult xiiOSFile::InternalWrite(const void* pBuffer, xiiUInt64 uiBytes)
{
  const xiiUInt32 uiBatchBytes = 1024 * 1024 * 1024; // 1 GB

  // first write out all the data in 1GB batches
  while (uiBytes > uiBatchBytes)
  {
    if (fwrite(pBuffer, 1, uiBatchBytes, m_FileData.m_pFileHandle) != uiBatchBytes)
    {
      xiiLog::Error("fwrite 1GB failed for '{}'", m_sFileName);
      return XII_FAILURE;
    }

    uiBytes -= uiBatchBytes;
    pBuffer = xiiMemoryUtils::AddByteOffset(pBuffer, uiBatchBytes);
  }

  if (uiBytes > 0)
  {
    const xiiUInt32 uiBytes32 = static_cast<xiiUInt32>(uiBytes);

    if (fwrite(pBuffer, 1, uiBytes32, m_FileData.m_pFileHandle) != uiBytes)
    {
      xiiLog::Error("fwrite failed for '{}'", m_sFileName);
      return XII_FAILURE;
    }
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
    const xiiUInt64 uiReadThisTime = fread(pBuffer, 1, uiBatchBytes, m_FileData.m_pFileHandle);
    uiBytesRead += uiReadThisTime;

    if (uiReadThisTime != uiBatchBytes)
      return uiBytesRead;

    uiBytes -= uiBatchBytes;
    pBuffer = xiiMemoryUtils::AddByteOffset(pBuffer, uiBatchBytes);
  }

  if (uiBytes > 0)
  {
    const xiiUInt32 uiBytes32 = static_cast<xiiUInt32>(uiBytes);

    uiBytesRead += fread(pBuffer, 1, uiBytes32, m_FileData.m_pFileHandle);
  }

  return uiBytesRead;
}

xiiUInt64 xiiOSFile::InternalGetFilePosition() const
{
#if XII_ENABLED(XII_USE_OLD_POSIX_FUNCTIONS)
  return static_cast<xiiUInt64>(ftell(m_FileData.m_pFileHandle));
#else
  return static_cast<xiiUInt64>(ftello(m_FileData.m_pFileHandle));
#endif
}

void xiiOSFile::InternalSetFilePosition(xiiInt64 iDistance, xiiFileSeekMode::Enum Pos) const
{
#if XII_ENABLED(XII_USE_OLD_POSIX_FUNCTIONS)
  switch (Pos)
  {
    case xiiFileSeekMode::FromStart:
      XII_VERIFY(fseek(m_FileData.m_pFileHandle, (long)iDistance, SEEK_SET) == 0, "Seek Failed");
      break;
    case xiiFileSeekMode::FromEnd:
      XII_VERIFY(fseek(m_FileData.m_pFileHandle, (long)iDistance, SEEK_END) == 0, "Seek Failed");
      break;
    case xiiFileSeekMode::FromCurrent:
      XII_VERIFY(fseek(m_FileData.m_pFileHandle, (long)iDistance, SEEK_CUR) == 0, "Seek Failed");
      break;
  }
#else
  switch (Pos)
  {
    case xiiFileSeekMode::FromStart:
      XII_VERIFY(fseeko(m_FileData.m_pFileHandle, iDistance, SEEK_SET) == 0, "Seek Failed");
      break;
    case xiiFileSeekMode::FromEnd:
      XII_VERIFY(fseeko(m_FileData.m_pFileHandle, iDistance, SEEK_END) == 0, "Seek Failed");
      break;
    case xiiFileSeekMode::FromCurrent:
      XII_VERIFY(fseeko(m_FileData.m_pFileHandle, iDistance, SEEK_CUR) == 0, "Seek Failed");
      break;
  }
#endif
}

// this might not be defined on Windows
#ifndef S_ISDIR
#  define S_ISDIR(m) (((m)&S_IFMT) == S_IFDIR)
#endif

bool xiiOSFile::InternalExistsFile(xiiStringView sFile)
{
  struct stat sb;
  return (stat(xiiString(sFile), &sb) == 0 && !S_ISDIR(sb.st_mode));
}

bool xiiOSFile::InternalExistsDirectory(xiiStringView sDirectory)
{
  struct stat sb;
  return (stat(xiiString(sDirectory), &sb) == 0 && S_ISDIR(sb.st_mode));
}

xiiResult xiiOSFile::InternalDeleteFile(xiiStringView sFile)
{
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  xiiInt32 iRes = _unlink(xiiString(sFile));
#else
  xiiInt32 iRes = unlink(xiiString(sFile));
#endif

  if (iRes == 0 || (iRes == -1 && errno == ENOENT))
    return XII_SUCCESS;

  return XII_FAILURE;
}

xiiResult xiiOSFile::InternalDeleteDirectory(xiiStringView sDirectory)
{
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  xiiInt32 iRes = _rmdir(xiiString(sDirectory));
#else
  xiiInt32 iRes = rmdir(xiiString(sDirectory));
#endif

  if (iRes == 0 || (iRes == -1 && errno == ENOENT))
    return XII_SUCCESS;

  return XII_FAILURE;
}

xiiResult xiiOSFile::InternalCreateDirectory(xiiStringView sDirectory)
{
  // handle drive letters as always successful
  if (xiiStringUtils::GetCharacterCount(sDirectory.GetStartPointer(), sDirectory.GetEndPointer()) <= 1) // '/'
    return XII_SUCCESS;

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  xiiInt32 iRes = _mkdir(xiiString(sDirectory));
#else
  xiiInt32 iRes = mkdir(xiiString(sDirectory), 0777);
#endif

  if (iRes == 0 || (iRes == -1 && errno == EEXIST))
    return XII_SUCCESS;

  // If we were not allowed to access the folder but it alreay exists, we treat the operation as successful.
  // Note that this is espcially relevant for calls to xiiOSFile::CreateDirectoryStructure where we may call mkdir on top level directories that are
  // not accessible.
  if (errno == EACCES && InternalExistsDirectory(sDirectory))
    return XII_SUCCESS;

  return XII_FAILURE;
}

xiiResult xiiOSFile::InternalMoveFileOrDirectory(xiiStringView sDirectoryFrom, xiiStringView sDirectoryTo)
{
  if (rename(xiiString(sDirectoryFrom), xiiString(sDirectoryTo)) != 0)
  {
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

#if XII_ENABLED(XII_SUPPORTS_FILE_STATS) && XII_DISABLED(XII_PLATFORM_WINDOWS_UWP)
xiiResult xiiOSFile::InternalGetFileStats(xiiStringView sFileOrFolder, xiiFileStats& out_Stats)
{
  struct stat tempStat;
  xiiInt32    iRes = stat(xiiString(sFileOrFolder), &tempStat);

  if (iRes != 0)
    return XII_FAILURE;

  out_Stats.m_bIsDirectory = S_ISDIR(tempStat.st_mode);
  out_Stats.m_uiFileSize   = tempStat.st_size;
  out_Stats.m_sParentPath  = sFileOrFolder;
  out_Stats.m_sParentPath.PathParentDirectory();
  out_Stats.m_sName = xiiPathUtils::GetFileNameAndExtension(sFileOrFolder); // no OS support, so just pass it through
  out_Stats.m_LastModificationTime.SetInt64(tempStat.st_mtime, xiiSIUnitOfTime::Second);

  return XII_SUCCESS;
}
#endif

#if XII_DISABLED(XII_PLATFORM_WINDOWS_UWP)

xiiStringView xiiOSFile::GetApplicationDirectory()
{
  static xiiString256 s_Path;

  if (s_Path.IsEmpty())
  {
#  if XII_ENABLED(XII_PLATFORM_OSX)

    CFBundleRef appBundle  = CFBundleGetMainBundle();
    CFURLRef    bundleURL  = CFBundleCopyBundleURL(appBundle);
    CFStringRef bundlePath = CFURLCopyFileSystemPath(bundleURL, kCFURLPOSIXPathStyle);

    if (bundlePath != nullptr)
    {
      CFIndex length  = CFStringGetLength(bundlePath);
      CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;

      xiiArrayPtr<char> temp = XII_DEFAULT_NEW_ARRAY(char, static_cast<xiiUInt32>(maxSize));

      if (CFStringGetCString(bundlePath, temp.GetPtr(), maxSize, kCFStringEncodingUTF8))
      {
        s_Path = temp.GetPtr();
      }

      XII_DEFAULT_DELETE_ARRAY(temp);
    }

    CFRelease(bundlePath);
    CFRelease(bundleURL);
    CFRelease(appBundle);
#  elif XII_ENABLED(XII_PLATFORM_ANDROID)
    {
      xiiJniAttachment attachment;

      xiiJniString packagePath = attachment.GetActivity().Call<xiiJniString>("getPackageCodePath");
      // By convention, android requires assets to be placed in the 'Assets' folder
      // inside the apk thus we use that as our SDK root.
      xiiStringBuilder sTemp = packagePath.GetData();
      sTemp.AppendPath("Assets");
      s_Path = sTemp;
    }
#  else
    char             result[PATH_MAX];
    ssize_t          length = readlink("/proc/self/exe", result, PATH_MAX);
    xiiStringBuilder path(xiiStringView(result, result + length));
    s_Path = path.GetFileDirectory();
#  endif
  }

  return s_Path;
}

xiiString xiiOSFile::GetUserDataFolder(xiiStringView sSubFolder)
{
  if (s_sUserDataPath.IsEmpty())
  {
#  if XII_ENABLED(XII_PLATFORM_ANDROID)
    android_app* pAndroidApp = xiiAndroidUtils::GetNativeAndroidApp();
    s_sUserDataPath          = pAndroidApp->activity->internalDataPath;
#  else
    s_sUserDataPath = getenv("HOME");

    if (s_sUserDataPath.IsEmpty())
      s_sUserDataPath = getpwuid(getuid())->pw_dir;
#  endif
  }

  xiiStringBuilder s = s_sUserDataPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

xiiString xiiOSFile::GetTempDataFolder(xiiStringView sSubFolder)
{
  if (s_sTempDataPath.IsEmpty())
  {
#  if XII_ENABLED(XII_PLATFORM_ANDROID)
    xiiJniAttachment attachment;

    xiiJniObject cacheDir = attachment.GetActivity().Call<xiiJniObject>("getCacheDir");
    xiiJniString path     = cacheDir.Call<xiiJniString>("getPath");
    s_sTempDataPath       = path.GetData();
#  else
    s_sTempDataPath = GetUserDataFolder(".cache").GetData();
#  endif
  }

  xiiStringBuilder s = s_sTempDataPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

xiiString xiiOSFile::GetUserDocumentsFolder(xiiStringView sSubFolder)
{
  if (s_sUserDocumentsPath.IsEmpty())
  {
#  if XII_ENABLED(XII_PLATFORM_ANDROID)
    android_app* pAndroidApp = xiiAndroidUtils::GetNativeAndroidApp();
    // s_sUserDataPath = pAndroidApp->activity->internalDataPath;
    XII_ASSERT_NOT_IMPLEMENTED;
#  else
    s_sUserDataPath = getenv("HOME");

    if (s_sUserDataPath.IsEmpty())
      s_sUserDataPath = getpwuid(getuid())->pw_dir;
#  endif
  }

  xiiStringBuilder s = s_sUserDocumentsPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

const xiiString xiiOSFile::GetCurrentWorkingDirectory()
{
  char tmp[PATH_MAX];

  xiiStringBuilder clean = getcwd(tmp, XII_ARRAY_SIZE(tmp));
  clean.MakeCleanPath();

  return clean;
}

#  if XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS)

xiiFileSystemIterator::xiiFileSystemIterator() = default;

xiiFileSystemIterator::~xiiFileSystemIterator()
{
  while (!m_Data.m_Handles.IsEmpty())
  {
    closedir((DIR*)m_Data.m_Handles.PeekBack());
    m_Data.m_Handles.PopBack();
  }
}

bool xiiFileSystemIterator::IsValid() const
{
  return !m_Data.m_Handles.IsEmpty();
}

namespace
{
  xiiResult UpdateCurrentFile(xiiFileStats& curFile, const xiiStringBuilder& curPath, DIR* hSearch, const xiiString& wildcardSearch)
  {
    struct dirent* hCurrentFile = readdir(hSearch);
    if (hCurrentFile == nullptr)
      return XII_FAILURE;

    if (!wildcardSearch.IsEmpty())
    {
      while (fnmatch(wildcardSearch.GetData(), hCurrentFile->d_name, FNM_NOESCAPE) != 0)
      {
        hCurrentFile = readdir(hSearch);
        if (hCurrentFile == nullptr)
          return XII_FAILURE;
      }
    }

    xiiStringBuilder absFileName = curPath;
    absFileName.AppendPath(hCurrentFile->d_name);

    struct stat fileStat = {};
    stat(absFileName.GetData(), &fileStat);

    curFile.m_uiFileSize   = fileStat.st_size;
    curFile.m_bIsDirectory = hCurrentFile->d_type == DT_DIR;
    curFile.m_sParentPath  = curPath;
    curFile.m_sName        = hCurrentFile->d_name;
    curFile.m_LastModificationTime.SetInt64(fileStat.st_mtime, xiiSIUnitOfTime::Second);

    return XII_SUCCESS;
  }
} // namespace

void xiiFileSystemIterator::StartSearch(xiiStringView sSearchTerm, xiiBitflags<xiiFileSystemIteratorFlags> flags /*= xiiFileSystemIteratorFlags::All*/)
{
  XII_ASSERT_DEV(m_Data.m_Handles.IsEmpty(), "Cannot start another search.");

  m_sSearchTerm = sSearchTerm;

  xiiStringBuilder sSearch = sSearchTerm;
  sSearch.MakeCleanPath();

  // same as just passing in the folder path, so remove this
  if (sSearch.EndsWith("/*"))
    sSearch.Shrink(0, 2);

  // Remove a trailing slash if any
  sSearch.Trim(nullptr, "/");

  // Since the use of wildcard-ed file names will disable recursion, we ensure both are not used simultaneously.
  const bool bHasWildcard = sSearch.FindLastSubString("*") || sSearch.FindLastSubString("?");
  if (flags.IsSet(xiiFileSystemIteratorFlags::Recursive) == true && bHasWildcard == true)
  {
    XII_ASSERT_DEV(false, "Recursive file iteration does not support wildcards. Either don't use recursion, or filter the filenames manually.");
    return;
  }

  if (bHasWildcard)
  {
    m_Data.m_wildcardSearch = sSearch.GetFileNameAndExtension();
    m_sCurPath              = sSearch.GetFileDirectory();
  }
  else
  {
    m_Data.m_wildcardSearch.Clear();
    m_sCurPath = sSearch;
  }

  XII_ASSERT_DEV(m_sCurPath.IsAbsolutePath(), "The path '{0}' is not absolute.", m_sCurPath);

  m_Flags = flags;

  DIR* hSearch = opendir(m_sCurPath.GetData());

  if (hSearch == nullptr)
    return;

  if (UpdateCurrentFile(m_CurFile, m_sCurPath, hSearch, m_Data.m_wildcardSearch).Failed())
  {
    return;
  }

  m_Data.m_Handles.PushBack(hSearch);

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
  constexpr xiiInt32 CallInternalNext = 2;

  if (m_Data.m_Handles.IsEmpty())
    return XII_FAILURE;

  if (m_Flags.IsSet(xiiFileSystemIteratorFlags::Recursive) && m_CurFile.m_bIsDirectory && (m_CurFile.m_sName != "..") && (m_CurFile.m_sName != "."))
  {
    m_sCurPath.AppendPath(m_CurFile.m_sName.GetData());

    DIR* hSearch = opendir(m_sCurPath.GetData());

    if (hSearch != nullptr && UpdateCurrentFile(m_CurFile, m_sCurPath, hSearch, m_Data.m_wildcardSearch).Succeeded())
    {
      m_Data.m_Handles.PushBack(hSearch);

      if ((m_CurFile.m_sName == "..") || (m_CurFile.m_sName == "."))
        return CallInternalNext; // will search for the next file or folder that is not ".." or "." ; might return false though

      if (m_CurFile.m_bIsDirectory)
      {
        if (!m_Flags.IsSet(xiiFileSystemIteratorFlags::ReportFolders))
          return CallInternalNext;
      }
      else
      {
        if (!m_Flags.IsSet(xiiFileSystemIteratorFlags::ReportFiles))
          return CallInternalNext;
      }

      return XII_SUCCESS;
    }

    // if the recursion did not work, just iterate in this folder further
  }

  if (UpdateCurrentFile(m_CurFile, m_sCurPath, (DIR*)m_Data.m_Handles.PeekBack(), m_Data.m_wildcardSearch).Failed())
  {
    // nothing found in this directory anymore
    closedir((DIR*)m_Data.m_Handles.PeekBack());
    m_Data.m_Handles.PopBack();

    if (m_Data.m_Handles.IsEmpty())
      return XII_FAILURE;

    m_sCurPath.PathParentDirectory();
    if (m_sCurPath.GetElementCount() > 1 && m_sCurPath.EndsWith("/"))
    {
      m_sCurPath.Shrink(0, 1);
    }

    return CallInternalNext;
  }

  if ((m_CurFile.m_sName == "..") || (m_CurFile.m_sName == "."))
    return CallInternalNext;

  if (m_CurFile.m_bIsDirectory)
  {
    if (!m_Flags.IsSet(xiiFileSystemIteratorFlags::ReportFolders))
      return CallInternalNext;
  }
  else
  {
    if (!m_Flags.IsSet(xiiFileSystemIteratorFlags::ReportFiles))
      return CallInternalNext;
  }

  return XII_SUCCESS;
}

#  endif

#endif // XII_DISABLED(XII_PLATFORM_WINDOWS_UWP)
