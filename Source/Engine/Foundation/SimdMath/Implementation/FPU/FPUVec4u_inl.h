#pragma once

XII_ALWAYS_INLINE xiiSimdVec4u::xiiSimdVec4u()
{
#if XII_ENABLED(XII_MATH_CHECK_FOR_NAN)
  m_v.Set(0xCDCDCDCD);
#endif
}

XII_ALWAYS_INLINE xiiSimdVec4u::xiiSimdVec4u(xiiUInt32 xyzw)
{
  m_v.Set(xyzw);
}

XII_ALWAYS_INLINE xiiSimdVec4u::xiiSimdVec4u(xiiUInt32 x, xiiUInt32 y, xiiUInt32 z, xiiUInt32 w)
{
  m_v.Set(x, y, z, w);
}

XII_ALWAYS_INLINE xiiSimdVec4u::xiiSimdVec4u(xiiInternal::QuadUInt v)
{
  m_v = v;
}

XII_ALWAYS_INLINE void xiiSimdVec4u::Set(xiiUInt32 xyzw)
{
  m_v.Set(xyzw);
}

XII_ALWAYS_INLINE void xiiSimdVec4u::Set(xiiUInt32 x, xiiUInt32 y, xiiUInt32 z, xiiUInt32 w)
{
  m_v.Set(x, y, z, w);
}

XII_ALWAYS_INLINE void xiiSimdVec4u::SetZero()
{
  m_v.SetZero();
}

// Needs to be implemented here because of include dependencies
XII_ALWAYS_INLINE xiiSimdVec4i::xiiSimdVec4i(const xiiSimdVec4u& u) :
  m_v(u.m_v.x, u.m_v.y, u.m_v.z, u.m_v.w)
{
}

