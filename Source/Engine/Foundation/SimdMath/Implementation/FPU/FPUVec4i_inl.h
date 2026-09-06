/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

XII_ALWAYS_INLINE xiiSimdVec4i::xiiSimdVec4i()
{
#if XII_ENABLED(XII_MATH_CHECK_FOR_NAN)
  m_v.Set(0xCDCDCDCD);
#endif
}

XII_ALWAYS_INLINE xiiSimdVec4i::xiiSimdVec4i(xiiInt32 xyzw)
{
  m_v.Set(xyzw);
}

XII_ALWAYS_INLINE xiiSimdVec4i::xiiSimdVec4i(xiiInt32 x, xiiInt32 y, xiiInt32 z, xiiInt32 w)
{
  m_v.Set(x, y, z, w);
}

XII_ALWAYS_INLINE xiiSimdVec4i::xiiSimdVec4i(xiiInternal::QuadInt v)
{
  m_v = v;
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::MakeZero()
{
  return xiiSimdVec4i(0);
}

XII_ALWAYS_INLINE void xiiSimdVec4i::Set(xiiInt32 xyzw)
{
  m_v.Set(xyzw);
}

XII_ALWAYS_INLINE void xiiSimdVec4i::Set(xiiInt32 x, xiiInt32 y, xiiInt32 z, xiiInt32 w)
{
  m_v.Set(x, y, z, w);
}

XII_ALWAYS_INLINE void xiiSimdVec4i::SetZero()
{
  m_v.SetZero();
}

template <xiiInt32 N>
XII_ALWAYS_INLINE void xiiSimdVec4i::Load(const xiiInt32* pInts)
{
  m_v.SetZero();
  for (xiiUInt32 i = 0; i < N; ++i)
  {
    (&m_v.x)[i] = pInts[i];
  }
}

template <xiiInt32 N>
XII_ALWAYS_INLINE void xiiSimdVec4i::Store(xiiInt32* pInts) const
{
  for (xiiUInt32 i = 0; i < N; ++i)
  {
    pInts[i] = (&m_v.x)[i];
  }
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4i::ToFloat() const
{
  xiiSimdVec4f result;
  result.m_v.x = (float)m_v.x;
  result.m_v.y = (float)m_v.y;
  result.m_v.z = (float)m_v.z;
  result.m_v.w = (float)m_v.w;

  return result;
}

// static
XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::Truncate(const xiiSimdVec4f& f)
{
  xiiSimdVec4i result;
  result.m_v.x = (xiiInt32)f.m_v.x;
  result.m_v.y = (xiiInt32)f.m_v.y;
  result.m_v.z = (xiiInt32)f.m_v.z;
  result.m_v.w = (xiiInt32)f.m_v.w;

  return result;
}

template <xiiInt32 N>
XII_ALWAYS_INLINE xiiInt32 xiiSimdVec4i::GetComponent() const
{
  return (&m_v.x)[N];
}

XII_ALWAYS_INLINE xiiInt32 xiiSimdVec4i::x() const
{
  return m_v.x;
}

XII_ALWAYS_INLINE xiiInt32 xiiSimdVec4i::y() const
{
  return m_v.y;
}

XII_ALWAYS_INLINE xiiInt32 xiiSimdVec4i::z() const
{
  return m_v.z;
}

XII_ALWAYS_INLINE xiiInt32 xiiSimdVec4i::w() const
{
  return m_v.w;
}

template <xiiSwizzle::Enum s>
XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::Get() const
{
  xiiSimdVec4i result;

  const xiiInt32* v = &m_v.x;
  result.m_v.x      = v[(s & 0x3000) >> 12];
  result.m_v.y      = v[(s & 0x0300) >> 8];
  result.m_v.z      = v[(s & 0x0030) >> 4];
  result.m_v.w      = v[(s & 0x0003)];

  return result;
}

template <xiiSwizzle::Enum s>
XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::GetCombined(const xiiSimdVec4i& other) const
{
  xiiSimdVec4i result;

  const xiiInt32* v = &m_v.x;
  const xiiInt32* o = &other.m_v.x;
  result.m_v.x      = v[(s & 0x3000) >> 12];
  result.m_v.y      = v[(s & 0x0300) >> 8];
  result.m_v.z      = o[(s & 0x0030) >> 4];
  result.m_v.w      = o[(s & 0x0003)];

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator-() const
{
  return -m_v;
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator+(const xiiSimdVec4i& v) const
{
  return m_v + v.m_v;
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator-(const xiiSimdVec4i& v) const
{
  return m_v - v.m_v;
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::CompMul(const xiiSimdVec4i& v) const
{
  return m_v.CompMul(v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::CompDiv(const xiiSimdVec4i& v) const
{
  return m_v.CompDiv(v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator|(const xiiSimdVec4i& v) const
{
  xiiSimdVec4i result;
  result.m_v.x = m_v.x | v.m_v.x;
  result.m_v.y = m_v.y | v.m_v.y;
  result.m_v.z = m_v.z | v.m_v.z;
  result.m_v.w = m_v.w | v.m_v.w;

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator&(const xiiSimdVec4i& v) const
{
  xiiSimdVec4i result;
  result.m_v.x = m_v.x & v.m_v.x;
  result.m_v.y = m_v.y & v.m_v.y;
  result.m_v.z = m_v.z & v.m_v.z;
  result.m_v.w = m_v.w & v.m_v.w;

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator^(const xiiSimdVec4i& v) const
{
  xiiSimdVec4i result;
  result.m_v.x = m_v.x ^ v.m_v.x;
  result.m_v.y = m_v.y ^ v.m_v.y;
  result.m_v.z = m_v.z ^ v.m_v.z;
  result.m_v.w = m_v.w ^ v.m_v.w;

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator~() const
{
  xiiSimdVec4i result;
  result.m_v.x = ~m_v.x;
  result.m_v.y = ~m_v.y;
  result.m_v.z = ~m_v.z;
  result.m_v.w = ~m_v.w;

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator<<(xiiUInt32 uiShift) const
{
  xiiSimdVec4i result;
  result.m_v.x = m_v.x << uiShift;
  result.m_v.y = m_v.y << uiShift;
  result.m_v.z = m_v.z << uiShift;
  result.m_v.w = m_v.w << uiShift;

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator>>(xiiUInt32 uiShift) const
{
  xiiSimdVec4i result;
  result.m_v.x = m_v.x >> uiShift;
  result.m_v.y = m_v.y >> uiShift;
  result.m_v.z = m_v.z >> uiShift;
  result.m_v.w = m_v.w >> uiShift;

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator<<(const xiiSimdVec4i& v) const
{
  xiiSimdVec4i result;
  result.m_v.x = m_v.x << v.m_v.x;
  result.m_v.y = m_v.y << v.m_v.y;
  result.m_v.z = m_v.z << v.m_v.z;
  result.m_v.w = m_v.w << v.m_v.w;

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator>>(const xiiSimdVec4i& v) const
{
  xiiSimdVec4i result;
  result.m_v.x = m_v.x >> v.m_v.x;
  result.m_v.y = m_v.y >> v.m_v.y;
  result.m_v.z = m_v.z >> v.m_v.z;
  result.m_v.w = m_v.w >> v.m_v.w;

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4i& xiiSimdVec4i::operator+=(const xiiSimdVec4i& v)
{
  m_v += v.m_v;
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4i& xiiSimdVec4i::operator-=(const xiiSimdVec4i& v)
{
  m_v -= v.m_v;
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4i& xiiSimdVec4i::operator|=(const xiiSimdVec4i& v)
{
  m_v.x |= v.m_v.x;
  m_v.y |= v.m_v.y;
  m_v.z |= v.m_v.z;
  m_v.w |= v.m_v.w;
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4i& xiiSimdVec4i::operator&=(const xiiSimdVec4i& v)
{
  m_v.x &= v.m_v.x;
  m_v.y &= v.m_v.y;
  m_v.z &= v.m_v.z;
  m_v.w &= v.m_v.w;
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4i& xiiSimdVec4i::operator^=(const xiiSimdVec4i& v)
{
  m_v.x ^= v.m_v.x;
  m_v.y ^= v.m_v.y;
  m_v.z ^= v.m_v.z;
  m_v.w ^= v.m_v.w;
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4i& xiiSimdVec4i::operator<<=(xiiUInt32 uiShift)
{
  m_v.x <<= uiShift;
  m_v.y <<= uiShift;
  m_v.z <<= uiShift;
  m_v.w <<= uiShift;
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4i& xiiSimdVec4i::operator>>=(xiiUInt32 uiShift)
{
  m_v.x >>= uiShift;
  m_v.y >>= uiShift;
  m_v.z >>= uiShift;
  m_v.w >>= uiShift;
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::CompMin(const xiiSimdVec4i& v) const
{
  return m_v.CompMin(v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::CompMax(const xiiSimdVec4i& v) const
{
  return m_v.CompMax(v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::Abs() const
{
  xiiSimdVec4i result;
  result.m_v.x = xiiMath::Abs(m_v.x);
  result.m_v.y = xiiMath::Abs(m_v.y);
  result.m_v.z = xiiMath::Abs(m_v.z);
  result.m_v.w = xiiMath::Abs(m_v.w);

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4i::operator==(const xiiSimdVec4i& v) const
{
  bool result[4];
  result[0] = m_v.x == v.m_v.x;
  result[1] = m_v.y == v.m_v.y;
  result[2] = m_v.z == v.m_v.z;
  result[3] = m_v.w == v.m_v.w;

  return xiiSimdVec4b(result[0], result[1], result[2], result[3]);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4i::operator!=(const xiiSimdVec4i& v) const
{
  return !(*this == v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4i::operator<=(const xiiSimdVec4i& v) const
{
  return !(*this > v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4i::operator<(const xiiSimdVec4i& v) const
{
  bool result[4];
  result[0] = m_v.x < v.m_v.x;
  result[1] = m_v.y < v.m_v.y;
  result[2] = m_v.z < v.m_v.z;
  result[3] = m_v.w < v.m_v.w;

  return xiiSimdVec4b(result[0], result[1], result[2], result[3]);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4i::operator>=(const xiiSimdVec4i& v) const
{
  return !(*this < v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4i::operator>(const xiiSimdVec4i& v) const
{
  bool result[4];
  result[0] = m_v.x > v.m_v.x;
  result[1] = m_v.y > v.m_v.y;
  result[2] = m_v.z > v.m_v.z;
  result[3] = m_v.w > v.m_v.w;

  return xiiSimdVec4b(result[0], result[1], result[2], result[3]);
}

// static
XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::Select(const xiiSimdVec4b& cmp, const xiiSimdVec4i& ifTrue, const xiiSimdVec4i& ifFalse)
{
  xiiSimdVec4i result;
  result.m_v.x = cmp.m_v.x ? ifTrue.m_v.x : ifFalse.m_v.x;
  result.m_v.y = cmp.m_v.y ? ifTrue.m_v.y : ifFalse.m_v.y;
  result.m_v.z = cmp.m_v.z ? ifTrue.m_v.z : ifFalse.m_v.z;
  result.m_v.w = cmp.m_v.w ? ifTrue.m_v.w : ifFalse.m_v.w;

  return result;
}
