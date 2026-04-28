/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

constexpr XII_ALWAYS_INLINE xiiTime::xiiTime(double fTime) :
  m_fTime(fTime)
{
}

constexpr XII_ALWAYS_INLINE float xiiTime::AsFloatInSeconds() const
{
  return static_cast<float>(m_fTime);
}

constexpr XII_ALWAYS_INLINE double xiiTime::GetNanoseconds() const
{
  return m_fTime * 1000000000.0;
}

constexpr XII_ALWAYS_INLINE double xiiTime::GetMicroseconds() const
{
  return m_fTime * 1000000.0;
}

constexpr XII_ALWAYS_INLINE double xiiTime::GetMilliseconds() const
{
  return m_fTime * 1000.0;
}

constexpr XII_ALWAYS_INLINE double xiiTime::GetSeconds() const
{
  return m_fTime;
}

constexpr XII_ALWAYS_INLINE double xiiTime::GetMinutes() const
{
  return m_fTime / 60.0;
}

constexpr XII_ALWAYS_INLINE double xiiTime::GetHours() const
{
  return m_fTime / (60.0 * 60.0);
}

constexpr XII_ALWAYS_INLINE void xiiTime::operator-=(const xiiTime& other)
{
  m_fTime -= other.m_fTime;
}

constexpr XII_ALWAYS_INLINE void xiiTime::operator+=(const xiiTime& other)
{
  m_fTime += other.m_fTime;
}

constexpr XII_ALWAYS_INLINE void xiiTime::operator*=(double fFactor)
{
  m_fTime *= fFactor;
}

constexpr XII_ALWAYS_INLINE void xiiTime::operator/=(double fFactor)
{
  m_fTime /= fFactor;
}

constexpr XII_ALWAYS_INLINE xiiTime xiiTime::operator-() const
{
  return xiiTime(-m_fTime);
}

constexpr XII_ALWAYS_INLINE xiiTime xiiTime::operator-(const xiiTime& other) const
{
  return xiiTime(m_fTime - other.m_fTime);
}

constexpr XII_ALWAYS_INLINE xiiTime xiiTime::operator+(const xiiTime& other) const
{
  return xiiTime(m_fTime + other.m_fTime);
}

constexpr XII_ALWAYS_INLINE xiiTime operator*(const xiiTime& t, double f)
{
  return xiiTime::MakeFromSeconds(t.GetSeconds() * f);
}

constexpr XII_ALWAYS_INLINE xiiTime operator*(double f, const xiiTime& t)
{
  return xiiTime::MakeFromSeconds(t.GetSeconds() * f);
}

constexpr XII_ALWAYS_INLINE xiiTime operator*(const xiiTime& f, const xiiTime& t)
{
  return xiiTime::MakeFromSeconds(t.GetSeconds() * f.GetSeconds());
}

constexpr XII_ALWAYS_INLINE xiiTime operator/(const xiiTime& t, double f)
{
  return xiiTime::MakeFromSeconds(t.GetSeconds() / f);
}

constexpr XII_ALWAYS_INLINE xiiTime operator/(double f, const xiiTime& t)
{
  return xiiTime::MakeFromSeconds(f / t.GetSeconds());
}

constexpr XII_ALWAYS_INLINE xiiTime operator/(const xiiTime& f, const xiiTime& t)
{
  return xiiTime::MakeFromSeconds(f.GetSeconds() / t.GetSeconds());
}
