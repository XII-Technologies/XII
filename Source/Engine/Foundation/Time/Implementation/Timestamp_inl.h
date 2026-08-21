/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Math.h>

inline xiiTimestamp::xiiTimestamp() = default;

inline bool xiiTimestamp::IsValid() const
{
  return m_iTimestamp != XII_INVALID_TIME_STAMP;
}

inline void xiiTimestamp::operator+=(const xiiTime& timeSpan)
{
  XII_ASSERT_DEBUG(IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  m_iTimestamp += (xiiInt64)timeSpan.GetMicroseconds();
}

inline void xiiTimestamp::operator-=(const xiiTime& timeSpan)
{
  XII_ASSERT_DEBUG(IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  m_iTimestamp -= (xiiInt64)timeSpan.GetMicroseconds();
}

inline const xiiTime xiiTimestamp::operator-(const xiiTimestamp& other) const
{
  XII_ASSERT_DEBUG(IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  XII_ASSERT_DEBUG(other.IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  return xiiTime::MakeFromMicroseconds((double)(m_iTimestamp - other.m_iTimestamp));
}

inline const xiiTimestamp xiiTimestamp::operator+(const xiiTime& timeSpan) const
{
  XII_ASSERT_DEBUG(IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  return xiiTimestamp::MakeFromInt(m_iTimestamp + (xiiInt64)timeSpan.GetMicroseconds(), xiiSIUnitOfTime::Microsecond);
}

inline const xiiTimestamp xiiTimestamp::operator-(const xiiTime& timeSpan) const
{
  XII_ASSERT_DEBUG(IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  return xiiTimestamp::MakeFromInt(m_iTimestamp - (xiiInt64)timeSpan.GetMicroseconds(), xiiSIUnitOfTime::Microsecond);
}

inline const xiiTimestamp operator+(const xiiTime& timeSpan, const xiiTimestamp& timestamp)
{
  XII_ASSERT_DEBUG(timestamp.IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  return xiiTimestamp::MakeFromInt(timestamp.GetInt64(xiiSIUnitOfTime::Microsecond) + (xiiInt64)timeSpan.GetMicroseconds(), xiiSIUnitOfTime::Microsecond);
}

inline xiiUInt32 xiiDateTime::GetYear() const
{
  return m_iYear;
}

inline void xiiDateTime::SetYear(xiiInt16 iYear)
{
  m_iYear = iYear;
}

inline xiiUInt8 xiiDateTime::GetMonth() const
{
  return m_uiMonth;
}

inline void xiiDateTime::SetMonth(xiiUInt8 uiMonth)
{
  XII_ASSERT_DEBUG(uiMonth >= 1 && uiMonth <= 12, "Invalid month value");
  m_uiMonth = uiMonth;
}

inline xiiUInt8 xiiDateTime::GetDay() const
{
  return m_uiDay;
}

inline void xiiDateTime::SetDay(xiiUInt8 uiDay)
{
  XII_ASSERT_DEBUG(uiDay >= 1 && uiDay <= 31, "Invalid day value");
  m_uiDay = uiDay;
}

inline xiiUInt8 xiiDateTime::GetDayOfWeek() const
{
  return m_uiDayOfWeek;
}

inline void xiiDateTime::SetDayOfWeek(xiiUInt8 uiDayOfWeek)
{
  XII_ASSERT_DEBUG(uiDayOfWeek <= 6, "Invalid day of week value");
  m_uiDayOfWeek = uiDayOfWeek;
}

inline xiiUInt8 xiiDateTime::GetHour() const
{
  return m_uiHour;
}

inline void xiiDateTime::SetHour(xiiUInt8 uiHour)
{
  XII_ASSERT_DEBUG(uiHour <= 23, "Invalid hour value");
  m_uiHour = uiHour;
}

inline xiiUInt8 xiiDateTime::GetMinute() const
{
  return m_uiMinute;
}

inline void xiiDateTime::SetMinute(xiiUInt8 uiMinute)
{
  XII_ASSERT_DEBUG(uiMinute <= 59, "Invalid minute value");
  m_uiMinute = uiMinute;
}

inline xiiUInt8 xiiDateTime::GetSecond() const
{
  return m_uiSecond;
}

inline void xiiDateTime::SetSecond(xiiUInt8 uiSecond)
{
  XII_ASSERT_DEBUG(uiSecond <= 59, "Invalid second value");
  m_uiSecond = uiSecond;
}

inline xiiUInt32 xiiDateTime::GetMicroseconds() const
{
  return m_uiMicroseconds;
}

inline void xiiDateTime::SetMicroseconds(xiiUInt32 uiMicroSeconds)
{
  XII_ASSERT_DEBUG(uiMicroSeconds <= 999999u, "Invalid micro-second value");
  m_uiMicroseconds = uiMicroSeconds;
}
