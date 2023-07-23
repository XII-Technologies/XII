#include <Foundation/FoundationPCH.h>

#include <Foundation/Strings/Implementation/StringIterator.h>
#include <Foundation/Strings/StringBuilder.h>

const char* xiiPathUtils::FindPreviousSeparator(const char* szPathStart, const char* szStartSearchAt)
{
  if (xiiStringUtils::IsNullOrEmpty(szPathStart))
    return nullptr;

  while (szStartSearchAt > szPathStart)
  {
    xiiUnicodeUtils::MoveToPriorUtf8(szStartSearchAt);

    if (IsPathSeparator(*szStartSearchAt))
      return szStartSearchAt;
  }

  return nullptr;
}

bool xiiPathUtils::HasAnyExtension(xiiStringView sPath)
{
  const char* szDot = xiiStringUtils::FindLastSubString(sPath.GetStartPointer(), ".", nullptr, sPath.GetEndPointer());

  if (szDot == nullptr)
    return false;

  // find the last separator in the string
  const char* szSeparator = FindPreviousSeparator(sPath.GetStartPointer(), sPath.GetEndPointer());

  return (szSeparator < szDot);
}

bool xiiPathUtils::HasExtension(xiiStringView sPath, xiiStringView sExtension)
{
  if (xiiStringUtils::StartsWith(sExtension.GetStartPointer(), ".", sExtension.GetEndPointer()))
    return xiiStringUtils::EndsWith_NoCase(sPath.GetStartPointer(), sExtension.GetStartPointer(), sPath.GetEndPointer(), sExtension.GetEndPointer());

  xiiStringBuilder sExt;
  sExt.Append(".", sExtension);

  return xiiStringUtils::EndsWith_NoCase(sPath.GetStartPointer(), sExt.GetData(), sPath.GetEndPointer());
}

xiiStringView xiiPathUtils::GetFileExtension(xiiStringView sPath)
{
  const char* szDot = xiiStringUtils::FindLastSubString(sPath.GetStartPointer(), ".", nullptr, sPath.GetEndPointer());

  if (szDot == nullptr)
    return xiiStringView(nullptr);

  // find the last separator in the string
  const char* szSeparator = FindPreviousSeparator(sPath.GetStartPointer(), sPath.GetEndPointer());

  if (szSeparator > szDot)
    return xiiStringView(nullptr);

  return xiiStringView(szDot + 1, sPath.GetEndPointer());
}

xiiStringView xiiPathUtils::GetFileNameAndExtension(xiiStringView sPath)
{
  const char* szSeparator = FindPreviousSeparator(sPath.GetStartPointer(), sPath.GetEndPointer());

  if (szSeparator == nullptr)
    return sPath;

  return xiiStringView(szSeparator + 1, sPath.GetEndPointer());
}

xiiStringView xiiPathUtils::GetFileName(xiiStringView sPath)
{
  const char* szSeparator = FindPreviousSeparator(sPath.GetStartPointer(), sPath.GetEndPointer());

  const char* szDot = xiiStringUtils::FindLastSubString(sPath.GetStartPointer(), ".", sPath.GetEndPointer());

  if (szDot < szSeparator) // includes (szDot == nullptr), szSeparator will never be nullptr here -> no extension
  {
    return xiiStringView(szSeparator + 1, sPath.GetEndPointer());
  }

  if (szSeparator == nullptr)
  {
    if (szDot == nullptr) // no folder, no extension -> the entire thing is just a name
      return sPath;

    return xiiStringView(sPath.GetStartPointer(), szDot); // no folder, but an extension -> remove the extension
  }

  // now: there is a separator AND an extension

  return xiiStringView(szSeparator + 1, szDot);
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

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
const char xiiPathUtils::OsSpecificPathSeparator = '\\';
#elif XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_ANDROID)
const char xiiPathUtils::OsSpecificPathSeparator = '/';
#elif XII_ENABLED(XII_PLATFORM_OSX)
const char xiiPathUtils::OsSpecificPathSeparator = '/';
#else
#  error "Unknown platform."
#endif

bool xiiPathUtils::IsAbsolutePath(xiiStringView sPath)
{
  if (sPath.GetElementCount() < 2)
    return false;

  const char* szPath = sPath.GetStartPointer();

  // szPath[0] will not be \0 -> so we can access szPath[1] without problems

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  /// if it is an absolute path, character 0 must be ASCII (A - Z)
  /// checks for local paths, i.e. 'C:\stuff' and UNC paths, i.e. '\\server\stuff'
  /// not sure if we should handle '//' identical to '\\' (currently we do)
  return ((szPath[1] == ':') || (IsPathSeparator(szPath[0]) && IsPathSeparator(szPath[1])));
#elif XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_ANDROID)
  return (szPath[0] == '/');
#elif XII_ENABLED(XII_PLATFORM_OSX)
  return (szPath[0] == '/');
#else
#  error "Unknown platform."
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
    xiiUnicodeUtils::MoveToNextUtf8(szStart, szPathEnd);

    if (*szStart == '\0')
      return;

  } while (IsPathSeparator(*szStart));

  const char* szEnd = szStart;
  xiiUnicodeUtils::MoveToNextUtf8(szEnd, szPathEnd);

  while (*szEnd != '\0' && !IsPathSeparator(*szEnd))
    xiiUnicodeUtils::MoveToNextUtf8(szEnd, szPathEnd);

  ref_sRoot = xiiStringView(szStart, szEnd);
  if (*szEnd == '\0')
  {
    ref_sRelPath = xiiStringView();
  }
  else
  {
    // skip path separator for the relative path
    xiiUnicodeUtils::MoveToNextUtf8(szEnd, szPathEnd);
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

bool xiiPathUtils::IsSubPath(xiiStringView sPrefixPath, xiiStringView sFullPath)
{
  /// \test this is new

  xiiStringBuilder tmp = sPrefixPath;
  tmp.MakeCleanPath();
  tmp.AppendPath("");

  return sFullPath.StartsWith_NoCase(tmp);
}

XII_STATICLINK_FILE(Foundation, Foundation_Strings_Implementation_PathUtils);
