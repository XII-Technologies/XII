/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

XII_ALWAYS_INLINE xiiSimdVec4f::xiiSimdVec4f(xiiInternal::QuadFloat v)
{
  m_v = v;
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::MakeZero()
{
  return xiiSimdVec4f(xiiSimdFloat::MakeZero());
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::MakeNaN()
{
  return xiiSimdVec4f(xiiSimdFloat::MakeNaN());
}

template <xiiInt32 N, xiiMathFloatBits::Enum acc>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::GetLength() const
{
  const xiiSimdFloat squaredLen = GetLengthSquared<N>();
  return squaredLen.GetSqrt<acc>();
}

template <xiiInt32 N, xiiMathFloatBits::Enum acc>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::GetInvLength() const
{
  const xiiSimdFloat squaredLen = GetLengthSquared<N>();
  return squaredLen.GetInvSqrt<acc>();
}

template <xiiInt32 N>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::GetLengthSquared() const
{
  return Dot<N>(*this);
}

template <xiiInt32 N, xiiMathFloatBits::Enum acc>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::GetLengthAndNormalize()
{
  const xiiSimdFloat squaredLen    = GetLengthSquared<N>();
  const xiiSimdFloat reciprocalLen = squaredLen.GetInvSqrt<acc>();
  *this                            = (*this) * reciprocalLen;
  return squaredLen * reciprocalLen;
}

template <xiiInt32 N, xiiMathFloatBits::Enum acc>
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::GetNormalized() const
{
  return (*this) * GetInvLength<N, acc>();
}

template <xiiInt32 N, xiiMathFloatBits::Enum acc>
XII_ALWAYS_INLINE void xiiSimdVec4f::Normalize()
{
  *this = GetNormalized<N, acc>();
}

template <xiiInt32 N>
XII_ALWAYS_INLINE bool xiiSimdVec4f::IsNormalized(const xiiSimdFloat& fEpsilon) const
{
  const xiiSimdFloat sqLength = GetLengthSquared<N>();
  return sqLength.IsEqual(1.0f, fEpsilon);
}

inline xiiSimdFloat xiiSimdVec4f::GetComponent(xiiInt32 i) const
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

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::Fraction() const
{
  return *this - Trunc();
}

// static
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::Lerp(const xiiSimdVec4f& a, const xiiSimdVec4f& b, const xiiSimdVec4f& t)
{
  return a + t.CompMul(b - a);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4f::IsEqual(const xiiSimdVec4f& rhs, const xiiSimdFloat& fEpsilon) const
{
  xiiSimdVec4f minusEps = rhs - xiiSimdVec4f(fEpsilon);
  xiiSimdVec4f plusEps  = rhs + xiiSimdVec4f(fEpsilon);
  return (*this >= minusEps) && (*this <= plusEps);
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::HorizontalSum<1>() const
{
  return GetComponent<0>();
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::HorizontalMin<1>() const
{
  return GetComponent<0>();
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::HorizontalMax<1>() const
{
  return GetComponent<0>();
}
