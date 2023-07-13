#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Basics/Platform/OSX/ScopedCFRef.h>

#include <CoreFoundation/CFCalendar.h>
#include <CoreFoundation/CoreFoundation.h>

const xiiTimestamp xiiTimestamp::CurrentTimestamp()
{
  timeval currentTime;
  gettimeofday(&currentTime, nullptr);

  return xiiTimestamp(currentTime.tv_sec * 1000000LL + currentTime.tv_usec, xiiSIUnitOfTime::Microsecond);
}

const xiiTimestamp xiiDateTime::GetTimestamp() const
{
  xiiScopedCFRef<CFTimeZoneRef> timxiione(CFTimeZoneCreateWithTimeIntervalFromGMT(kCFAllocatorDefault, 0));
  xiiScopedCFRef<CFCalendarRef> calendar(CFCalendarCreateWithIdentifier(kCFAllocatorSystemDefault, kCFGregorianCalendar));
  CFCalendarSetTimeZone(calendar, timxiione);

  xiiInt32 year = m_iYear, month = m_uiMonth, day = m_uiDay, hour = m_uiHour, minute = m_uiMinute, second = m_uiSecond;

  // Validate the year against the valid range of the calendar
  {
    auto yearMin = CFCalendarGetMinimumRangeOfUnit(calendar, kCFCalendarUnitYear), yearMax = CFCalendarGetMaximumRangeOfUnit(calendar, kCFCalendarUnitYear);

    if (year < yearMin.location || year > yearMax.length)
    {
      return xiiTimestamp();
    }
  }

  // Validate the month against the valid range of the calendar
  {
    auto monthMin = CFCalendarGetMinimumRangeOfUnit(calendar, kCFCalendarUnitMonth), monthMax = CFCalendarGetMaximumRangeOfUnit(calendar, kCFCalendarUnitMonth);

    if (month < monthMin.location || month > monthMax.length)
    {
      return xiiTimestamp();
    }
  }

  // Validate the day against the valid range of the calendar
  {
    auto dayMin = CFCalendarGetMinimumRangeOfUnit(calendar, kCFCalendarUnitDay), dayMax = CFCalendarGetMaximumRangeOfUnit(calendar, kCFCalendarUnitDay);

    if (day < dayMin.location || day > dayMax.length)
    {
      return xiiTimestamp();
    }
  }

  CFAbsoluteTime absTime;
  if (CFCalendarComposeAbsoluteTime(calendar, &absTime, "yMdHms", year, month, day, hour, minute, second) == FALSE)
  {
    return xiiTimestamp();
  }

  return xiiTimestamp(static_cast<xiiInt64>((absTime + kCFAbsoluteTimeIntervalSince1970) * 1000000.0), xiiSIUnitOfTime::Microsecond);
}

bool xiiDateTime::SetTimestamp(xiiTimestamp timestamp)
{
  // Round the microseconds to the full second so that we can reconstruct the right date / time afterwards
  xiiInt64 us           = timestamp.GetInt64(xiiSIUnitOfTime::Microsecond);
  xiiInt64 microseconds = us % (1000 * 1000);

  CFAbsoluteTime at = (static_cast<CFAbsoluteTime>((us - microseconds) / 1000000.0)) - kCFAbsoluteTimeIntervalSince1970;

  xiiScopedCFRef<CFTimeZoneRef> timxiione(CFTimeZoneCreateWithTimeIntervalFromGMT(kCFAllocatorDefault, 0));
  xiiScopedCFRef<CFCalendarRef> calendar(CFCalendarCreateWithIdentifier(kCFAllocatorSystemDefault, kCFGregorianCalendar));
  CFCalendarSetTimeZone(calendar, timxiione);

  xiiInt32 year, month, day, dayOfWeek, hour, minute, second;

  if (CFCalendarDecomposeAbsoluteTime(calendar, at, "yMdHmsE", &year, &month, &day, &hour, &minute, &second, &dayOfWeek) == FALSE)
  {
    return false;
  }

  m_iYear          = (xiiInt16)year;
  m_uiMonth        = (xiiUInt8)month;
  m_uiDay          = (xiiUInt8)day;
  m_uiDayOfWeek    = (xiiUInt8)(dayOfWeek - 1);
  m_uiHour         = (xiiUInt8)hour;
  m_uiMinute       = (xiiUInt8)minute;
  m_uiSecond       = (xiiUInt8)second;
  m_uiMicroseconds = (xiiUInt32)microseconds;
  return true;
}
