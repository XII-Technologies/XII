/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <sys/time.h>
#include <time.h>

const xiiTimestamp xiiTimestamp::CurrentTimestamp()
{
  timeval currentTime;
  gettimeofday(&currentTime, nullptr);

  return xiiTimestamp::MakeFromInt(currentTime.tv_sec * 1000000LL + currentTime.tv_usec, xiiSIUnitOfTime::Microsecond);
}

bool operator!=(const tm& lhs, const tm& rhs)
{
  if (lhs.tm_isdst == rhs.tm_isdst)
  {
    return !((lhs.tm_sec == rhs.tm_sec) && (lhs.tm_min == rhs.tm_min) && (lhs.tm_hour == rhs.tm_hour) && (lhs.tm_mday == rhs.tm_mday) &&
             (lhs.tm_mon == rhs.tm_mon) && (lhs.tm_year == rhs.tm_year) && (lhs.tm_isdst == rhs.tm_isdst));
  }
  else
  {
    /// \todo check whether the times are equal if one is in dst and the other not.
    /// mktime totally ignores your settings and overwrites them, there is no easy way
    /// to check whether the times are equal when dst is involved.
    /// mktime's dst *fix-up* will change hour, dst, day, month and year in the worst case.
    return false;
  }
}

const xiiTimestamp xiiDateTime::GetTimestamp() const
{
  // Validate fields like macOS and Windows
  if (m_iYear < 1 || m_iYear > 9999)
    return xiiTimestamp::MakeInvalid();

  if (m_uiMonth < 1 || m_uiMonth > 12)
    return xiiTimestamp::MakeInvalid();

  if (m_uiDay < 1 || m_uiDay > 31)
    return xiiTimestamp::MakeInvalid();

  tm timeinfo = {0};

  timeinfo.tm_sec  = m_uiSecond;
  timeinfo.tm_min  = m_uiMinute;
  timeinfo.tm_hour = m_uiHour;
  timeinfo.tm_mday = m_uiDay;
  timeinfo.tm_mon  = m_uiMonth - 1;
  timeinfo.tm_year = m_iYear - 1900;

  time_t ts = timegm(&timeinfo);
  if (ts == (time_t)-1)
    return xiiTimestamp::MakeInvalid();

  return xiiTimestamp::MakeFromInt(ts, xiiSIUnitOfTime::Second);
}

xiiResult xiiDateTime::SetFromTimestamp(xiiTimestamp timestamp)
{
  tm     timeinfo = {0};
  time_t iTime    = (time_t)timestamp.GetInt64(xiiSIUnitOfTime::Second);
  if (gmtime_r(&iTime, &timeinfo) == nullptr)
    return XII_FAILURE;

  m_iYear          = timeinfo.tm_year + 1900;
  m_uiMonth        = timeinfo.tm_mon + 1;
  m_uiDay          = timeinfo.tm_mday;
  m_uiDayOfWeek    = timeinfo.tm_wday;
  m_uiHour         = timeinfo.tm_hour;
  m_uiMinute       = timeinfo.tm_min;
  m_uiSecond       = timeinfo.tm_sec;
  m_uiMicroseconds = 0;

  return XII_SUCCESS;
}