XII_ALWAYS_INLINE xiiSimdVec4u::xiiSimdVec4u(const xiiSimdVec4i& i) :
  m_v(i.m_v.x, i.m_v.y, i.m_v.z, i.m_v.w)
{
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4u::ToFloat() const
{
  xiiSimdVec4f result;
  result.m_v.x = (float)m_v.x;
  result.m_v.y = (float)m_v.y;
  result.m_v.z = (float)m_v.z;
  result.m_v.w = (float)m_v.w;

  return result;
}

// static
XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::Truncate(const xiiSimdVec4f& f)
{
  xiiSimdVec4f clampedF = f.CompMax(xiiSimdVec4f::MakeZero());

  xiiSimdVec4u result;
  result.m_v.x = (xiiUInt32)clampedF.m_v.x;
  result.m_v.y = (xiiUInt32)clampedF.m_v.y;
  result.m_v.z = (xiiUInt32)clampedF.m_v.z;
  result.m_v.w = (xiiUInt32)clampedF.m_v.w;

  return result;
}

template <xiiInt32 N>
XII_ALWAYS_INLINE xiiUInt32 xiiSimdVec4u::GetComponent() const
{
  return (&m_v.x)[N];
}

XII_ALWAYS_INLINE xiiUInt32 xiiSimdVec4u::x() const
{
  return m_v.x;
}

XII_ALWAYS_INLINE xiiUInt32 xiiSimdVec4u::y() const
{
  return m_v.y;
}

XII_ALWAYS_INLINE xiiUInt32 xiiSimdVec4u::z() const
{
  return m_v.z;
}

XII_ALWAYS_INLINE xiiUInt32 xiiSimdVec4u::w() const
{
  return m_v.w;
}

template <xiiSwizzle::Enum s>
XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::Get() const
{
  xiiSimdVec4u result;

  const xiiUInt32* v = &m_v.x;
  result.m_v.x       = v[(s & 0x3000) >> 12];
  result.m_v.y       = v[(s & 0x0300) >> 8];
  result.m_v.z       = v[(s & 0x0030) >> 4];
  result.m_v.w       = v[(s & 0x0003)];

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::operator+(const xiiSimdVec4u& v) const
{
  return m_v + v.m_v;
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::operator-(const xiiSimdVec4u& v) const
{
  return m_v - v.m_v;
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::CompMul(const xiiSimdVec4u& v) const
{
  return m_v.CompMul(v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::operator|(const xiiSimdVec4u& v) const
{
  xiiSimdVec4u result;
  result.m_v.x = m_v.x | v.m_v.x;
  result.m_v.y = m_v.y | v.m_v.y;
  result.m_v.z = m_v.z | v.m_v.z;
  result.m_v.w = m_v.w | v.m_v.w;

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::operator&(const xiiSimdVec4u& v) const
{
  xiiSimdVec4u result;
  result.m_v.x = m_v.x & v.m_v.x;
  result.m_v.y = m_v.y & v.m_v.y;
  result.m_v.z = m_v.z & v.m_v.z;
  result.m_v.w = m_v.w & v.m_v.w;

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::operator^(const xiiSimdVec4u& v) const
{
  xiiSimdVec4u result;
  result.m_v.x = m_v.x ^ v.m_v.x;
  result.m_v.y = m_v.y ^ v.m_v.y;
  result.m_v.z = m_v.z ^ v.m_v.z;
  result.m_v.w = m_v.w ^ v.m_v.w;

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::operator~() const
{
  xiiSimdVec4u result;
  result.m_v.x = ~m_v.x;
  result.m_v.y = ~m_v.y;
  result.m_v.z = ~m_v.z;
  result.m_v.w = ~m_v.w;

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::operator<<(xiiUInt32 uiShift) const
{
  xiiSimdVec4u result;
  result.m_v.x = m_v.x << uiShift;
  result.m_v.y = m_v.y << uiShift;
  result.m_v.z = m_v.z << uiShift;
  result.m_v.w = m_v.w << uiShift;

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::operator>>(xiiUInt32 uiShift) const
{
  xiiSimdVec4u result;
  result.m_v.x = m_v.x >> uiShift;
  result.m_v.y = m_v.y >> uiShift;
  result.m_v.z = m_v.z >> uiShift;
  result.m_v.w = m_v.w >> uiShift;

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4u& xiiSimdVec4u::operator+=(const xiiSimdVec4u& v)
{
  m_v += v.m_v;
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4u& xiiSimdVec4u::operator-=(const xiiSimdVec4u& v)
{
  m_v -= v.m_v;
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4u& xiiSimdVec4u::operator|=(const xiiSimdVec4u& v)
{
  m_v.x |= v.m_v.x;
  m_v.y |= v.m_v.y;
  m_v.z |= v.m_v.z;
  m_v.w |= v.m_v.w;
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4u& xiiSimdVec4u::operator&=(const xiiSimdVec4u& v)
{
  m_v.x &= v.m_v.x;
  m_v.y &= v.m_v.y;
  m_v.z &= v.m_v.z;
  m_v.w &= v.m_v.w;
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4u& xiiSimdVec4u::operator^=(const xiiSimdVec4u& v)
{
  m_v.x ^= v.m_v.x;
  m_v.y ^= v.m_v.y;
  m_v.z ^= v.m_v.z;
  m_v.w ^= v.m_v.w;
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4u& xiiSimdVec4u::operator<<=(xiiUInt32 uiShift)
{
  m_v.x <<= uiShift;
  m_v.y <<= uiShift;
  m_v.z <<= uiShift;
  m_v.w <<= uiShift;
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4u& xiiSimdVec4u::operator>>=(xiiUInt32 uiShift)
{
  m_v.x >>= uiShift;
  m_v.y >>= uiShift;
  m_v.z >>= uiShift;
  m_v.w >>= uiShift;
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::CompMin(const xiiSimdVec4u& v) const
{
  return m_v.CompMin(v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::CompMax(const xiiSimdVec4u& v) const
{
  return m_v.CompMax(v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4u::operator==(const xiiSimdVec4u& v) const
{
  bool result[4];
  result[0] = m_v.x == v.m_v.x;
  result[1] = m_v.y == v.m_v.y;
  result[2] = m_v.z == v.m_v.z;
  result[3] = m_v.w == v.m_v.w;

  return xiiSimdVec4b(result[0], result[1], result[2], result[3]);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4u::operator!=(const xiiSimdVec4u& v) const
{
  return !(*this == v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4u::operator<=(const xiiSimdVec4u& v) const
{
  return !(*this > v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4u::operator<(const xiiSimdVec4u& v) const
{
  bool result[4];
  result[0] = m_v.x < v.m_v.x;
  result[1] = m_v.y < v.m_v.y;
  result[2] = m_v.z < v.m_v.z;
  result[3] = m_v.w < v.m_v.w;

  return xiiSimdVec4b(result[0], result[1], result[2], result[3]);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4u::operator>=(const xiiSimdVec4u& v) const
{
  return !(*this < v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4u::operator>(const xiiSimdVec4u& v) const
{
  bool result[4];
  result[0] = m_v.x > v.m_v.x;
  result[1] = m_v.y > v.m_v.y;
  result[2] = m_v.z > v.m_v.z;
  result[3] = m_v.w > v.m_v.w;

  return xiiSimdVec4b(result[0], result[1], result[2], result[3]);
}

// static
XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::MakeZero()
{
  return xiiVec4U32::MakeZero();
}
