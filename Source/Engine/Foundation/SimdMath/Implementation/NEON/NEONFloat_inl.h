#pragma once

XII_ALWAYS_INLINE xiiSimdFloat::xiiSimdFloat()
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  m_v = vmovq_n_f32(xiiMath::NaN<float>());
#endif
}

XII_ALWAYS_INLINE xiiSimdFloat::xiiSimdFloat(float f)
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);

  m_v = vmovq_n_f32(f);
}

XII_ALWAYS_INLINE xiiSimdFloat::xiiSimdFloat(xiiInt32 i)
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);

  m_v = vcvtq_f32_s32(vmovq_n_s32(i));
}

XII_ALWAYS_INLINE xiiSimdFloat::xiiSimdFloat(xiiUInt32 i)
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);

  m_v = vcvtq_f32_u32(vmovq_n_u32(i));
}

XII_ALWAYS_INLINE xiiSimdFloat::xiiSimdFloat(xiiAngle a)
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);

  m_v = vmovq_n_f32(a.GetRadian());
}

XII_ALWAYS_INLINE xiiSimdFloat::xiiSimdFloat(xiiInternal::QuadFloat v)
{
  m_v = v;
}

XII_ALWAYS_INLINE xiiSimdFloat::operator float() const
{
  return vgetq_lane_f32(m_v, 0);
}

// static
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::Zero()
{
  return vmovq_n_f32(0.0f);
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::operator+(const xiiSimdFloat& f) const
{
  return vaddq_f32(m_v, f.m_v);
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::operator-(const xiiSimdFloat& f) const
{
  return vsubq_f32(m_v, f.m_v);
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::operator*(const xiiSimdFloat& f) const
{
  return vmulq_f32(m_v, f.m_v);
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::operator/(const xiiSimdFloat& f) const
{
  return vdivq_f32(m_v, f.m_v);
}

XII_ALWAYS_INLINE xiiSimdFloat& xiiSimdFloat::operator+=(const xiiSimdFloat& f)
{
  m_v = vaddq_f32(m_v, f.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdFloat& xiiSimdFloat::operator-=(const xiiSimdFloat& f)
{
  m_v = vsubq_f32(m_v, f.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdFloat& xiiSimdFloat::operator*=(const xiiSimdFloat& f)
{
  m_v = vmulq_f32(m_v, f.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdFloat& xiiSimdFloat::operator/=(const xiiSimdFloat& f)
{
  m_v = vdivq_f32(m_v, f.m_v);
  return *this;
}

XII_ALWAYS_INLINE bool xiiSimdFloat::IsEqual(const xiiSimdFloat& rhs, const xiiSimdFloat& fEpsilon) const
{
  xiiSimdFloat minusEps = rhs - fEpsilon;
  xiiSimdFloat plusEps  = rhs + fEpsilon;
  return ((*this >= minusEps) && (*this <= plusEps));
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator==(const xiiSimdFloat& f) const
{
  return vgetq_lane_u32(vceqq_f32(m_v, f.m_v), 0) & 1;
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator!=(const xiiSimdFloat& f) const
{
  return !operator==(f);
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator>=(const xiiSimdFloat& f) const
{
  return vgetq_lane_u32(vcgeq_f32(m_v, f.m_v), 0) & 1;
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator>(const xiiSimdFloat& f) const
{
  return vgetq_lane_u32(vcgtq_f32(m_v, f.m_v), 0) & 1;
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator<=(const xiiSimdFloat& f) const
{
  return vgetq_lane_u32(vcleq_f32(m_v, f.m_v), 0) & 1;
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator<(const xiiSimdFloat& f) const
{
  return vgetq_lane_u32(vcltq_f32(m_v, f.m_v), 0) & 1;
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator==(float f) const
{
  return (*this) == xiiSimdFloat(f);
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator!=(float f) const
{
  return (*this) != xiiSimdFloat(f);
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator>(float f) const
{
  return (*this) > xiiSimdFloat(f);
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator>=(float f) const
{
  return (*this) >= xiiSimdFloat(f);
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator<(float f) const
{
  return (*this) < xiiSimdFloat(f);
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator<=(float f) const
{
  return (*this) <= xiiSimdFloat(f);
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::GetReciprocal<xiiMathAcc::FULL>() const
{
  return vdivq_f32(vmovq_n_f32(1.0f), m_v);
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::GetReciprocal<xiiMathAcc::BITS_23>() const
{
  float32x4_t x0 = vrecpeq_f32(m_v);

  // Two iterations of Newton-Raphson
  float32x4_t x1 = vmulq_f32(vrecpsq_f32(m_v, x0), x0);
  float32x4_t x2 = vmulq_f32(vrecpsq_f32(m_v, x1), x1);

  return x2;
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::GetReciprocal<xiiMathAcc::BITS_12>() const
{
  float32x4_t x0 = vrecpeq_f32(m_v);

  // One iteration of Newton-Raphson
  float32x4_t x1 = vmulq_f32(vrecpsq_f32(m_v, x0), x0);

  return x1;
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::GetInvSqrt<xiiMathAcc::FULL>() const
{
  return vdivq_f32(vmovq_n_f32(1.0f), vsqrtq_f32(m_v));
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::GetInvSqrt<xiiMathAcc::BITS_23>() const
{
  const float32x4_t x0 = vrsqrteq_f32(m_v);

  // Two iterations of Newton-Raphson
  const float32x4_t x1 = vmulq_f32(vrsqrtsq_f32(vmulq_f32(x0, m_v), x0), x0);
  return vmulq_f32(vrsqrtsq_f32(vmulq_f32(x1, m_v), x1), x1);
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::GetInvSqrt<xiiMathAcc::BITS_12>() const
{
  const float32x4_t x0 = vrsqrteq_f32(m_v);

  // One iteration of Newton-Raphson
  return vmulq_f32(vrsqrtsq_f32(vmulq_f32(x0, m_v), x0), x0);
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::GetSqrt<xiiMathAcc::FULL>() const
{
  return vsqrtq_f32(m_v);
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::GetSqrt<xiiMathAcc::BITS_23>() const
{
  return (*this) * GetInvSqrt<xiiMathAcc::BITS_23>();
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::GetSqrt<xiiMathAcc::BITS_12>() const
{
  return (*this) * GetInvSqrt<xiiMathAcc::BITS_12>();
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::Max(const xiiSimdFloat& f) const
{
  return vmaxq_f32(m_v, f.m_v);
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::Min(const xiiSimdFloat& f) const
{
  return vminq_f32(m_v, f.m_v);
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::Abs() const
{
  return vabsq_f32(m_v);
}
