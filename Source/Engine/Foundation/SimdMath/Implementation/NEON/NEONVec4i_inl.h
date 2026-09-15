/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

XII_ALWAYS_INLINE xiiSimdVec4i::xiiSimdVec4i()
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);

#if XII_ENABLED(XII_MATH_CHECK_FOR_NAN)
  m_v = vmovq_n_u32(0xCDCDCDCD);
#endif
}

XII_ALWAYS_INLINE xiiSimdVec4i::xiiSimdVec4i(xiiInt32 xyzw)
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);

  m_v = vmovq_n_s32(xyzw);
}

XII_ALWAYS_INLINE xiiSimdVec4i::xiiSimdVec4i(xiiInt32 x, xiiInt32 y, xiiInt32 z, xiiInt32 w)
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);

  alignas(16) xiiInt32 values[4] = {x, y, z, w};
  m_v                            = vld1q_s32(values);
}

XII_ALWAYS_INLINE xiiSimdVec4i::xiiSimdVec4i(xiiInternal::QuadInt v)
{
  m_v = v;
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::MakeZero()
{
  return vmovq_n_s32(0);
}

XII_ALWAYS_INLINE void xiiSimdVec4i::Set(xiiInt32 xyzw)
{
  m_v = vmovq_n_s32(xyzw);
}

XII_ALWAYS_INLINE void xiiSimdVec4i::Set(xiiInt32 x, xiiInt32 y, xiiInt32 z, xiiInt32 w)
{
  alignas(16) xiiInt32 values[4] = {x, y, z, w};
  m_v                            = vld1q_s32(values);
}

XII_ALWAYS_INLINE void xiiSimdVec4i::SetZero()
{
  m_v = vmovq_n_s32(0);
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4i::Load<1>(const xiiInt32* pInts)
{
  m_v = vld1q_lane_s32(pInts, vmovq_n_s32(0), 0);
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4i::Load<2>(const xiiInt32* pInts)
{
  m_v = vreinterpretq_s32_s64(vld1q_lane_s64(reinterpret_cast<const int64_t*>(pInts), vmovq_n_s64(0), 0));
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4i::Load<3>(const xiiInt32* pInts)
{
  m_v = vcombine_s32(vld1_s32(pInts), vld1_lane_s32(pInts + 2, vmov_n_s32(0), 0));
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4i::Load<4>(const xiiInt32* pInts)
{
  m_v = vld1q_s32(pInts);
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4i::Store<1>(xiiInt32* pInts) const
{
  vst1q_lane_s32(pInts, m_v, 0);
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4i::Store<2>(xiiInt32* pInts) const
{
  vst1q_lane_s64(reinterpret_cast<int64_t*>(pInts), vreinterpretq_s64_s32(m_v), 0);
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4i::Store<3>(xiiInt32* pInts) const
{
  vst1q_lane_s64(reinterpret_cast<int64_t*>(pInts), vreinterpretq_s64_s32(m_v), 0);
  vst1q_lane_s32(pInts + 2, m_v, 2);
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4i::Store<4>(xiiInt32* pInts) const
{
  vst1q_s32(pInts, m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4i::ToFloat() const
{
  return vcvtq_f32_s32(m_v);
}

// static
XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::Truncate(const xiiSimdVec4f& f)
{
  return vcvtq_s32_f32(f.m_v);
}

template <xiiInt32 N>
XII_ALWAYS_INLINE xiiInt32 xiiSimdVec4i::GetComponent() const
{
  return vgetq_lane_s32(m_v, N);
}

XII_ALWAYS_INLINE xiiInt32 xiiSimdVec4i::x() const
{
  return GetComponent<0>();
}

XII_ALWAYS_INLINE xiiInt32 xiiSimdVec4i::y() const
{
  return GetComponent<1>();
}

XII_ALWAYS_INLINE xiiInt32 xiiSimdVec4i::z() const
{
  return GetComponent<2>();
}

XII_ALWAYS_INLINE xiiInt32 xiiSimdVec4i::w() const
{
  return GetComponent<3>();
}

template <xiiSwizzle::Enum s>
XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::Get() const
{
  return __builtin_shufflevector(m_v, m_v, XII_TO_SHUFFLE(s));
}

template <xiiSwizzle::Enum s>
XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::GetCombined(const xiiSimdVec4i& other) const
{
  return __builtin_shufflevector(m_v, other.m_v, XII_TO_SHUFFLE(s));
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator-() const
{
  return vnegq_s32(m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator+(const xiiSimdVec4i& v) const
{
  return vaddq_s32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator-(const xiiSimdVec4i& v) const
{
  return vsubq_s32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::CompMul(const xiiSimdVec4i& v) const
{
  return vmulq_s32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::CompDiv(const xiiSimdVec4i& v) const
{
  xiiInt32 a[4];
  xiiInt32 b[4];
  Store<4>(a);
  v.Store<4>(b);

  for (xiiUInt32 i = 0; i < 4; ++i)
  {
    a[i] = a[i] / b[i];
  }

  xiiSimdVec4i r;
  r.Load<4>(a);
  return r;
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator|(const xiiSimdVec4i& v) const
{
  return vorrq_s32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator&(const xiiSimdVec4i& v) const
{
  return vandq_s32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator^(const xiiSimdVec4i& v) const
{
  return veorq_s32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator~() const
{
  return vmvnq_s32(m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator<<(xiiUInt32 uiShift) const
{
  return vshlq_s32(m_v, vmovq_n_s32(uiShift));
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator>>(xiiUInt32 uiShift) const
{
  return vshlq_s32(m_v, vmovq_n_s32(-uiShift));
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator<<(const xiiSimdVec4i& v) const
{
  return vshlq_s32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator>>(const xiiSimdVec4i& v) const
{
  return vshlq_s32(m_v, vnegq_s32(v.m_v));
}

XII_ALWAYS_INLINE xiiSimdVec4i& xiiSimdVec4i::operator+=(const xiiSimdVec4i& v)
{
  m_v = vaddq_s32(m_v, v.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4i& xiiSimdVec4i::operator-=(const xiiSimdVec4i& v)
{
  m_v = vsubq_s32(m_v, v.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4i& xiiSimdVec4i::operator|=(const xiiSimdVec4i& v)
{
  m_v = vorrq_s32(m_v, v.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4i& xiiSimdVec4i::operator&=(const xiiSimdVec4i& v)
{
  m_v = vandq_s32(m_v, v.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4i& xiiSimdVec4i::operator^=(const xiiSimdVec4i& v)
{
  m_v = veorq_s32(m_v, v.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4i& xiiSimdVec4i::operator<<=(xiiUInt32 uiShift)
{
  m_v = vshlq_s32(m_v, vmovq_n_s32(uiShift));
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4i& xiiSimdVec4i::operator>>=(xiiUInt32 uiShift)
{
  m_v = vshlq_s32(m_v, vmovq_n_s32(-uiShift));
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::CompMin(const xiiSimdVec4i& v) const
{
  return vminq_s32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::CompMax(const xiiSimdVec4i& v) const
{
  return vmaxq_s32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::Abs() const
{
  return vabsq_s32(m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4i::operator==(const xiiSimdVec4i& v) const
{
  return vceqq_s32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4i::operator!=(const xiiSimdVec4i& v) const
{
  return vmvnq_u32(vceqq_s32(m_v, v.m_v));
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4i::operator<=(const xiiSimdVec4i& v) const
{
  return vcleq_s32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4i::operator<(const xiiSimdVec4i& v) const
{
  return vcltq_s32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4i::operator>=(const xiiSimdVec4i& v) const
{
  return vcgeq_s32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4i::operator>(const xiiSimdVec4i& v) const
{
  return vcgtq_s32(m_v, v.m_v);
}

// static
XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::Select(const xiiSimdVec4b& vCmp, const xiiSimdVec4i& vTrue, const xiiSimdVec4i& vFalse)
{
  return vbslq_s32(vCmp.m_v, vTrue.m_v, vFalse.m_v);
}

// not needed atm
#if 0
void xiiSimdVec4i::Transpose(xiiSimdVec4i& v0, xiiSimdVec4i& v1, xiiSimdVec4i& v2, xiiSimdVec4i& v3)
{
  int32x4x2_t P0 = vzipq_s32(v0.m_v, v2.m_v);
  int32x4x2_t P1 = vzipq_s32(v1.m_v, v3.m_v);

  int32x4x2_t T0 = vzipq_s32(P0.val[0], P1.val[0]);
  int32x4x2_t T1 = vzipq_s32(P0.val[1], P1.val[1]);

  v0.m_v = T0.val[0];
  v1.m_v = T0.val[1];
  v2.m_v = T1.val[0];
  v3.m_v = T1.val[1];
}
#endif
