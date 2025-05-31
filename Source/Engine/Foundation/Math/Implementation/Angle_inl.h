#pragma once

template <typename Type>
inline constexpr Type xiiAngleTemplate<Type>::Pi()
{
  return static_cast<Type>(3.1415926535897932384626433832795);
}

template <typename Type>
constexpr XII_ALWAYS_INLINE Type xiiAngleTemplate<Type>::DegToRadMultiplier()
{
  return Pi() / (Type)180;
}

template <typename Type>
constexpr XII_ALWAYS_INLINE Type xiiAngleTemplate<Type>::RadToDegMultiplier()
{
  return ((Type)180) / Pi();
}

template <typename Type>
constexpr Type xiiAngleTemplate<Type>::DegToRad(Type f)
{
  return f * DegToRadMultiplier();
}

template <typename Type>
constexpr Type xiiAngleTemplate<Type>::RadToDeg(Type f)
{
  return f * RadToDegMultiplier();
}

template <typename Type>
constexpr inline xiiAngleTemplate<Type> xiiAngleTemplate<Type>::MakeFromDegree(Type fDegree)
{
  return xiiAngleTemplate<Type>(DegToRad(fDegree));
}

template <typename Type>
constexpr XII_ALWAYS_INLINE xiiAngleTemplate<Type> xiiAngleTemplate<Type>::MakeFromRadian(Type fRadian)
{
  return xiiAngleTemplate<Type>(fRadian);
}

template <typename Type>
constexpr inline Type xiiAngleTemplate<Type>::GetDegree() const
{
  return RadToDeg(m_fRadian);
}

template <typename Type>
constexpr XII_ALWAYS_INLINE Type xiiAngleTemplate<Type>::GetRadian() const
{
  return m_fRadian;
}

template <typename Type>
inline void xiiAngleTemplate<Type>::NormalizeRange()
{
  constexpr Type fTwoPi    = static_cast<Type>(2) * xiiAngleTemplate<Type>::Pi();
  constexpr Type fTwoPiTen = static_cast<Type>(10) * xiiAngleTemplate<Type>::Pi();

  if (m_fRadian > fTwoPiTen || m_fRadian < -fTwoPiTen)
  {
    if constexpr (std::is_same_v<Type, float>)
    {
      m_fRadian = fmodf(m_fRadian, fTwoPi);
    }
    else
    {
      m_fRadian = fmod(m_fRadian, fTwoPi);
    }
  }

  while (m_fRadian >= fTwoPi)
  {
    m_fRadian -= fTwoPi;
  }

  while (m_fRadian < 0.0f)
  {
    m_fRadian += fTwoPi;
  }
}

template <typename Type>
inline xiiAngleTemplate<Type> xiiAngleTemplate<Type>::GetNormalizedRange() const
{
  xiiAngleTemplate<Type> out(m_fRadian);
  out.NormalizeRange();
  return out;
}

template <typename Type>
inline bool xiiAngleTemplate<Type>::IsEqualSimple(xiiAngleTemplate<Type> rhs, xiiAngleTemplate<Type> epsilon) const
{
  const xiiAngleTemplate<Type> diff = AngleBetween(*this, rhs);

  return ((diff.m_fRadian >= -epsilon.m_fRadian) && (diff.m_fRadian <= epsilon.m_fRadian));
}

template <typename Type>
inline bool xiiAngleTemplate<Type>::IsEqualNormalized(xiiAngleTemplate<Type> rhs, xiiAngleTemplate<Type> epsilon) const
{
  // Equality between normalized angles
  const xiiAngleTemplate<Type> aNorm = GetNormalizedRange();
  const xiiAngleTemplate<Type> bNorm = rhs.GetNormalizedRange();

  return aNorm.IsEqualSimple(bNorm, epsilon);
}

template <typename Type>
constexpr XII_ALWAYS_INLINE xiiAngleTemplate<Type> xiiAngleTemplate<Type>::operator-() const
{
  return xiiAngleTemplate<Type>(-m_fRadian);
}

template <typename Type>
XII_ALWAYS_INLINE void xiiAngleTemplate<Type>::operator+=(xiiAngleTemplate<Type> r)
{
  m_fRadian += r.m_fRadian;
}

template <typename Type>
XII_ALWAYS_INLINE void xiiAngleTemplate<Type>::operator-=(xiiAngleTemplate<Type> r)
{
  m_fRadian -= r.m_fRadian;
}

template <typename Type>
constexpr inline xiiAngleTemplate<Type> xiiAngleTemplate<Type>::operator+(xiiAngleTemplate<Type> r) const
{
  return xiiAngleTemplate<Type>(m_fRadian + r.m_fRadian);
}

template <typename Type>
constexpr inline xiiAngleTemplate<Type> xiiAngleTemplate<Type>::operator-(xiiAngleTemplate<Type> r) const
{
  return xiiAngleTemplate<Type>(m_fRadian - r.m_fRadian);
}

template <typename Type>
constexpr XII_ALWAYS_INLINE bool xiiAngleTemplate<Type>::operator==(const xiiAngleTemplate<Type>& r) const
{
  return m_fRadian == r.m_fRadian;
}

template <typename Type>
constexpr XII_ALWAYS_INLINE std::partial_ordering xiiAngleTemplate<Type>::operator<=>(const xiiAngleTemplate<Type>& r) const
{
  return m_fRadian <=> r.m_fRadian;
}

template <typename Type>
constexpr inline xiiAngleTemplate<Type> operator*(const xiiAngleTemplate<Type>& a, Type f)
{
  return xiiAngleTemplate<Type>::MakeFromRadian(a.GetRadian() * f);
}

template <typename Type>
constexpr inline xiiAngleTemplate<Type> operator*(Type f, const xiiAngleTemplate<Type>& a)
{
  return xiiAngleTemplate<Type>::MakeFromRadian(a.GetRadian() * f);
}

template <typename Type>
constexpr inline xiiAngleTemplate<Type> operator/(const xiiAngleTemplate<Type>& a, Type f)
{
  return xiiAngleTemplate<Type>::MakeFromRadian(a.GetRadian() / f);
}

template <typename Type>
constexpr inline Type operator/(const xiiAngleTemplate<Type>& a, const xiiAngleTemplate<Type>& b)
{
  return a.GetRadian() / b.GetRadian();
}
