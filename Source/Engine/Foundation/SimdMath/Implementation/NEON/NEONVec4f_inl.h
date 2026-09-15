/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

XII_ALWAYS_INLINE xiiSimdVec4f::xiiSimdVec4f()
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);

#if XII_ENABLED(XII_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  m_v = vmovq_n_f32(xiiMath::NaN<float>());
#endif
}

XII_ALWAYS_INLINE xiiSimdVec4f::xiiSimdVec4f(float xyzw)
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);

  m_v = vmovq_n_f32(xyzw);
}

XII_ALWAYS_INLINE xiiSimdVec4f::xiiSimdVec4f(const xiiSimdFloat& xyzw)
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);

  m_v = xyzw.m_v;
}

XII_ALWAYS_INLINE xiiSimdVec4f::xiiSimdVec4f(float x, float y, float z, float w)
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);

  alignas(16) float values[4] = {x, y, z, w};
  m_v                         = vld1q_f32(values);
}

XII_ALWAYS_INLINE void xiiSimdVec4f::Set(float xyzw)
{
  m_v = vmovq_n_f32(xyzw);
}

XII_ALWAYS_INLINE void xiiSimdVec4f::Set(float x, float y, float z, float w)
{
  alignas(16) float values[4] = {x, y, z, w};
  m_v                         = vld1q_f32(values);
}

XII_ALWAYS_INLINE void xiiSimdVec4f::SetX(const xiiSimdFloat& f)
{
  m_v = vsetq_lane_f32(f, m_v, 0);
}

XII_ALWAYS_INLINE void xiiSimdVec4f::SetY(const xiiSimdFloat& f)
{
  m_v = vsetq_lane_f32(f, m_v, 1);
}

XII_ALWAYS_INLINE void xiiSimdVec4f::SetZ(const xiiSimdFloat& f)
{
  m_v = vsetq_lane_f32(f, m_v, 2);
}

XII_ALWAYS_INLINE void xiiSimdVec4f::SetW(const xiiSimdFloat& f)
{
  m_v = vsetq_lane_f32(f, m_v, 3);
}

