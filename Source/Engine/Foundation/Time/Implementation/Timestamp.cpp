#include <Foundation/FoundationPCH.h>

#include <Foundation/Time/Timestamp.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiTimestamp, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("time", m_iTimestamp),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiInt64 xiiTimestamp::GetInt64(xiiSIUnitOfTime::Enum unitOfTime) const
{
  XII_ASSERT_DEV(IsValid(), "Can't retrieve timestamp of invalid values!");
  XII_ASSERT_DEV(unitOfTime >= xiiSIUnitOfTime::Nanosecond && unitOfTime <= xiiSIUnitOfTime::Second, "Invalid xiiSIUnitOfTime value ({0})", unitOfTime);

  switch (unitOfTime)
  {
    case xiiSIUnitOfTime::Nanosecond:
      return m_iTimestamp * 1000LL;
    case xiiSIUnitOfTime::Microsecond:
      return m_iTimestamp;
    case xiiSIUnitOfTime::Millisecond:
      return m_iTimestamp / 1000LL;
    case xiiSIUnitOfTime::Second:
      return m_iTimestamp / 1000000LL;
  }
  return XII_INVALID_TIME_STAMP;
}

void xiiTimestamp::SetInt64(xiiInt64 iTimeValue, xiiSIUnitOfTime::Enum unitOfTime)
{
  XII_ASSERT_DEV(unitOfTime >= xiiSIUnitOfTime::Nanosecond && unitOfTime <= xiiSIUnitOfTime::Second, "Invalid xiiSIUnitOfTime value ({0})", unitOfTime);

  switch (unitOfTime)
  {
    case xiiSIUnitOfTime::Nanosecond:
      m_iTimestamp = iTimeValue / 1000LL;
      break;
    case xiiSIUnitOfTime::Microsecond:
      m_iTimestamp = iTimeValue;
      break;
    case xiiSIUnitOfTime::Millisecond:
      m_iTimestamp = iTimeValue * 1000LL;
      break;
    case xiiSIUnitOfTime::Second:
      m_iTimestamp = iTimeValue * 1000000LL;
      break;
  }
}

bool xiiTimestamp::Compare(const xiiTimestamp& rhs, CompareMode::Enum mode) const
{
  switch (mode)
  {
    case CompareMode::FileTimeEqual:
      // Resolution of seconds until all platforms are tuned to milliseconds.
      return (m_iTimestamp / 1000000LL) == (rhs.m_iTimestamp / 1000000LL);

    case CompareMode::Identical:
      return m_iTimestamp == rhs.m_iTimestamp;

    case CompareMode::Newer:
      // Resolution of seconds until all platforms are tuned to milliseconds.
      return (m_iTimestamp / 1000000LL) > (rhs.m_iTimestamp / 1000000LL);
  }

  XII_ASSERT_NOT_IMPLEMENTED;
  return false;
}

xiiDateTime::xiiDateTime() :
  m_uiMicroseconds(0), m_iYear(0), m_uiMonth(0), m_uiDay(0), m_uiDayOfWeek(0), m_uiHour(0), m_uiMinute(0), m_uiSecond(0)
{
}

xiiDateTime::xiiDateTime(xiiTimestamp timestamp) :
  xiiDateTime()
{
  SetTimestamp(timestamp);
}

xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const xiiDateTime& arg)
{
  xiiStringUtils::snprintf(tmp, uiLength, "%04u-%02u-%02u_%02u-%02u-%02u-%03u", arg.GetYear(), arg.GetMonth(), arg.GetDay(), arg.GetHour(),
                           arg.GetMinute(), arg.GetSecond(), arg.GetMicroseconds() / 1000);

  return tmp;
}

namespace
{
  // This implementation chooses a 3-character-long short name for each of the twelve months
  // for consistency reasons. Mind, that other, potentially more widely-spread stylist
  // alternatives may exist.
  const char* GetMonthShortName(const xiiDateTime& dateTime)
  {
    switch (dateTime.GetMonth())
    {
      case 1:
        return "Jan";
      case 2:
        return "Feb";
      case 3:
        return "Mar";
      case 4:
        return "Apr";
      case 5:
        return "May";
      case 6:
        return "Jun";
      case 7:
        return "Jul";
      case 8:
        return "Aug";
      case 9:
        return "Sep";
      case 10:
        return "Oct";
      case 11:
        return "Nov";
      case 12:
        return "Dec";
      default:
        XII_ASSERT_DEV(false, "Unknown month.");
        return "Unknown Month";
    }
  }

