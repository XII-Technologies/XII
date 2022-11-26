#pragma once

template <typename Type>
constexpr XII_ALWAYS_INLINE Type xiiAngle::Pi()
{
  return static_cast<Type>(3.1415926535897932384626433832795);
}

template <typename Type>
constexpr XII_ALWAYS_INLINE Type xiiAngle::DegToRadMultiplier()
{
  return Pi<Type>() / (Type)180;
}

template <typename Type>
constexpr XII_ALWAYS_INLINE Type xiiAngle::RadToDegMultiplier()
{
  return ((Type)180) / Pi<Type>();
}

template <typename Type>
constexpr Type xiiAngle::DegToRad(Type f)
{
  return f * DegToRadMultiplier<Type>();
}

template <typename Type>
constexpr Type xiiAngle::RadToDeg(Type f)
{
  return f * RadToDegMultiplier<Type>();
}

constexpr inline xiiAngle xiiAngle::Degree(float fDegree)
{
  return xiiAngle(DegToRad(fDegree));
}

constexpr XII_ALWAYS_INLINE xiiAngle xiiAngle::Radian(float fRadian)
{
  return xiiAngle(fRadian);
}

constexpr inline float xiiAngle::GetDegree() const
{
  return RadToDeg(m_fRadian);
}

constexpr XII_ALWAYS_INLINE float xiiAngle::GetRadian() const
{
  return m_fRadian;
}

inline xiiAngle xiiAngle::GetNormalizedRange() const
{
  xiiAngle out(m_fRadian);
  out.NormalizeRange();
  return out;
}

inline bool xiiAngle::IsEqualSimple(xiiAngle rhs, xiiAngle epsilon) const
{
  const xiiAngle diff = AngleBetween(*this, rhs);

  return ((diff.m_fRadian >= -epsilon.m_fRadian) && (diff.m_fRadian <= epsilon.m_fRadian));
}

inline bool xiiAngle::IsEqualNormalized(xiiAngle rhs, xiiAngle epsilon) const
{
  // equality between normalized angles
  const xiiAngle aNorm = GetNormalizedRange();
  const xiiAngle bNorm = rhs.GetNormalizedRange();

  return aNorm.IsEqualSimple(bNorm, epsilon);
}

constexpr XII_ALWAYS_INLINE xiiAngle xiiAngle::operator-() const
{
  return xiiAngle(-m_fRadian);
}

XII_ALWAYS_INLINE void xiiAngle::operator+=(xiiAngle r)
{
  m_fRadian += r.m_fRadian;
}

XII_ALWAYS_INLINE void xiiAngle::operator-=(xiiAngle r)
{
  m_fRadian -= r.m_fRadian;
}

constexpr inline xiiAngle xiiAngle::operator+(xiiAngle r) const
{
  return xiiAngle(m_fRadian + r.m_fRadian);
}

constexpr inline xiiAngle xiiAngle::operator-(xiiAngle r) const
{
  return xiiAngle(m_fRadian - r.m_fRadian);
}

constexpr XII_ALWAYS_INLINE bool xiiAngle::operator==(const xiiAngle& r) const
{
  return m_fRadian == r.m_fRadian;
}

constexpr XII_ALWAYS_INLINE bool xiiAngle::operator!=(const xiiAngle& r) const
{
  return m_fRadian != r.m_fRadian;
}

constexpr XII_ALWAYS_INLINE bool xiiAngle::operator<(const xiiAngle& r) const
{
  return m_fRadian < r.m_fRadian;
}

constexpr XII_ALWAYS_INLINE bool xiiAngle::operator>(const xiiAngle& r) const
{
  return m_fRadian > r.m_fRadian;
}

constexpr XII_ALWAYS_INLINE bool xiiAngle::operator<=(const xiiAngle& r) const
{
  return m_fRadian <= r.m_fRadian;
}

constexpr XII_ALWAYS_INLINE bool xiiAngle::operator>=(const xiiAngle& r) const
{
  return m_fRadian >= r.m_fRadian;
}

constexpr inline xiiAngle operator*(xiiAngle a, float f)
{
  return xiiAngle::Radian(a.GetRadian() * f);
}

constexpr inline xiiAngle operator*(float f, xiiAngle a)
{
  return xiiAngle::Radian(a.GetRadian() * f);
}

constexpr inline xiiAngle operator/(xiiAngle a, float f)
{
  return xiiAngle::Radian(a.GetRadian() / f);
}

constexpr inline float operator/(xiiAngle a, xiiAngle b)
{
  return a.GetRadian() / b.GetRadian();
}