XII_ALWAYS_INLINE void xiiSimdVec4f::SetZero()
{
  m_v = vmovq_n_f32(0.0f);
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4f::Load<1>(const float* pFloat)
{
  m_v = vld1q_lane_f32(pFloat, vmovq_n_f32(0.0f), 0);
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4f::Load<2>(const float* pFloat)
{
  m_v = vreinterpretq_f32_f64(vld1q_lane_f64(reinterpret_cast<const float64_t*>(pFloat), vmovq_n_f64(0.0), 0));
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4f::Load<3>(const float* pFloat)
{
  m_v = vcombine_f32(vld1_f32(pFloat), vld1_lane_f32(pFloat + 2, vmov_n_f32(0.0f), 0));
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4f::Load<4>(const float* pFloat)
{
  m_v = vld1q_f32(pFloat);
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4f::Store<1>(float* pFloat) const
{
  vst1q_lane_f32(pFloat, m_v, 0);
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4f::Store<2>(float* pFloat) const
{
  vst1q_lane_f64(reinterpret_cast<float64_t*>(pFloat), vreinterpretq_f64_f32(m_v), 0);
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4f::Store<3>(float* pFloat) const
{
  vst1q_lane_f64(reinterpret_cast<float64_t*>(pFloat), vreinterpretq_f64_f32(m_v), 0);
  vst1q_lane_f32(pFloat + 2, m_v, 2);
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4f::Store<4>(float* pFloat) const
{
  vst1q_f32(pFloat, m_v);
}

template <>
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::GetReciprocal<xiiMathFloatBits::BITS_12>() const
{
  float32x4_t x0 = vrecpeq_f32(m_v);

  // One iteration of Newton-Raphson
  float32x4_t x1 = vmulq_f32(vrecpsq_f32(m_v, x0), x0);

  return x1;
}

template <>
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::GetReciprocal<xiiMathFloatBits::BITS_23>() const
{
  float32x4_t x0 = vrecpeq_f32(m_v);

  // Two iterations of Newton-Raphson
  float32x4_t x1 = vmulq_f32(vrecpsq_f32(m_v, x0), x0);
  float32x4_t x2 = vmulq_f32(vrecpsq_f32(m_v, x1), x1);

  return x2;
}

template <>
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::GetReciprocal<xiiMathFloatBits::FULL>() const
{
  return vdivq_f32(vmovq_n_f32(1.0f), m_v);
}

template <>
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::GetInvSqrt<xiiMathFloatBits::FULL>() const
{
  return vdivq_f32(vmovq_n_f32(1.0f), vsqrtq_f32(m_v));
}

template <>
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::GetInvSqrt<xiiMathFloatBits::BITS_23>() const
{
  const float32x4_t x0 = vrsqrteq_f32(m_v);

  // Two iterations of Newton-Raphson
  const float32x4_t x1 = vmulq_f32(vrsqrtsq_f32(vmulq_f32(x0, m_v), x0), x0);
  return vmulq_f32(vrsqrtsq_f32(vmulq_f32(x1, m_v), x1), x1);
}

template <>
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::GetInvSqrt<xiiMathFloatBits::BITS_12>() const
{
  const float32x4_t x0 = vrsqrteq_f32(m_v);

  // One iteration of Newton-Raphson
  return vmulq_f32(vrsqrtsq_f32(vmulq_f32(x0, m_v), x0), x0);
}

template <>
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::GetSqrt<xiiMathFloatBits::BITS_12>() const
{
  return CompMul(GetInvSqrt<xiiMathFloatBits::BITS_12>());
}

template <>
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::GetSqrt<xiiMathFloatBits::BITS_23>() const
{
  return CompMul(GetInvSqrt<xiiMathFloatBits::BITS_23>());
}

template <>
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::GetSqrt<xiiMathFloatBits::FULL>() const
{
  return vsqrtq_f32(m_v);
}

template <xiiInt32 N, xiiMathFloatBits::Enum acc>
void xiiSimdVec4f::NormalizeIfNotZero(const xiiSimdFloat& fEpsilon)
{
  xiiSimdFloat sqLength  = GetLengthSquared<N>();
  uint32x4_t   isNotZero = vcgtq_f32(sqLength.m_v, fEpsilon.m_v);
  m_v                    = vmulq_f32(m_v, sqLength.GetInvSqrt<acc>().m_v);
  m_v                    = vreinterpretq_f32_u32(vandq_u32(isNotZero, vreinterpretq_u32_f32(m_v)));
}

template <xiiInt32 N>
XII_ALWAYS_INLINE bool xiiSimdVec4f::IsZero() const
{
  const xiiInt32 mask = XII_BIT(N) - 1;
  return (xiiInternal::NeonMoveMask(vceqzq_f32(m_v)) & mask) == mask;
}

template <xiiInt32 N>
XII_ALWAYS_INLINE bool xiiSimdVec4f::IsZero(const xiiSimdFloat& fEpsilon) const
{
  const xiiInt32 mask   = XII_BIT(N) - 1;
  float32x4_t    absVal = Abs().m_v;
  return (xiiInternal::NeonMoveMask(vcltq_f32(absVal, fEpsilon.m_v)) & mask) == mask;
}

template <xiiInt32 N>
inline bool xiiSimdVec4f::IsNaN() const
{
  const xiiInt32 mask = XII_BIT(N) - 1;
  return (xiiInternal::NeonMoveMask(vceqq_f32(m_v, m_v)) & mask) != mask;
}

template <xiiInt32 N>
XII_ALWAYS_INLINE bool xiiSimdVec4f::IsValid() const
{
  // Check the 8 exponent bits.
  // NAN -> (exponent = all 1, mantissa = non-zero)
  // INF -> (exponent = all 1, mantissa = zero)

  uint32x4_t uiExponentMask = vmovq_n_u32(0x7f800000);
  uint32x4_t uiExponentIs1  = vceqq_u32(vandq_u32(vreinterpretq_u32_f32(m_v), uiExponentMask), uiExponentMask);

  const xiiInt32 uiMask = XII_BIT(N) - 1;
  return (xiiInternal::NeonMoveMask(uiExponentIs1) & uiMask) == 0;
}

template <xiiInt32 N>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::GetComponent() const
{
  return vdupq_laneq_f32(m_v, N);
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::x() const
{
  return GetComponent<0>();
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::y() const
{
  return GetComponent<1>();
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::z() const
{
  return GetComponent<2>();
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::w() const
{
  return GetComponent<3>();
}

template <xiiSwizzle::Enum s>
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::Get() const
{
  return __builtin_shufflevector(m_v, m_v, XII_TO_SHUFFLE(s));
}

template <xiiSwizzle::Enum s>
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::GetCombined(const xiiSimdVec4f& other) const
{
  return __builtin_shufflevector(m_v, other.m_v, XII_TO_SHUFFLE(s));
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::operator-() const
{
  return vnegq_f32(m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::operator+(const xiiSimdVec4f& v) const
{
  return vaddq_f32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::operator-(const xiiSimdVec4f& v) const
{
  return vsubq_f32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::operator*(const xiiSimdFloat& f) const
{
  return vmulq_f32(m_v, f.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::operator/(const xiiSimdFloat& f) const
{
  return vdivq_f32(m_v, f.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::CompMul(const xiiSimdVec4f& v) const
{
  return vmulq_f32(m_v, v.m_v);
}

template <>
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::CompDiv<xiiMathFloatBits::FULL>(const xiiSimdVec4f& v) const
{
  return vdivq_f32(m_v, v.m_v);
}

template <>
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::CompDiv<xiiMathFloatBits::BITS_23>(const xiiSimdVec4f& v) const
{
  return CompMul(v.GetReciprocal<xiiMathFloatBits::BITS_23>());
}

template <>
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::CompDiv<xiiMathFloatBits::BITS_12>(const xiiSimdVec4f& v) const
{
  return CompMul(v.GetReciprocal<xiiMathFloatBits::BITS_12>());
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::CompMin(const xiiSimdVec4f& v) const
{
  return vminq_f32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::CompMax(const xiiSimdVec4f& v) const
{
  return vmaxq_f32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::Abs() const
{
  return vabsq_f32(m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::Round() const
{
  return vrndnq_f32(m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::Floor() const
{
  return vrndmq_f32(m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::Ceil() const
{
  return vrndpq_f32(m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::Trunc() const
{
  return vrndq_f32(m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::FlipSign(const xiiSimdVec4b& cmp) const
{
  return vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(m_v), vshlq_n_u32(cmp.m_v, 31)));
}

// static
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::Select(const xiiSimdVec4b& cmp, const xiiSimdVec4f& ifTrue, const xiiSimdVec4f& ifFalse)
{
  return vbslq_f32(cmp.m_v, ifTrue.m_v, ifFalse.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4f& xiiSimdVec4f::operator+=(const xiiSimdVec4f& v)
{
  m_v = vaddq_f32(m_v, v.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4f& xiiSimdVec4f::operator-=(const xiiSimdVec4f& v)
{
  m_v = vsubq_f32(m_v, v.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4f& xiiSimdVec4f::operator*=(const xiiSimdFloat& f)
{
  m_v = vmulq_f32(m_v, f.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4f& xiiSimdVec4f::operator/=(const xiiSimdFloat& f)
{
  m_v = vdivq_f32(m_v, f.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4f::operator==(const xiiSimdVec4f& v) const
{
  return vceqq_f32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4f::operator!=(const xiiSimdVec4f& v) const
{
  return vmvnq_u32(vceqq_f32(m_v, v.m_v));
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4f::operator<=(const xiiSimdVec4f& v) const
{
  return vcleq_f32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4f::operator<(const xiiSimdVec4f& v) const
{
  return vcltq_f32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4f::operator>=(const xiiSimdVec4f& v) const
{
  return vcgeq_f32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4f::operator>(const xiiSimdVec4f& v) const
{
  return vcgtq_f32(m_v, v.m_v);
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::HorizontalSum<2>() const
{
  return vpadds_f32(vget_low_f32(m_v));
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::HorizontalSum<3>() const
{
  return HorizontalSum<2>() + GetComponent<2>();
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::HorizontalSum<4>() const
{
  float32x2_t x0 = vpadd_f32(vget_low_f32(m_v), vget_high_f32(m_v));
  return vpadds_f32(x0);
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::HorizontalMin<2>() const
{
  return vpmins_f32(vget_low_f32(m_v));
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::HorizontalMin<3>() const
{
  return vminq_f32(vmovq_n_f32(vpmins_f32(vget_low_f32(m_v))), vdupq_laneq_f32(m_v, 2));
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::HorizontalMin<4>() const
{
  return vpmins_f32(vpmin_f32(vget_low_f32(m_v), vget_high_f32(m_v)));
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::HorizontalMax<2>() const
{
  return vpmaxs_f32(vget_low_f32(m_v));
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::HorizontalMax<3>() const
{
  return vmaxq_f32(vmovq_n_f32(vpmaxs_f32(vget_low_f32(m_v))), vdupq_laneq_f32(m_v, 2));
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::HorizontalMax<4>() const
{
  return vpmaxs_f32(vpmax_f32(vget_low_f32(m_v), vget_high_f32(m_v)));
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::Dot<1>(const xiiSimdVec4f& v) const
{
  return vdupq_laneq_f32(vmulq_f32(m_v, v.m_v), 0);
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::Dot<2>(const xiiSimdVec4f& v) const
{
  return vpadds_f32(vmul_f32(vget_low_f32(m_v), vget_low_f32(v.m_v)));
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::Dot<3>(const xiiSimdVec4f& v) const
{
  return CompMul(v).HorizontalSum<3>();
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdVec4f::Dot<4>(const xiiSimdVec4f& v) const
{
  return CompMul(v).HorizontalSum<4>();
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::CrossRH(const xiiSimdVec4f& v) const
{
  float32x4_t a = vmulq_f32(m_v, __builtin_shufflevector(v.m_v, v.m_v, XII_TO_SHUFFLE(xiiSwizzle::YZXW)));
  float32x4_t b = vmulq_f32(v.m_v, __builtin_shufflevector(m_v, m_v, XII_TO_SHUFFLE(xiiSwizzle::YZXW)));
  float32x4_t c = vsubq_f32(a, b);

  return __builtin_shufflevector(c, c, XII_TO_SHUFFLE(xiiSwizzle::YZXW));
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::GetOrthogonalVector() const
{
  // See http://blog.selfshadow.com/2011/10/17/perp-vectors/ - this is Stark's first variant, SIMDified.
  return CrossRH(vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(m_v), vceqq_f32(m_v, HorizontalMin<3>().m_v))));
}

// static
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::MulAdd(const xiiSimdVec4f& a, const xiiSimdVec4f& b, const xiiSimdVec4f& c)
{
  return vfmaq_f32(c.m_v, a.m_v, b.m_v);
}

// static
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::MulAdd(const xiiSimdVec4f& a, const xiiSimdFloat& b, const xiiSimdVec4f& c)
{
  return vfmaq_f32(c.m_v, a.m_v, b.m_v);
}

// static
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::MulSub(const xiiSimdVec4f& a, const xiiSimdVec4f& b, const xiiSimdVec4f& c)
{
  return vnegq_f32(vfmsq_f32(c.m_v, a.m_v, b.m_v));
}

// static
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::MulSub(const xiiSimdVec4f& a, const xiiSimdFloat& b, const xiiSimdVec4f& c)
{
  return vnegq_f32(vfmsq_f32(c.m_v, a.m_v, b.m_v));
}

// static
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4f::CopySign(const xiiSimdVec4f& magnitude, const xiiSimdVec4f& sign)
{
  return vbslq_f32(vmovq_n_u32(0x80000000), sign.m_v, magnitude.m_v);
}
