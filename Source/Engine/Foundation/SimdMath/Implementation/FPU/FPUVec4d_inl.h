#pragma once

XII_ALWAYS_INLINE xiiSimdVec4d::xiiSimdVec4d() {}

XII_ALWAYS_INLINE xiiSimdVec4d::xiiSimdVec4d(double xyzw)
{
  m_v.Set(xyzw);
}

XII_ALWAYS_INLINE xiiSimdVec4d::xiiSimdVec4d(const xiiSimdDouble& xyzw)
{
  m_v = xyzw.m_v;
}

XII_ALWAYS_INLINE xiiSimdVec4d::xiiSimdVec4d(double x, double y, double z, double w)
{
  m_v.Set(x, y, z, w);
}

XII_ALWAYS_INLINE void xiiSimdVec4d::Set(double xyzw)
{
  m_v.Set(xyzw);
}

XII_ALWAYS_INLINE void xiiSimdVec4d::Set(double x, double y, double z, double w)
{
  m_v.Set(x, y, z, w);
}

XII_ALWAYS_INLINE void xiiSimdVec4d::SetX(const xiiSimdDouble& f)
{
  m_v.x = f.m_v.x;
}

XII_ALWAYS_INLINE void xiiSimdVec4d::SetY(const xiiSimdDouble& f)
{
  m_v.y = f.m_v.x;
}

XII_ALWAYS_INLINE void xiiSimdVec4d::SetZ(const xiiSimdDouble& f)
{
  m_v.z = f.m_v.x;
}

XII_ALWAYS_INLINE void xiiSimdVec4d::SetW(const xiiSimdDouble& f)
{
  m_v.w = f.m_v.x;
}

XII_ALWAYS_INLINE void xiiSimdVec4d::SetZero()
{
  m_v.SetZero();
}

template <xiiInt32 N>
XII_ALWAYS_INLINE void xiiSimdVec4d::Load(const double* pValues)
{
  m_v.SetZero();
  for (xiiInt32 i = 0; i < N; ++i)
  {
    (&m_v.x)[i] = pValues[i];
  }
}

template <xiiInt32 N>
XII_ALWAYS_INLINE void xiiSimdVec4d::Store(double* pValues) const
{
  for (xiiInt32 i = 0; i < N; ++i)
  {
    pValues[i] = (&m_v.x)[i];
  }
}

template <xiiMathDoubleBits::Enum acc>
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::GetReciprocal() const
{
  return xiiVec4d(1.0).CompDiv(m_v);
}

template <xiiMathDoubleBits::Enum acc>
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::GetSqrt() const
{
  xiiSimdVec4d result;
  result.m_v.x = xiiMath::Sqrt(m_v.x);
  result.m_v.y = xiiMath::Sqrt(m_v.y);
  result.m_v.z = xiiMath::Sqrt(m_v.z);
  result.m_v.w = xiiMath::Sqrt(m_v.w);

  return result;
}

template <xiiMathDoubleBits::Enum acc>
xiiSimdVec4d xiiSimdVec4d::GetInvSqrt() const
{
  xiiSimdVec4d result;
  result.m_v.x = 1.0 / xiiMath::Sqrt(m_v.x);
  result.m_v.y = 1.0 / xiiMath::Sqrt(m_v.y);
  result.m_v.z = 1.0 / xiiMath::Sqrt(m_v.z);
  result.m_v.w = 1.0 / xiiMath::Sqrt(m_v.w);

  return result;
}

template <xiiInt32 N, xiiMathDoubleBits::Enum acc>
void xiiSimdVec4d::NormalizeIfNotZero(const xiiSimdDouble& fEpsilon)
{
  xiiSimdDouble sqLength = GetLengthSquared<N>();
  m_v *= sqLength.GetInvSqrt<acc>();
  m_v = sqLength > fEpsilon.m_v ? m_v : xiiVec4d::MakeZero();
}

