/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FileservePlugin/FileservePluginPCH.h>

#include <FileservePlugin/Fileserver/ClientContext.h>
#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/StringConversion.h>
#include <filesystem>

namespace
{
  namespace fs = std::filesystem;

  fs::path NativePath(xiiStringView sPath)
  {
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    return fs::path(xiiStringWChar(sPath).GetData());
#else
    return fs::path(xiiStringBuilder(sPath).GetData());
#endif
  }
} // namespace

bool xiiFileserveClientContext::IsValidRelativePath(xiiStringView sPath)
{
  if (sPath.IsEmpty() || sPath.GetElementCount() > 4096 || sPath.StartsWith("/") || sPath.EndsWith("/"))
    return false;

  for (const auto c : sPath)
  {
    if (c < 32 || c == '\\' || c == ':' || c == '<' || c == '>' || c == '"' || c == '|' || c == '?' || c == '*')
      return false;
  }

  while (!sPath.IsEmpty())
  {
    const char*         pSlash = sPath.FindSubString("/");
    const xiiStringView segment(sPath.GetStartPointer(), pSlash != nullptr ? pSlash : sPath.GetEndPointer());
    if (segment.IsEmpty() || segment.EndsWith(".") || segment.EndsWith(" "))
      return false;

    const char*         pDot = segment.FindSubString(".");
    const xiiStringView stem(segment.GetStartPointer(), pDot != nullptr ? pDot : segment.GetEndPointer());
    if (stem.IsEqual_NoCase("CON") || stem.IsEqual_NoCase("PRN") || stem.IsEqual_NoCase("AUX") || stem.IsEqual_NoCase("NUL") ||
        (stem.GetElementCount() == 4 && (stem.StartsWith_NoCase("COM") || stem.StartsWith_NoCase("LPT")) &&
         stem.GetStartPointer()[3] >= '0' && stem.GetStartPointer()[3] <= '9'))
      return false;

    if (pSlash == nullptr)
      break;
    sPath = xiiStringView(pSlash + 1, sPath.GetEndPointer());
  }
  return true;
}

xiiResult xiiFileserveClientContext::ResolveFilePath(xiiStringView sRoot, xiiStringView sRelativePath, xiiStringBuilder& out_sFile, bool bDirectory)
{
  if (!IsValidRelativePath(sRelativePath))
    return XII_FAILURE;

  std::error_code error;
  const auto      root = fs::canonical(NativePath(sRoot), error);
  if (error || !fs::is_directory(root, error) || error)
    return XII_FAILURE;

  auto candidate = root;
  for (const auto& component : NativePath(sRelativePath))
  {
    candidate /= component;
    const auto status = fs::symlink_status(candidate, error);
    if (fs::is_symlink(status) || (error && error != std::errc::no_such_file_or_directory))
      return XII_FAILURE;
  }

  // Canonicalization also detects filesystem indirections which symlink_status may classify as directories.
  candidate = fs::weakly_canonical(candidate, error);
  if (error)
    return XII_FAILURE;

  auto segment = candidate.begin();
  for (const auto& rootSegment : root)
  {
    if (segment == candidate.end() || *segment != rootSegment)
      return XII_FAILURE;
    ++segment;
  }
  if (segment == candidate.end())
    return XII_FAILURE;

  const auto status = fs::status(candidate, error);
  if ((error && error != std::errc::no_such_file_or_directory) ||
      (fs::exists(status) && (bDirectory ? !fs::is_directory(status) : !fs::is_regular_file(status))))
    return XII_FAILURE;

  const xiiStringUtf8 utf8(candidate.c_str());
  out_sFile = utf8.GetData();
  out_sFile.MakeCleanPath();
  return XII_SUCCESS;
}

xiiResult xiiFileserveClientContext::HashFile(xiiOSFile& ref_file, xiiUInt64 uiSize, xiiUInt64& out_uiHash)
{
  if (ref_file.GetFileSize() != uiSize)
    return XII_FAILURE;

  xiiHashStreamWriter64 hash(1);
  xiiUInt8              bytes[16 * 1024];
  xiiUInt64             uiRemaining = uiSize;
  while (uiRemaining != 0)
  {
    const auto uiRead = xiiMath::Min<xiiUInt64>(uiRemaining, sizeof(bytes));
    if (ref_file.Read(bytes, uiRead) != uiRead || hash.WriteBytes(bytes, uiRead).Failed())
      return XII_FAILURE;
    uiRemaining -= uiRead;
  }
  out_uiHash = uiSize == 0 ? 1 : hash.GetHashValue();
  return XII_SUCCESS;
}

xiiFileserveFileState xiiFileserveClientContext::GetFileStatus(xiiUInt16& inout_uiDataDirID, xiiStringView sRequestedFile, FileStatus& inout_status, xiiOSFile& out_file, bool bForceThisDataDir) const
{
  if (!IsValidRelativePath(sRequestedFile) || inout_uiDataDirID >= m_MountedDataDirs.GetCount())
    return xiiFileserveFileState::None;

  for (xiiUInt32 i = m_MountedDataDirs.GetCount(); i > 0; --i)
  {
    const auto  uiDataDirID = static_cast<xiiUInt16>(i - 1);
    const auto& dir         = m_MountedDataDirs[uiDataDirID];
    if (!dir.m_bMounted || (bForceThisDataDir && uiDataDirID != inout_uiDataDirID))
      continue;

    xiiStringBuilder sAbsPath;
    xiiFileStats     stat;
    if (ResolveFilePath(dir.m_sPathOnServer, sRequestedFile, sAbsPath).Failed() ||
        xiiOSFile::GetFileStats(sAbsPath, stat).Failed() || stat.m_bIsDirectory ||
        out_file.Open(sAbsPath, xiiFileOpenMode::Read).Failed())
      continue;

    const auto uiFileSize = out_file.GetFileSize();
    xiiUInt64  uiHash     = 0;
    if (uiFileSize > xiiMath::MaxValue<xiiUInt32>() || HashFile(out_file, uiFileSize, uiHash).Failed())
    {
      out_file.Close();
      return xiiFileserveFileState::None;
    }

    const auto iTimestamp     = stat.m_LastModificationTime.GetInt64(xiiSIUnitOfTime::Microsecond);
    const bool bSameHash      = inout_status.m_uiHash == uiHash && inout_uiDataDirID == uiDataDirID;
    const bool bSameTimestamp = inout_status.m_iTimestamp == iTimestamp;
    inout_status              = {iTimestamp, uiHash, uiFileSize};
    inout_uiDataDirID         = uiDataDirID;
    if (bSameHash)
    {
      out_file.Close();
      return bSameTimestamp ? xiiFileserveFileState::SameTimestamp : xiiFileserveFileState::SameHash;
    }

    out_file.SetFilePosition(0, xiiFileSeekMode::FromStart);
    return xiiFileserveFileState::Different;
  }

  const bool bPreviouslyCached = inout_status.m_uiHash != 0;
  inout_status                 = {0, 0, 0};
  if (!bForceThisDataDir)
    inout_uiDataDirID = 0xffff;
  return bPreviouslyCached ? xiiFileserveFileState::NonExistant : xiiFileserveFileState::NonExistantEither;
}
