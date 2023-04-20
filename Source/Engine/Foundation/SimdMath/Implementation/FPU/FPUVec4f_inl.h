#pragma once

XII_ALWAYS_INLINE xiiSimdVec4f::xiiSimdVec4f() {}

XII_ALWAYS_INLINE xiiSimdVec4f::xiiSimdVec4f(float xyzw)
{
  m_v.Set(xyzw);
}

XII_ALWAYS_INLINE xiiSimdVec4f::xiiSimdVec4f(const xiiSimdFloat& xyzw)
{
  m_v = xyzw.m_v;
}

XII_ALWAYS_INLINE xiiSimdVec4f::xiiSimdVec4f(float x, float y, float z, float w)
{
  m_v.Set(x, y, z, w);
}

XII_ALWAYS_INLINE void xiiSimdVec4f::Set(float xyzw)
{
  m_v.Set(xyzw);
}

XII_ALWAYS_INLINE void xiiSimdVec4f::Set(float x, float y, float z, float w)
{
  m_v.Set(x, y, z, w);
}

XII_ALWAYS_INLINE void xiiSimdVec4f::SetX(const xiiSimdFloat& f)
{
  m_v.x = f.m_v.x;
}

XII_ALWAYS_INLINE void xiiSimdVec4f::SetY(const xiiSimdFloat& f)
{
  m_v.y = f.m_v.x;
}

XII_ALWAYS_INLINE void xiiSimdVec4f::SetZ(const xiiSimdFloat& f)
{
  m_v.z = f.m_v.x;
}

XII_ALWAYS_INLINE void xiiSimdVec4f::SetW(const xiiSimdFloat& f)
{
  m_v.w = f.m_v.x;
}

XII_ALWAYS_INLINE void xiiSimdVec4f::SetZero()
{
  m_v.SetZero();
}

template <xiiInt32 N>
XII_ALWAYS_INLINE void xiiSimdVec4f::Load(const float* pFloats)
{
  m_v.SetZero();
  for (xiiUInt32 i = 0; i < N; ++i)
  {
    (&m_v.x)[i] = pFloats[i];
  }
}

template <xiiInt32 N>
XII_ALWAYS_INLINE void xiiSimdVec4f::Store(float* pFloats) const
{
  for (xiiUInt32 i = 0; i < N; ++i)
  {
    pFloats[i] = (&m_v.x)[i];
  }
}

template <xiiMathFloatBits::Enum acc>
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::GetReciprocal() const
{
  return xiiVec4(1.0f).CompDiv(m_v);
}

template <xiiMathFloatBits::Enum acc>
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::GetSqrt() const
{
  xiiSimdVec4f result;
  result.m_v.x = xiiMath::Sqrt(m_v.x);
  result.m_v.y = xiiMath::Sqrt(m_v.y);
  result.m_v.z = xiiMath::Sqrt(m_v.z);
  result.m_v.w = xiiMath::Sqrt(m_v.w);

  return result;
}

template <xiiMathFloatBits::Enum acc>
xiiSimdVec4f xiiSimdVec4f::GetInvSqrt() const
{
  xiiSimdVec4f result;
  result.m_v.x = 1.0f / xiiMath::Sqrt(m_v.x);
  result.m_v.y = 1.0f / xiiMath::Sqrt(m_v.y);
  result.m_v.z = 1.0f / xiiMath::Sqrt(m_v.z);
  result.m_v.w = 1.0f / xiiMath::Sqrt(m_v.w);

  return result;
}

template <xiiInt32 N, xiiMathFloatBits::Enum acc>
void xiiSimdVec4f::NormalizeIfNotZero(const xiiSimdFloat& fEpsilon)
{
  xiiSimdFloat sqLength = GetLengthSquared<N>();
  m_v *= sqLength.GetInvSqrt<acc>();
  m_v = sqLength > fEpsilon.m_v ? m_v : xiiVec4::ZeroVector();
}

template <xiiInt32 N>
XII_ALWAYS_INLINE bool xiiSimdVec4f::IsZero() const
{
  for (xiiUInt32 i = 0; i < N; ++i)
  {
    if ((&m_v.x)[i] != 0.0f)
      return false;
  }

  return true;
}

template <xiiInt32 N>
XII_ALWAYS_INLINE bool xiiSimdVec4f::IsZero(const xiiSimdFloat& fEpsilon) const
{
  for (xiiUInt32 i = 0; i < N; ++i)
  {
    if (!xiiMath::IsZero((&m_v.x)[i], (float)fEpsilon))
      return false;
  }

  return true;
}

template <xiiInt32 N>
XII_ALWAYS_INLINE bool xiiSimdVec4f::IsNaN() const
{
  for (xiiUInt32 i = 0; i < N; ++i)
  {
    if (xiiMath::IsNaN((&m_v.x)[i]))
      return true;
  }

  return false;
}