template <xiiInt32 N>
XII_ALWAYS_INLINE bool xiiSimdVec4d::IsZero() const
{
  for (xiiInt32 i = 0; i < N; ++i)
  {
    if ((&m_v.x)[i] != 0.0)
      return false;
  }

  return true;
}

template <xiiInt32 N>
XII_ALWAYS_INLINE bool xiiSimdVec4d::IsZero(const xiiSimdDouble& fEpsilon) const
{
  for (xiiInt32 i = 0; i < N; ++i)
  {
    if (!xiiMath::IsZero((&m_v.x)[i], (double)fEpsilon))
      return false;
  }

  return true;
}

template <xiiInt32 N>
XII_ALWAYS_INLINE bool xiiSimdVec4d::IsNaN() const
{
  for (xiiInt32 i = 0; i < N; ++i)
  {
    if (xiiMath::IsNaN((&m_v.x)[i]))
      return true;
  }

  return false;
}

template <xiiInt32 N>
XII_ALWAYS_INLINE bool xiiSimdVec4d::IsValid() const
{
  for (xiiInt32 i = 0; i < N; ++i)
  {
    if (!xiiMath::IsFinite((&m_v.x)[i]))
      return false;
  }

  return true;
}

template <xiiInt32 N>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::GetComponent() const
{
  return (&m_v.x)[N];
}

XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::x() const
{
  return m_v.x;
}

XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::y() const
{
  return m_v.y;
}

XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::z() const
{
  return m_v.z;
}

XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::w() const
{
  return m_v.w;
}

template <xiiSwizzle::Enum s>
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::Get() const
{
  xiiSimdVec4d result;

  const double* v = &m_v.x;
  result.m_v.x    = v[(s & 0x3000) >> 12];
  result.m_v.y    = v[(s & 0x0300) >> 8];
  result.m_v.z    = v[(s & 0x0030) >> 4];
  result.m_v.w    = v[(s & 0x0003)];

  return result;
}

