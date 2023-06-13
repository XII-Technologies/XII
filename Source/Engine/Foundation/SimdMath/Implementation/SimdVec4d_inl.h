#pragma once

XII_ALWAYS_INLINE xiiSimdVec4d::xiiSimdVec4d(xiiInternal::QuadDouble v)
{
  m_v = v;
}

template <xiiInt32 N, xiiMathDoubleBits::Enum acc>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::GetLength() const
{
  const xiiSimdDouble squaredLength = GetLengthSquared<N>();
  return squaredLength.GetSqrt<acc>();
}

template <xiiInt32 N, xiiMathDoubleBits::Enum acc>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::GetInvLength() const
{
  const xiiSimdDouble squaredLength = GetLengthSquared<N>();
  return squaredLength.GetInvSqrt<acc>();
}

template <xiiInt32 N>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::GetLengthSquared() const
{
  return Dot<N>(*this);
}

template <xiiInt32 N, xiiMathDoubleBits::Enum acc>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::GetLengthAndNormalize()
{
  const xiiSimdDouble squaredLen    = GetLengthSquared<N>();
  const xiiSimdDouble reciprocalLen = squaredLen.GetInvSqrt<acc>();
  *this                             = (*this) * reciprocalLen;
  return squaredLen * reciprocalLen;
}

template <xiiInt32 N, xiiMathDoubleBits::Enum acc>
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::GetNormalized() const
{
  return (*this) * GetInvLength<N, acc>();
}

template <xiiInt32 N, xiiMathDoubleBits::Enum acc>
XII_ALWAYS_INLINE void xiiSimdVec4d::Normalize()
{
  *this = GetNormalized<N, acc>();
}

template <xiiInt32 N>
XII_ALWAYS_INLINE bool xiiSimdVec4d::IsNormalized(const xiiSimdDouble& fEpsilon) const
{
  const xiiSimdDouble sqLength = GetLengthSquared<N>();
  return sqLength.IsEqual(1.0f, fEpsilon);
}

inline xiiSimdDouble xiiSimdVec4d::GetComponent(xiiInt32 i) const
{
  switch (i)
  {
    case 0:
      return GetComponent<0>();

    case 1:
      return GetComponent<1>();

    case 2:
      return GetComponent<2>();

    default:
      return GetComponent<3>();
  }
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::Fraction() const
{
  return *this - Trunc();
}

// static
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::Lerp(const xiiSimdVec4d& a, const xiiSimdVec4d& b, const xiiSimdVec4d& t)
{
  return a + t.CompMul(b - a);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4d::IsEqual(const xiiSimdVec4d& rhs, const xiiSimdDouble& fEpsilon) const
{
  xiiSimdVec4d minusEps = rhs - xiiSimdVec4d(fEpsilon);
  xiiSimdVec4d plusEps  = rhs + xiiSimdVec4d(fEpsilon);
  return (*this >= minusEps) && (*this <= plusEps);
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::HorizontalSum<1>() const
{
  return GetComponent<0>();
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::HorizontalMin<1>() const
{
  return GetComponent<0>();
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::HorizontalMax<1>() const
{
  return GetComponent<0>();
}
