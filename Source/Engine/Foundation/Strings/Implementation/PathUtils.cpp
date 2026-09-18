/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Strings/Implementation/StringIterator.h>
#include <Foundation/Strings/StringBuilder.h>

const char* xiiPathUtils::FindPreviousSeparator(const char* szPathStart, const char* szStartSearchAt)
{
  if (xiiStringUtils::IsNullOrEmpty(szPathStart))
    return nullptr;

  while (szStartSearchAt > szPathStart)
  {
    xiiUnicodeUtils::MoveToPriorUtf8(szStartSearchAt, szPathStart).AssertSuccess();

    if (IsPathSeparator(*szStartSearchAt))
      return szStartSearchAt;
  }

  return nullptr;
}

bool xiiPathUtils::HasAnyExtension(xiiStringView sPath)
{
  return !GetFileExtension(sPath, true).IsEmpty();
}

bool xiiPathUtils::HasExtension(xiiStringView sPath, xiiStringView sExtension)
{
  sPath                 = GetFileNameAndExtension(sPath);
  xiiStringView fullExt = GetFileExtension(sPath, true);

  if (sExtension.IsEmpty() && fullExt.IsEmpty())
    return true;

  // if there is a single dot at the start of the extension, remove it
  if (sExtension.StartsWith("."))
    sExtension.ChopAwayFirstCharacterAscii();

  if (!fullExt.EndsWith_NoCase(sExtension))
    return false;

  // remove the checked extension
  sPath = xiiStringView(sPath.GetStartPointer(), sPath.GetEndPointer() - sExtension.GetElementCount());

  // checked extension didn't start with a dot -> make sure there is one at the end of sPath
  if (!sPath.EndsWith("."))
    return false;

  // now make sure the rest isn't just the dot
  return sPath.GetElementCount() > 1;
}

xiiStringView xiiPathUtils::GetFileExtension(xiiStringView sPath, bool bFullExtension)
{
  // get rid of any path before the filename
  sPath = GetFileNameAndExtension(sPath);

  // ignore all dots that the file name may start with (".", "..", ".file", "..file", etc)
  // filename may be empty afterwards, which means no dot will be found -> no extension
  while (sPath.StartsWith("."))
    sPath.ChopAwayFirstCharacterAscii();

  const char* szDot;

  if (bFullExtension)
  {
    szDot = sPath.FindSubString(".");
  }
  else
  {
    szDot = sPath.FindLastSubString(".");
  }

  // no dot at all -> no extension
  if (szDot == nullptr)
    return xiiStringView();

  // dot at the very end of the string -> not an extension
  if (szDot + 1 == sPath.GetEndPointer())
    return xiiStringView();

  return xiiStringView(szDot + 1, sPath.GetEndPointer());
}

xiiStringView xiiPathUtils::GetFileNameAndExtension(xiiStringView sPath)
{
  const char* szSeparator = FindPreviousSeparator(sPath.GetStartPointer(), sPath.GetEndPointer());

  if (szSeparator == nullptr)
    return sPath;

  return xiiStringView(szSeparator + 1, sPath.GetEndPointer());
}

xiiStringView xiiPathUtils::GetFileName(xiiStringView sPath, bool bRemoveFullExtension)
{
  // reduce the problem to just the filename + extension
  sPath = GetFileNameAndExtension(sPath);

  return GetWithoutExtension(sPath, bRemoveFullExtension);
}

xiiStringView xiiPathUtils::GetWithoutExtension(xiiStringView sPath, bool bRemoveFullExtension)
{
  xiiStringView sExtension = GetFileExtension(sPath, bRemoveFullExtension);

  if (sExtension.IsEmpty())
    return sPath;

  return xiiStringView(sPath.GetStartPointer(), sPath.GetEndPointer() - sExtension.GetElementCount() - 1);
}

xiiStringView xiiPathUtils::GetFileDirectory(xiiStringView sPath)
{
  auto it = rbegin(sPath);

  // if it already ends in a path separator, do not return a different directory
  if (IsPathSeparator(it.GetCharacter()))
    return sPath;

  // find the last separator in the string
  const char* szSeparator = FindPreviousSeparator(sPath.GetStartPointer(), sPath.GetEndPointer());

  // no path separator -> root dir -> return the empty path
  if (szSeparator == nullptr)
    return xiiStringView(nullptr);

  return xiiStringView(sPath.GetStartPointer(), szSeparator + 1);
}

const char xiiPathUtils::OsSpecificPathSeparator = XII_PLATFORM_PATH_SEPARATOR;

bool xiiPathUtils::IsAbsolutePath(xiiStringView sPath)
{
  if (sPath.GetElementCount() < 1)
    return false;

  const char* szPath = sPath.GetStartPointer();

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  if (sPath.GetElementCount() < 2)
    return false;

  // szPath[0] will not be \0 -> so we can access szPath[1] without problems

  /// if it is an absolute path, character 0 must be ASCII (A - Z)
  /// checks for local paths, i.e. 'C:\stuff' and UNC paths, i.e. '\\server\stuff'
  /// not sure if we should handle '//' identical to '\\' (currently we do)
  return ((szPath[1] == ':') || (IsPathSeparator(szPath[0]) && IsPathSeparator(szPath[1])));
#else
  return (szPath[0] == '/');
#endif
}

