#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/DependencyFile.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>

enum class xiiDependencyFileVersion : xiiUInt8
{
  Version0 = 0,
  Version1,
  Version2, ///< added 'sum' time

  ENUM_COUNT,
  Current = ENUM_COUNT - 1,
};

xiiMutex                                             xiiDependencyFile::s_FileTimestampsLock;
xiiMap<xiiString, xiiDependencyFile::FileCheckCache> xiiDependencyFile::s_FileTimestamps;

xiiDependencyFile::xiiDependencyFile()
{
  Clear();
}

void xiiDependencyFile::Clear()
{
  m_iMaxTimeStampStored  = 0;
  m_uiSumTimeStampStored = 0;
  m_AssetTransformDependencies.Clear();
}

void xiiDependencyFile::AddFileDependency(xiiStringView sFile)
{
  if (sFile.IsEmpty())
    return;

  m_AssetTransformDependencies.PushBack(sFile);
}

void xiiDependencyFile::StoreCurrentTimeStamp()
{
  XII_LOG_BLOCK("xiiDependencyFile::StoreCurrentTimeStamp");

  m_iMaxTimeStampStored  = 0;
  m_uiSumTimeStampStored = 0;

#if XII_DISABLED(XII_SUPPORTS_FILE_STATS)
  xiiLog::Warning("Trying to retrieve file time stamps on a platform that does not support it");
  return;
#endif

  for (const auto& sFile : m_AssetTransformDependencies)
  {
    xiiTimestamp ts;
    if (RetrieveFileTimeStamp(sFile, ts).Failed())
      continue;

    const xiiInt64 time   = ts.GetInt64(xiiSIUnitOfTime::Second);
    m_iMaxTimeStampStored = xiiMath::Max<xiiInt64>(m_iMaxTimeStampStored, time);
    m_uiSumTimeStampStored += (xiiUInt64)time;
  }
}

bool xiiDependencyFile::HasAnyFileChanged() const
{
#if XII_DISABLED(XII_SUPPORTS_FILE_STATS)
  xiiLog::Warning("Trying to retrieve file time stamps on a platform that does not support it");
  return true;
#endif

  xiiUInt64 uiSumTs = 0;

  for (const auto& sFile : m_AssetTransformDependencies)
  {
    xiiTimestamp ts;
    if (RetrieveFileTimeStamp(sFile, ts).Failed())
      continue;

    const xiiInt64 time = ts.GetInt64(xiiSIUnitOfTime::Second);

    if (time > m_iMaxTimeStampStored)
    {
      xiiLog::Dev("Detected file change in '{0}' (TimeStamp {1} > MaxTimeStamp {2})", xiiArgSensitive(sFile, "File"), ts.GetInt64(xiiSIUnitOfTime::Second), m_iMaxTimeStampStored);
      return true;
    }

    uiSumTs += (xiiUInt64)time;
  }

  if (uiSumTs != m_uiSumTimeStampStored)
  {
    xiiLog::Dev("Detected file change, but exact file is not known.");
    return true;
  }

  return false;
}

xiiResult xiiDependencyFile::WriteDependencyFile(xiiStreamWriter& ref_stream) const
{
  ref_stream << (xiiUInt8)xiiDependencyFileVersion::Current;

  ref_stream << m_iMaxTimeStampStored;
  ref_stream << m_uiSumTimeStampStored;
  ref_stream << m_AssetTransformDependencies.GetCount();

  for (const auto& sFile : m_AssetTransformDependencies)
    ref_stream << sFile;

  return XII_SUCCESS;
}

xiiResult xiiDependencyFile::ReadDependencyFile(xiiStreamReader& ref_stream)
{
  xiiUInt8 uiVersion = (xiiUInt8)xiiDependencyFileVersion::Version0;
  ref_stream >> uiVersion;

  if (uiVersion > (xiiUInt8)xiiDependencyFileVersion::Current)
  {
    xiiLog::Error("Dependency file has incorrect file version ({0})", uiVersion);
    return XII_FAILURE;
  }

  XII_ASSERT_DEV(uiVersion <= (xiiUInt8)xiiDependencyFileVersion::Current, "Invalid file version {0}", uiVersion);

  ref_stream >> m_iMaxTimeStampStored;

  if (uiVersion >= (xiiUInt8)xiiDependencyFileVersion::Version2)
  {
    ref_stream >> m_uiSumTimeStampStored;
  }

  xiiUInt32 count = 0;
  ref_stream >> count;
  m_AssetTransformDependencies.SetCount(count);

  for (xiiUInt32 i = 0; i < m_AssetTransformDependencies.GetCount(); ++i)
    ref_stream >> m_AssetTransformDependencies[i];

  return XII_SUCCESS;
}

xiiResult xiiDependencyFile::RetrieveFileTimeStamp(xiiStringView sFile, xiiTimestamp& out_Result)
{
#if XII_ENABLED(XII_SUPPORTS_FILE_STATS)

  XII_LOCK(s_FileTimestampsLock);

  bool bExisted = false;
  auto it       = s_FileTimestamps.FindOrAdd(sFile, &bExisted);

  if (!bExisted || it.Value().m_LastCheck + xiiTime::MakeFromSeconds(2.0) < xiiTime::Now())
  {
    it.Value().m_LastCheck = xiiTime::Now();

    xiiFileStats stats;
    if (xiiFileSystem::GetFileStats(sFile, stats).Failed())
    {
      xiiLog::Error("Could not query the file stats for '{0}'", xiiArgSensitive(sFile, "File"));
      return XII_FAILURE;
    }

    it.Value().m_FileTimestamp = stats.m_LastModificationTime;
  }

  out_Result = it.Value().m_FileTimestamp;

#else

  out_Result = xiiTimestamp::MakeFromInt(0, xiiSIUnitOfTime::Second);
  xiiLog::Warning("Trying to retrieve a file time stamp on a platform that does not support it (file: '{0}')", xiiArgSensitive(sFile, "File"));

#endif

  return out_Result.IsValid() ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiDependencyFile::WriteDependencyFile(xiiStringView sFile) const
{
  XII_LOG_BLOCK("xiiDependencyFile::WriteDependencyFile", sFile);

  xiiFileWriter file;
  if (file.Open(sFile).Failed())
    return XII_FAILURE;

  return WriteDependencyFile(file);
}

xiiResult xiiDependencyFile::ReadDependencyFile(xiiStringView sFile)
{
  XII_LOG_BLOCK("xiiDependencyFile::ReadDependencyFile", sFile);

  xiiFileReader file;
  if (file.Open(sFile).Failed())
    return XII_FAILURE;

  return ReadDependencyFile(file);
}

XII_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_DependencyFile);
