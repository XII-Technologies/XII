#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

// Helper function to shift windows file time into Unix epoch (in microseconds).
xiiInt64 FileTimeToEpoch(FILETIME fileTime)
{
  ULARGE_INTEGER currentTime;
  currentTime.LowPart  = fileTime.dwLowDateTime;
  currentTime.HighPart = fileTime.dwHighDateTime;

  xiiInt64 iTemp = currentTime.QuadPart / 10;
  iTemp -= 11644473600000000LL;
  return iTemp;
}

// Helper function to shift Unix epoch (in microseconds) into windows file time.
FILETIME EpochToFileTime(xiiInt64 iFileTime)
{
  xiiInt64 iTemp = iFileTime + 11644473600000000LL;
  iTemp *= 10;

  FILETIME       fileTime;
  ULARGE_INTEGER currentTime;
  currentTime.QuadPart    = iTemp;
  fileTime.dwLowDateTime  = currentTime.LowPart;
  fileTime.dwHighDateTime = currentTime.HighPart;
  return fileTime;
}

const xiiTimestamp xiiTimestamp::CurrentTimestamp()
{
  FILETIME fileTime;
  GetSystemTimeAsFileTime(&fileTime);
  return xiiTimestamp(FileTimeToEpoch(fileTime), xiiSIUnitOfTime::Microsecond);
}

const xiiTimestamp xiiDateTime::GetTimestamp() const
{
  SYSTEMTIME st;
  FILETIME   fileTime;
  memset(&st, 0, sizeof(SYSTEMTIME));
  st.wYear         = (WORD)m_iYear;
  st.wMonth        = m_uiMonth;
  st.wDay          = m_uiDay;
  st.wDayOfWeek    = m_uiDayOfWeek;
  st.wHour         = m_uiHour;
  st.wMinute       = m_uiMinute;
  st.wSecond       = m_uiSecond;
  st.wMilliseconds = (WORD)(m_uiMicroseconds / 1000);
  BOOL         res = SystemTimeToFileTime(&st, &fileTime);
  xiiTimestamp timestamp;
  if (res != 0)
    timestamp.SetInt64(FileTimeToEpoch(fileTime), xiiSIUnitOfTime::Microsecond);

  return timestamp;
}

bool xiiDateTime::SetTimestamp(xiiTimestamp timestamp)
{
  FILETIME fileTime = EpochToFileTime(timestamp.GetInt64(xiiSIUnitOfTime::Microsecond));

  SYSTEMTIME st;
  BOOL       res = FileTimeToSystemTime(&fileTime, &st);
  if (res == 0)
    return false;

  m_iYear          = (xiiInt16)st.wYear;
  m_uiMonth        = (xiiUInt8)st.wMonth;
  m_uiDay          = (xiiUInt8)st.wDay;
  m_uiDayOfWeek    = (xiiUInt8)st.wDayOfWeek;
  m_uiHour         = (xiiUInt8)st.wHour;
  m_uiMinute       = (xiiUInt8)st.wMinute;
  m_uiSecond       = (xiiUInt8)st.wSecond;
  m_uiMicroseconds = xiiUInt32(st.wMilliseconds * 1000);
  return true;
}