template <xiiSwizzle::Enum s>
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::GetCombined(const xiiSimdVec4d& other) const
{
  xiiSimdVec4d result;

  const double* v = &m_v.x;
  const double* o = &other.m_v.x;
  result.m_v.x    = v[(s & 0x3000) >> 12];
  result.m_v.y    = v[(s & 0x0300) >> 8];
  result.m_v.z    = o[(s & 0x0030) >> 4];
  result.m_v.w    = o[(s & 0x0003)];

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::operator-() const
{
  return -m_v;
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::operator+(const xiiSimdVec4d& v) const
{
  return m_v + v.m_v;
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::operator-(const xiiSimdVec4d& v) const
{
  return m_v - v.m_v;
}


XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::operator*(const xiiSimdDouble& f) const
{
  return m_v * f.m_v.x;
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::operator/(const xiiSimdDouble& f) const
{
  return m_v / f.m_v.x;
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::CompMul(const xiiSimdVec4d& v) const
{
  return m_v.CompMul(v.m_v);
}

template <xiiMathDoubleBits::Enum acc>
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::CompDiv(const xiiSimdVec4d& v) const
{
  return m_v.CompDiv(v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::CompMin(const xiiSimdVec4d& v) const
{
  return m_v.CompMin(v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::CompMax(const xiiSimdVec4d& v) const
{
  return m_v.CompMax(v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::Abs() const
{
  return m_v.Abs();
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::Round() const
{
  xiiSimdVec4d result;
  result.m_v.x = xiiMath::Round(m_v.x);
  result.m_v.y = xiiMath::Round(m_v.y);
  result.m_v.z = xiiMath::Round(m_v.z);
  result.m_v.w = xiiMath::Round(m_v.w);

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::Floor() const
{
  xiiSimdVec4d result;
  result.m_v.x = xiiMath::Floor(m_v.x);
  result.m_v.y = xiiMath::Floor(m_v.y);
  result.m_v.z = xiiMath::Floor(m_v.z);
  result.m_v.w = xiiMath::Floor(m_v.w);

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::Ceil() const
{
  xiiSimdVec4d result;
  result.m_v.x = xiiMath::Ceil(m_v.x);
  result.m_v.y = xiiMath::Ceil(m_v.y);
  result.m_v.z = xiiMath::Ceil(m_v.z);
  result.m_v.w = xiiMath::Ceil(m_v.w);

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::Trunc() const
{
  xiiSimdVec4d result;
  result.m_v.x = xiiMath::Trunc(m_v.x);
  result.m_v.y = xiiMath::Trunc(m_v.y);
  result.m_v.z = xiiMath::Trunc(m_v.z);
  result.m_v.w = xiiMath::Trunc(m_v.w);

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::FlipSign(const xiiSimdVec4b& vCmp) const
{
  xiiSimdVec4d result;

#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE) || (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_NEON)
  result.m_v.x = vCmp.x() ? -m_v.x : m_v.x;
  result.m_v.y = vCmp.y() ? -m_v.y : m_v.y;
  result.m_v.z = vCmp.z() ? -m_v.z : m_v.z;
  result.m_v.w = vCmp.w() ? -m_v.w : m_v.w;
#else
  result.m_v.x = vCmp.m_v.x ? -m_v.x : m_v.x;
  result.m_v.y = vCmp.m_v.y ? -m_v.y : m_v.y;
  result.m_v.z = vCmp.m_v.z ? -m_v.z : m_v.z;
  result.m_v.w = vCmp.m_v.w ? -m_v.w : m_v.w;
#endif

  return result;
}

// static
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::Select(const xiiSimdVec4b& vCmp, const xiiSimdVec4d& vIfTrue, const xiiSimdVec4d& vIfFalse)
{
  xiiSimdVec4d result;

#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE) || (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_NEON)
  result.m_v.x = vCmp.x() ? vIfTrue.m_v.x : vIfFalse.m_v.x;
  result.m_v.y = vCmp.y() ? vIfTrue.m_v.y : vIfFalse.m_v.y;
  result.m_v.z = vCmp.z() ? vIfTrue.m_v.z : vIfFalse.m_v.z;
  result.m_v.w = vCmp.w() ? vIfTrue.m_v.w : vIfFalse.m_v.w;
#else
  result.m_v.x = vCmp.m_v.x ? vIfTrue.m_v.x : vIfFalse.m_v.x;
  result.m_v.y = vCmp.m_v.y ? vIfTrue.m_v.y : vIfFalse.m_v.y;
  result.m_v.z = vCmp.m_v.z ? vIfTrue.m_v.z : vIfFalse.m_v.z;
  result.m_v.w = vCmp.m_v.w ? vIfTrue.m_v.w : vIfFalse.m_v.w;
#endif

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4d& xiiSimdVec4d::operator+=(const xiiSimdVec4d& v)
{
  m_v += v.m_v;
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4d& xiiSimdVec4d::operator-=(const xiiSimdVec4d& v)
{
  m_v -= v.m_v;
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4d& xiiSimdVec4d::operator*=(const xiiSimdDouble& f)
{
  m_v *= f.m_v.x;
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4d& xiiSimdVec4d::operator/=(const xiiSimdDouble& f)
{
  m_v /= f.m_v.x;
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4d::operator==(const xiiSimdVec4d& v) const
{
  bool result[4];
  result[0] = m_v.x == v.m_v.x;
  result[1] = m_v.y == v.m_v.y;
  result[2] = m_v.z == v.m_v.z;
  result[3] = m_v.w == v.m_v.w;

  return xiiSimdVec4b(result[0], result[1], result[2], result[3]);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4d::operator!=(const xiiSimdVec4d& v) const
{
  return !(*this == v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4d::operator<=(const xiiSimdVec4d& v) const
{
  return !(*this > v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4d::operator<(const xiiSimdVec4d& v) const
{
  bool result[4];
  result[0] = m_v.x < v.m_v.x;
  result[1] = m_v.y < v.m_v.y;
  result[2] = m_v.z < v.m_v.z;
  result[3] = m_v.w < v.m_v.w;

  return xiiSimdVec4b(result[0], result[1], result[2], result[3]);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4d::operator>=(const xiiSimdVec4d& v) const
{
  return !(*this < v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4d::operator>(const xiiSimdVec4d& v) const
{
  bool result[4];
  result[0] = m_v.x > v.m_v.x;
  result[1] = m_v.y > v.m_v.y;
  result[2] = m_v.z > v.m_v.z;
  result[3] = m_v.w > v.m_v.w;

  return xiiSimdVec4b(result[0], result[1], result[2], result[3]);
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::HorizontalSum<2>() const
{
  return m_v.x + m_v.y;
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::HorizontalSum<3>() const
{
  return (double)HorizontalSum<2>() + m_v.z;
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::HorizontalSum<4>() const
{
  return (double)HorizontalSum<3>() + m_v.w;
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::HorizontalMin<2>() const
{
  return xiiMath::Min(m_v.x, m_v.y);
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::HorizontalMin<3>() const
{
  return xiiMath::Min((double)HorizontalMin<2>(), m_v.z);
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::HorizontalMin<4>() const
{
  return xiiMath::Min((double)HorizontalMin<3>(), m_v.w);
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::HorizontalMax<2>() const
{
  return xiiMath::Max(m_v.x, m_v.y);
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::HorizontalMax<3>() const
{
  return xiiMath::Max((double)HorizontalMax<2>(), m_v.z);
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::HorizontalMax<4>() const
{
  return xiiMath::Max((double)HorizontalMax<3>(), m_v.w);
}

template <xiiInt32 N>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::Dot(const xiiSimdVec4d& v) const
{
  double result = 0.0;

  for (xiiInt32 i = 0; i < N; ++i)
  {
    result += (&m_v.x)[i] * (&v.m_v.x)[i];
  }

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::CrossRH(const xiiSimdVec4d& v) const
{
  return m_v.GetAsVec3().CrossRH(v.m_v.GetAsVec3()).GetAsVec4(0.0);
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::GetOrthogonalVector() const
{
  if (xiiMath::Abs(m_v.y) < 0.99)
  {
    return xiiVec4d(-m_v.z, 0.0, m_v.x, 0.0);
  }
  else
  {
    return xiiVec4d(0.0, m_v.z, -m_v.y, 0.0);
  }
}

// static
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::MulAdd(const xiiSimdVec4d& a, const xiiSimdVec4d& b, const xiiSimdVec4d& c)
{
  return a.CompMul(b) + c;
}

// static
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::MulAdd(const xiiSimdVec4d& a, const xiiSimdDouble& b, const xiiSimdVec4d& c)
{
  return a * b + c;
}

// static
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::MulSub(const xiiSimdVec4d& a, const xiiSimdVec4d& b, const xiiSimdVec4d& c)
{
  return a.CompMul(b) - c;
}

// static
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::MulSub(const xiiSimdVec4d& a, const xiiSimdDouble& b, const xiiSimdVec4d& c)
{
  return a * b - c;
}

// static
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::CopySign(const xiiSimdVec4d& magnitude, const xiiSimdVec4d& sign)
{
  xiiSimdVec4d result;
  result.m_v.x = sign.m_v.x < 0.0 ? -magnitude.m_v.x : magnitude.m_v.x;
  result.m_v.y = sign.m_v.y < 0.0 ? -magnitude.m_v.y : magnitude.m_v.y;
  result.m_v.z = sign.m_v.z < 0.0 ? -magnitude.m_v.z : magnitude.m_v.z;
  result.m_v.w = sign.m_v.w < 0.0 ? -magnitude.m_v.w : magnitude.m_v.w;

  return result;
}