bool xiiPathUtils::IsRelativePath(xiiStringView sPath)
{
  if (sPath.IsEmpty())
    return true;

  // if it starts with a separator, it is not a relative path, ever
  if (xiiPathUtils::IsPathSeparator(*sPath.GetStartPointer()))
    return false;

  return !IsAbsolutePath(sPath) && !IsRootedPath(sPath);
}

bool xiiPathUtils::IsRootedPath(xiiStringView sPath)
{
  return !sPath.IsEmpty() && *sPath.GetStartPointer() == ':';
}

void xiiPathUtils::GetRootedPathParts(xiiStringView sPath, xiiStringView& ref_sRoot, xiiStringView& ref_sRelPath)
{
  ref_sRoot    = xiiStringView();
  ref_sRelPath = sPath;

  if (!IsRootedPath(sPath))
    return;

  const char* szStart   = sPath.GetStartPointer();
  const char* szPathEnd = sPath.GetEndPointer();

  do
  {
    xiiUnicodeUtils::MoveToNextUtf8(szStart, szPathEnd).AssertSuccess();

    if (*szStart == '\0')
      return;

  } while (IsPathSeparator(*szStart));

  const char* szEnd = szStart;
  xiiUnicodeUtils::MoveToNextUtf8(szEnd, szPathEnd).AssertSuccess();

  while (*szEnd != '\0' && !IsPathSeparator(*szEnd))
    xiiUnicodeUtils::MoveToNextUtf8(szEnd, szPathEnd).AssertSuccess();

  ref_sRoot = xiiStringView(szStart, szEnd);
  if (*szEnd == '\0')
  {
    ref_sRelPath = xiiStringView();
  }
  else
  {
    // skip path separator for the relative path
    xiiUnicodeUtils::MoveToNextUtf8(szEnd, szPathEnd).AssertSuccess();
    ref_sRelPath = xiiStringView(szEnd, szPathEnd);
  }
}

xiiStringView xiiPathUtils::GetRootedPathRootName(xiiStringView sPath)
{
  xiiStringView root, relPath;
  GetRootedPathParts(sPath, root, relPath);
  return root;
}

bool xiiPathUtils::IsValidFilenameChar(xiiUInt32 uiCharacter)
{
  /// \test Not tested yet

  // Windows: https://msdn.microsoft.com/library/windows/desktop/aa365247(v=vs.85).aspx
  // Unix: https://en.wikipedia.org/wiki/Filename#Reserved_characters_and_words
  // Details can be more complicated (there might be reserved names depending on the filesystem), but in general all platforms behave like
  // this:
  static const xiiUInt32 forbiddenFilenameChars[] = {'<', '>', ':', '"', '|', '?', '*', '\\', '/', '\t', '\b', '\n', '\r', '\0'};

  for (xiiInt32 i = 0; i < XII_ARRAY_SIZE(forbiddenFilenameChars); ++i)
  {
    if (forbiddenFilenameChars[i] == uiCharacter)
      return false;
  }

  return true;
}

bool xiiPathUtils::ContainsInvalidFilenameChars(xiiStringView sPath)
{
  /// \test Not tested yet

  xiiStringIterator it = sPath.GetIteratorFront();

  for (; it.IsValid(); ++it)
  {
    if (!IsValidFilenameChar(it.GetCharacter()))
      return true;
  }

  return false;
}

void xiiPathUtils::MakeValidFilename(xiiStringView sFilename, xiiUInt32 uiReplacementCharacter, xiiStringBuilder& out_sFilename)
{
  XII_ASSERT_DEBUG(IsValidFilenameChar(uiReplacementCharacter), "Given replacement character is not allowed for filenames.");

  out_sFilename.Clear();

  for (auto it = sFilename.GetIteratorFront(); it.IsValid(); ++it)
  {
    xiiUInt32 currentChar = it.GetCharacter();

    if (IsValidFilenameChar(currentChar) == false)
      out_sFilename.Append(uiReplacementCharacter);
    else
      out_sFilename.Append(currentChar);
  }
}

bool xiiPathUtils::IsSubPath(xiiStringView sPrefixPath, xiiStringView sFullPath0)
{
  if (sPrefixPath.IsEmpty())
  {
    if (sFullPath0.IsAbsolutePath())
      return true;

    XII_REPORT_FAILURE("Prefixpath is empty and checked path is not absolute.");
    return false;
  }

  xiiStringBuilder tmp = sPrefixPath;
  tmp.MakeCleanPath();
  tmp.Trim("", "/");

  xiiStringBuilder sFullPath = sFullPath0;
  sFullPath.MakeCleanPath();

  if (sFullPath.StartsWith(tmp))
  {
    if (tmp.GetElementCount() == sFullPath.GetElementCount())
      return true;

    return sFullPath.GetData()[tmp.GetElementCount()] == '/';
  }

  return false;
}

bool xiiPathUtils::IsSubPath_NoCase(xiiStringView sPrefixPath, xiiStringView sFullPath)
{
  xiiStringBuilder tmp = sPrefixPath;
  tmp.MakeCleanPath();
  tmp.Trim("", "/");

  if (sFullPath.StartsWith_NoCase(tmp))
  {
    if (tmp.GetElementCount() == sFullPath.GetElementCount())
      return true;

    return sFullPath.GetStartPointer()[tmp.GetElementCount()] == '/';
  }

  return false;
}

XII_STATICLINK_FILE(Foundation, Foundation_Strings_Implementation_PathUtils);