template <xiiInt32 N>
XII_ALWAYS_INLINE bool xiiSimdVec4f::IsValid() const
{
  for (xiiUInt32 i = 0; i < N; ++i)
  {
    if (!xiiMath::IsFinite((&m_v.x)[i]))
      return false;
  }

  return true;
}

template <xiiInt32 N>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::GetComponent() const
{
  return (&m_v.x)[N];
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::x() const
{
  return m_v.x;
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::y() const
{
  return m_v.y;
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::z() const
{
  return m_v.z;
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::w() const
{
  return m_v.w;
}

template <xiiSwizzle::Enum s>
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::Get() const
{
  xiiSimdVec4f result;

  const float* v = &m_v.x;
  result.m_v.x   = v[(s & 0x3000) >> 12];
  result.m_v.y   = v[(s & 0x0300) >> 8];
  result.m_v.z   = v[(s & 0x0030) >> 4];
  result.m_v.w   = v[(s & 0x0003)];

  return result;
}

template <xiiSwizzle::Enum s>
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::GetCombined(const xiiSimdVec4f& other) const
{
  xiiSimdVec4f result;

  const float* v = &m_v.x;
  const float* o = &other.m_v.x;
  result.m_v.x   = v[(s & 0x3000) >> 12];
  result.m_v.y   = v[(s & 0x0300) >> 8];
  result.m_v.z   = o[(s & 0x0030) >> 4];
  result.m_v.w   = o[(s & 0x0003)];

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::operator-() const
{
  return -m_v;
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::operator+(const xiiSimdVec4f& v) const
{
  return m_v + v.m_v;
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::operator-(const xiiSimdVec4f& v) const
{
  return m_v - v.m_v;
}


XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::operator*(const xiiSimdFloat& f) const
{
  return m_v * f.m_v.x;
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::operator/(const xiiSimdFloat& f) const
{
  return m_v / f.m_v.x;
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::CompMul(const xiiSimdVec4f& v) const
{
  return m_v.CompMul(v.m_v);
}

template <xiiMathFloatBits::Enum acc>
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::CompDiv(const xiiSimdVec4f& v) const
{
  return m_v.CompDiv(v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::CompMin(const xiiSimdVec4f& v) const
{
  return m_v.CompMin(v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::CompMax(const xiiSimdVec4f& v) const
{
  return m_v.CompMax(v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::Abs() const
{
  return m_v.Abs();
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::Round() const
{
  xiiSimdVec4f result;
  result.m_v.x = xiiMath::Round(m_v.x);
  result.m_v.y = xiiMath::Round(m_v.y);
  result.m_v.z = xiiMath::Round(m_v.z);
  result.m_v.w = xiiMath::Round(m_v.w);

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::Floor() const
{
  xiiSimdVec4f result;
  result.m_v.x = xiiMath::Floor(m_v.x);
  result.m_v.y = xiiMath::Floor(m_v.y);
  result.m_v.z = xiiMath::Floor(m_v.z);
  result.m_v.w = xiiMath::Floor(m_v.w);

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::Ceil() const
{
  xiiSimdVec4f result;
  result.m_v.x = xiiMath::Ceil(m_v.x);
  result.m_v.y = xiiMath::Ceil(m_v.y);
  result.m_v.z = xiiMath::Ceil(m_v.z);
  result.m_v.w = xiiMath::Ceil(m_v.w);

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::Trunc() const
{
  xiiSimdVec4f result;
  result.m_v.x = xiiMath::Trunc(m_v.x);
  result.m_v.y = xiiMath::Trunc(m_v.y);
  result.m_v.z = xiiMath::Trunc(m_v.z);
  result.m_v.w = xiiMath::Trunc(m_v.w);

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::FlipSign(const xiiSimdVec4b& cmp) const
{
  xiiSimdVec4f result;
  result.m_v.x = cmp.m_v.x ? -m_v.x : m_v.x;
  result.m_v.y = cmp.m_v.y ? -m_v.y : m_v.y;
  result.m_v.z = cmp.m_v.z ? -m_v.z : m_v.z;
  result.m_v.w = cmp.m_v.w ? -m_v.w : m_v.w;

  return result;
}

// static
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::Select(const xiiSimdVec4b& cmp, const xiiSimdVec4f& ifTrue, const xiiSimdVec4f& ifFalse)
{
  xiiSimdVec4f result;
  result.m_v.x = cmp.m_v.x ? ifTrue.m_v.x : ifFalse.m_v.x;
  result.m_v.y = cmp.m_v.y ? ifTrue.m_v.y : ifFalse.m_v.y;
  result.m_v.z = cmp.m_v.z ? ifTrue.m_v.z : ifFalse.m_v.z;
  result.m_v.w = cmp.m_v.w ? ifTrue.m_v.w : ifFalse.m_v.w;

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4f& xiiSimdVec4f::operator+=(const xiiSimdVec4f& v)
{
  m_v += v.m_v;
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4f& xiiSimdVec4f::operator-=(const xiiSimdVec4f& v)
{
  m_v -= v.m_v;
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4f& xiiSimdVec4f::operator*=(const xiiSimdFloat& f)
{
  m_v *= f.m_v.x;
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4f& xiiSimdVec4f::operator/=(const xiiSimdFloat& f)
{
  m_v /= f.m_v.x;
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4f::operator==(const xiiSimdVec4f& v) const
{
  bool result[4];
  result[0] = m_v.x == v.m_v.x;
  result[1] = m_v.y == v.m_v.y;
  result[2] = m_v.z == v.m_v.z;
  result[3] = m_v.w == v.m_v.w;

  return xiiSimdVec4b(result[0], result[1], result[2], result[3]);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4f::operator!=(const xiiSimdVec4f& v) const
{
  return !(*this == v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4f::operator<=(const xiiSimdVec4f& v) const
{
  return !(*this > v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4f::operator<(const xiiSimdVec4f& v) const
{
  bool result[4];
  result[0] = m_v.x < v.m_v.x;
  result[1] = m_v.y < v.m_v.y;
  result[2] = m_v.z < v.m_v.z;
  result[3] = m_v.w < v.m_v.w;

  return xiiSimdVec4b(result[0], result[1], result[2], result[3]);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4f::operator>=(const xiiSimdVec4f& v) const
{
  return !(*this < v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4f::operator>(const xiiSimdVec4f& v) const
{
  bool result[4];
  result[0] = m_v.x > v.m_v.x;
  result[1] = m_v.y > v.m_v.y;
  result[2] = m_v.z > v.m_v.z;
  result[3] = m_v.w > v.m_v.w;

  return xiiSimdVec4b(result[0], result[1], result[2], result[3]);
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::HorizontalSum<2>() const
{
  return m_v.x + m_v.y;
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::HorizontalSum<3>() const
{
  return (float)HorizontalSum<2>() + m_v.z;
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::HorizontalSum<4>() const
{
  return (float)HorizontalSum<3>() + m_v.w;
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::HorizontalMin<2>() const
{
  return xiiMath::Min(m_v.x, m_v.y);
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::HorizontalMin<3>() const
{
  return xiiMath::Min((float)HorizontalMin<2>(), m_v.z);
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::HorizontalMin<4>() const
{
  return xiiMath::Min((float)HorizontalMin<3>(), m_v.w);
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::HorizontalMax<2>() const
{
  return xiiMath::Max(m_v.x, m_v.y);
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::HorizontalMax<3>() const
{
  return xiiMath::Max((float)HorizontalMax<2>(), m_v.z);
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::HorizontalMax<4>() const
{
  return xiiMath::Max((float)HorizontalMax<3>(), m_v.w);
}

template <int N>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::Dot(const xiiSimdVec4f& v) const
{
  float result = 0.0f;

  for (int i = 0; i < N; ++i)
  {
    result += (&m_v.x)[i] * (&v.m_v.x)[i];
  }

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::CrossRH(const xiiSimdVec4f& v) const
{
  return m_v.GetAsVec3().CrossRH(v.m_v.GetAsVec3()).GetAsVec4(0.0f);
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::GetOrthogonalVector() const
{
  if (xiiMath::Abs(m_v.y) < 0.99f)
  {
    return xiiVec4(-m_v.z, 0.0f, m_v.x, 0.0f);
  }
  else
  {
    return xiiVec4(0.0f, m_v.z, -m_v.y, 0.0f);
  }
}

// static
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::ZeroVector()
{
  return xiiVec4::ZeroVector();
}

// static
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::MulAdd(const xiiSimdVec4f& a, const xiiSimdVec4f& b, const xiiSimdVec4f& c)
{
  return a.CompMul(b) + c;
}

// static
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::MulAdd(const xiiSimdVec4f& a, const xiiSimdFloat& b, const xiiSimdVec4f& c)
{
  return a * b + c;
}

// static
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::MulSub(const xiiSimdVec4f& a, const xiiSimdVec4f& b, const xiiSimdVec4f& c)
{
  return a.CompMul(b) - c;
}

// static
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::MulSub(const xiiSimdVec4f& a, const xiiSimdFloat& b, const xiiSimdVec4f& c)
{
  return a * b - c;
}

// static
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::CopySign(const xiiSimdVec4f& magnitude, const xiiSimdVec4f& sign)
{
  xiiSimdVec4f result;
  result.m_v.x = sign.m_v.x < 0.0f ? -magnitude.m_v.x : magnitude.m_v.x;
  result.m_v.y = sign.m_v.y < 0.0f ? -magnitude.m_v.y : magnitude.m_v.y;
  result.m_v.z = sign.m_v.z < 0.0f ? -magnitude.m_v.z : magnitude.m_v.z;
  result.m_v.w = sign.m_v.w < 0.0f ? -magnitude.m_v.w : magnitude.m_v.w;

  return result;
}
