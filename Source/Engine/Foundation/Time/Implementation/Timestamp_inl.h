#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Math.h>

static const xiiInt64 XII_INVALID_TIME_STAMP = 0x7FFFFFFFFFFFFFFFLL;

inline xiiTimestamp::xiiTimestamp()
{
  Invalidate();
}

inline xiiTimestamp::xiiTimestamp(xiiInt64 iTimeValue, xiiSIUnitOfTime::Enum unitOfTime)
{
  SetInt64(iTimeValue, unitOfTime);
}

inline void xiiTimestamp::Invalidate()
{
  m_iTimestamp = XII_INVALID_TIME_STAMP;
}

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
  return xiiTime::Microseconds((double)(m_iTimestamp - other.m_iTimestamp));
}

inline const xiiTimestamp xiiTimestamp::operator+(const xiiTime& timeSpan) const
{
  XII_ASSERT_DEBUG(IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  return xiiTimestamp(m_iTimestamp + (xiiInt64)timeSpan.GetMicroseconds(), xiiSIUnitOfTime::Microsecond);
}

inline const xiiTimestamp xiiTimestamp::operator-(const xiiTime& timeSpan) const
{
  XII_ASSERT_DEBUG(IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  return xiiTimestamp(m_iTimestamp - (xiiInt64)timeSpan.GetMicroseconds(), xiiSIUnitOfTime::Microsecond);
}

inline const xiiTimestamp operator+(const xiiTime& timeSpan, const xiiTimestamp& timestamp)
{
  XII_ASSERT_DEBUG(timestamp.IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  return xiiTimestamp(timestamp.GetInt64(xiiSIUnitOfTime::Microsecond) + (xiiInt64)timeSpan.GetMicroseconds(), xiiSIUnitOfTime::Microsecond);
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
  m_uiMonth = xiiMath::Clamp<xiiUInt8>(uiMonth, 1, 12);
}

inline xiiUInt8 xiiDateTime::GetDay() const
{
  return m_uiDay;
}

inline void xiiDateTime::SetDay(xiiUInt8 uiDay)
{
  m_uiDay = xiiMath::Clamp<xiiUInt8>(uiDay, 1u, 31u);
}

inline xiiUInt8 xiiDateTime::GetDayOfWeek() const
{
  return m_uiDayOfWeek;
}

inline void xiiDateTime::SetDayOfWeek(xiiUInt8 uiDayOfWeek)
{
  m_uiDayOfWeek = xiiMath::Clamp<xiiUInt8>(uiDayOfWeek, 0u, 6u);
}

inline xiiUInt8 xiiDateTime::GetHour() const
{
  return m_uiHour;
}

inline void xiiDateTime::SetHour(xiiUInt8 uiHour)
{
  m_uiHour = xiiMath::Clamp<xiiUInt8>(uiHour, 0u, 23u);
}

inline xiiUInt8 xiiDateTime::GetMinute() const
{
  return m_uiMinute;
}

inline void xiiDateTime::SetMinute(xiiUInt8 uiMinute)
{
  m_uiMinute = xiiMath::Clamp<xiiUInt8>(uiMinute, 0u, 59u);
}

inline xiiUInt8 xiiDateTime::GetSecond() const
{
  return m_uiSecond;
}

inline void xiiDateTime::SetSecond(xiiUInt8 uiSecond)
{
  m_uiSecond = xiiMath::Clamp<xiiUInt8>(uiSecond, 0u, 59u);
}

inline xiiUInt32 xiiDateTime::GetMicroseconds() const
{
  return m_uiMicroseconds;
}

inline void xiiDateTime::SetMicroseconds(xiiUInt32 uiMicroSeconds)
{
  m_uiMicroseconds = xiiMath::Clamp<xiiUInt32>(uiMicroSeconds, 0u, 999999u);
}
