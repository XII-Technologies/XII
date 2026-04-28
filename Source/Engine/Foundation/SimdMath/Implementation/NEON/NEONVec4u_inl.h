/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

XII_ALWAYS_INLINE xiiSimdVec4u::xiiSimdVec4u()
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);

#if XII_ENABLED(XII_MATH_CHECK_FOR_NAN)
  m_v = vmovq_n_u32(0xCDCDCDCD);
#endif
}

XII_ALWAYS_INLINE xiiSimdVec4u::xiiSimdVec4u(xiiUInt32 xyzw)
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);

  m_v = vmovq_n_u32(xyzw);
}

XII_ALWAYS_INLINE xiiSimdVec4u::xiiSimdVec4u(xiiUInt32 x, xiiUInt32 y, xiiUInt32 z, xiiUInt32 w)
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);

  alignas(16) xiiUInt32 values[4] = {x, y, z, w};
  m_v                             = vld1q_u32(values);
}

XII_ALWAYS_INLINE xiiSimdVec4u::xiiSimdVec4u(xiiInternal::QuadUInt v)
{
  m_v = v;
}

XII_ALWAYS_INLINE void xiiSimdVec4u::Set(xiiUInt32 xyzw)
{
  m_v = vmovq_n_u32(xyzw);
}

XII_ALWAYS_INLINE void xiiSimdVec4u::Set(xiiUInt32 x, xiiUInt32 y, xiiUInt32 z, xiiUInt32 w)
{
  alignas(16) xiiUInt32 values[4] = {x, y, z, w};
  m_v                             = vld1q_u32(values);
}

XII_ALWAYS_INLINE void xiiSimdVec4u::SetZero()
{
  m_v = vmovq_n_u32(0);
}

// needs to be implemented here because of include dependencies
XII_ALWAYS_INLINE xiiSimdVec4i::xiiSimdVec4i(const xiiSimdVec4u& u) :
  m_v(u.m_v)
{
}

XII_ALWAYS_INLINE xiiSimdVec4u::xiiSimdVec4u(const xiiSimdVec4i& i) :
  m_v(i.m_v)
{
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4u::ToFloat() const
{
  return vcvtq_f32_u32(m_v);
}

// static
XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::Truncate(const xiiSimdVec4f& f)
{
  return vcvtq_u32_f32(f.m_v);
}

template <xiiInt32 N>
XII_ALWAYS_INLINE xiiUInt32 xiiSimdVec4u::GetComponent() const
{
  return vgetq_lane_u32(m_v, N);
}

XII_ALWAYS_INLINE xiiUInt32 xiiSimdVec4u::x() const
{
  return GetComponent<0>();
}

XII_ALWAYS_INLINE xiiUInt32 xiiSimdVec4u::y() const
{
  return GetComponent<1>();
}

XII_ALWAYS_INLINE xiiUInt32 xiiSimdVec4u::z() const
{
  return GetComponent<2>();
}

XII_ALWAYS_INLINE xiiUInt32 xiiSimdVec4u::w() const
{
  return GetComponent<3>();
}

template <xiiSwizzle::Enum s>
XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::Get() const
{
  return __builtin_shufflevector(m_v, m_v, XII_TO_SHUFFLE(s));
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::operator+(const xiiSimdVec4u& v) const
{
  return vaddq_u32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::operator-(const xiiSimdVec4u& v) const
{
  return vsubq_u32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::CompMul(const xiiSimdVec4u& v) const
{
  return vmulq_u32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::operator|(const xiiSimdVec4u& v) const
{
  return vorrq_u32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::operator&(const xiiSimdVec4u& v) const
{
  return vandq_u32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::operator^(const xiiSimdVec4u& v) const
{
  return veorq_u32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::operator~() const
{
  return vmvnq_u32(m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::operator<<(xiiUInt32 uiShift) const
{
  return vshlq_u32(m_v, vmovq_n_u32(uiShift));
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::operator>>(xiiUInt32 uiShift) const
{
  return vshlq_u32(m_v, vmovq_n_u32(-uiShift));
}

XII_ALWAYS_INLINE xiiSimdVec4u& xiiSimdVec4u::operator+=(const xiiSimdVec4u& v)
{
  m_v = vaddq_u32(m_v, v.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4u& xiiSimdVec4u::operator-=(const xiiSimdVec4u& v)
{
  m_v = vsubq_u32(m_v, v.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4u& xiiSimdVec4u::operator|=(const xiiSimdVec4u& v)
{
  m_v = vorrq_u32(m_v, v.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4u& xiiSimdVec4u::operator&=(const xiiSimdVec4u& v)
{
  m_v = vandq_u32(m_v, v.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4u& xiiSimdVec4u::operator^=(const xiiSimdVec4u& v)
{
  m_v = veorq_u32(m_v, v.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4u& xiiSimdVec4u::operator<<=(xiiUInt32 uiShift)
{
  m_v = vshlq_u32(m_v, vmovq_n_u32(uiShift));
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4u& xiiSimdVec4u::operator>>=(xiiUInt32 uiShift)
{
  m_v = vshlq_u32(m_v, vmovq_n_u32(-uiShift));
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::CompMin(const xiiSimdVec4u& v) const
{
  return vminq_u32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::CompMax(const xiiSimdVec4u& v) const
{
  return vmaxq_u32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4u::operator==(const xiiSimdVec4u& v) const
{
  return vceqq_u32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4u::operator!=(const xiiSimdVec4u& v) const
{
  return vmvnq_u32(vceqq_u32(m_v, v.m_v));
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4u::operator<=(const xiiSimdVec4u& v) const
{
  return vcleq_u32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4u::operator<(const xiiSimdVec4u& v) const
{
  return vcltq_u32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4u::operator>=(const xiiSimdVec4u& v) const
{
  return vcgeq_u32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4u::operator>(const xiiSimdVec4u& v) const
{
  return vcgtq_u32(m_v, v.m_v);
}

// static
XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::MakeZero()
{
  return vmovq_n_u32(0);
}

// not needed atm
#if 0
void xiiSimdVec4u::Transpose(xiiSimdVec4u& v0, xiiSimdVec4u& v1, xiiSimdVec4u& v2, xiiSimdVec4u& v3)
{
  uint32x4x2_t P0 = vzipq_u32(v0.m_v, v2.m_v);
  uint32x4x2_t P1 = vzipq_u32(v1.m_v, v3.m_v);

  uint32x4x2_t T0 = vzipq_u32(P0.val[0], P1.val[0]);
  uint32x4x2_t T1 = vzipq_u32(P0.val[1], P1.val[1]);

  v0.m_v = T0.val[0];
  v1.m_v = T0.val[1];
  v2.m_v = T1.val[0];
  v3.m_v = T1.val[1];
}
#endif