  // This implementation chooses a 3-character-long short name for each of the seven days
  // of the week for consistency reasons. Mind, that other, potentially more widely-spread
  // stylistic alternatives may exist.
  const char* GetDayOfWeekShortName(const xiiDateTime& dateTime)
  {
    switch (dateTime.GetDayOfWeek())
    {
      case 0:
        return "Sun";
      case 1:
        return "Mon";
      case 2:
        return "Tue";
      case 3:
        return "Wed";
      case 4:
        return "Thu";
      case 5:
        return "Fri";
      case 6:
        return "Sat";
      default:
        XII_ASSERT_DEV(false, "Unknown day of week.");
        return "Unknown Day of Week";
    }
  }
} // namespace

xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const xiiArgDateTime& arg)
{
  const xiiDateTime& dateTime = arg.m_Value;

  xiiUInt32 offset = 0;

  if ((arg.m_uiFormattingFlags & xiiArgDateTime::ShowDate) == xiiArgDateTime::ShowDate)
  {
    if ((arg.m_uiFormattingFlags & xiiArgDateTime::TextualDate) == xiiArgDateTime::TextualDate)
    {
      offset += xiiStringUtils::snprintf(
        tmp + offset, uiLength - offset, "%04u %s %02u", dateTime.GetYear(), ::GetMonthShortName(dateTime), dateTime.GetDay());
    }
    else
    {
      offset +=
        xiiStringUtils::snprintf(tmp + offset, uiLength - offset, "%04u-%02u-%02u", dateTime.GetYear(), dateTime.GetMonth(), dateTime.GetDay());
    }
  }

  if ((arg.m_uiFormattingFlags & xiiArgDateTime::ShowWeekday) == xiiArgDateTime::ShowWeekday)
  {
    // add a space
    if (offset != 0)
    {
      tmp[offset] = ' ';
      ++offset;
      tmp[offset] = '\0';
    }

    offset += xiiStringUtils::snprintf(tmp + offset, uiLength - offset, "(%s)", ::GetDayOfWeekShortName(dateTime));
  }

  if ((arg.m_uiFormattingFlags & xiiArgDateTime::ShowTime) == xiiArgDateTime::ShowTime)
  {
    // add a space
    if (offset != 0)
    {
      tmp[offset]     = ' ';
      tmp[offset + 1] = '-';
      tmp[offset + 2] = ' ';
      tmp[offset + 3] = '\0';
      offset += 3;
    }

    offset += xiiStringUtils::snprintf(tmp + offset, uiLength - offset, "%02u:%02u", dateTime.GetHour(), dateTime.GetMinute());

    if ((arg.m_uiFormattingFlags & xiiArgDateTime::ShowSeconds) == xiiArgDateTime::ShowSeconds)
    {
      offset += xiiStringUtils::snprintf(tmp + offset, uiLength - offset, ":%02u", dateTime.GetSecond());
    }

    if ((arg.m_uiFormattingFlags & xiiArgDateTime::ShowMilliseconds) == xiiArgDateTime::ShowMilliseconds)
    {
      offset += xiiStringUtils::snprintf(tmp + offset, uiLength - offset, ".%03u", dateTime.GetMicroseconds() / 1000);
    }

    if ((arg.m_uiFormattingFlags & xiiArgDateTime::ShowTimeZone) == xiiArgDateTime::ShowTimeZone)
    {
      offset += xiiStringUtils::snprintf(tmp + offset, uiLength - offset, " (UTC)");
    }
  }

  return tmp;
}

// Include inline file
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Time/Implementation/Win/Timestamp_win.h>
#elif XII_ENABLED(XII_PLATFORM_OSX)
#  include <Foundation/Time/Implementation/OSX/Timestamp_osx.h>
#elif XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <Foundation/Time/Implementation/Android/Timestamp_android.h>
#elif XII_ENABLED(XII_PLATFORM_LINUX)
#  include <Foundation/Time/Implementation/Posix/Timestamp_posix.h>
#else
#  error "Time functions are not implemented on current platform"
#endif

XII_STATICLINK_FILE(Foundation, Foundation_Time_Implementation_Timestamp);
