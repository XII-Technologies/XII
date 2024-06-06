#pragma once

XII_ALWAYS_INLINE xiiSimdFloat::xiiSimdFloat()
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);

#if XII_ENABLED(XII_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  m_v = _mm_set1_ps(xiiMath::NaN<float>());
#endif
}

XII_ALWAYS_INLINE xiiSimdFloat::xiiSimdFloat(float f)
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);

  m_v = _mm_set1_ps(f);
}

XII_ALWAYS_INLINE xiiSimdFloat::xiiSimdFloat(xiiInt32 i)
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);

  __m128 v = _mm_cvtsi32_ss(_mm_setzero_ps(), i);
  m_v      = _mm_shuffle_ps(v, v, XII_TO_SHUFFLE(xiiSwizzle::XXXX));
}

XII_ALWAYS_INLINE xiiSimdFloat::xiiSimdFloat(xiiUInt32 i)
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);

#if XII_ENABLED(XII_PLATFORM_64BIT)
  __m128 v = _mm_cvtsi64_ss(_mm_setzero_ps(), i);
#else
  __m128 v = _mm_cvtsi32_ss(_mm_setzero_ps(), i);
#endif
  m_v = _mm_shuffle_ps(v, v, XII_TO_SHUFFLE(xiiSwizzle::XXXX));
}

XII_ALWAYS_INLINE xiiSimdFloat::xiiSimdFloat(xiiAngle a)
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);

  m_v = _mm_set1_ps(a.GetRadian());
}

XII_ALWAYS_INLINE xiiSimdFloat::xiiSimdFloat(xiiInternal::QuadFloat v)
{
  m_v = v;
}

XII_ALWAYS_INLINE xiiSimdFloat::operator float() const
{
  float f;
  _mm_store_ss(&f, m_v);
  return f;
}

// static
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::MakeZero()
{
  return _mm_setzero_ps();
}

// static
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::MakeNaN()
{
  return _mm_set1_ps(xiiMath::NaN<float>());
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::operator+(const xiiSimdFloat& f) const
{
  return _mm_add_ps(m_v, f.m_v);
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::operator-(const xiiSimdFloat& f) const
{
  return _mm_sub_ps(m_v, f.m_v);
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::operator*(const xiiSimdFloat& f) const
{
  return _mm_mul_ps(m_v, f.m_v);
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::operator/(const xiiSimdFloat& f) const
{
  return _mm_div_ps(m_v, f.m_v);
}

XII_ALWAYS_INLINE xiiSimdFloat& xiiSimdFloat::operator+=(const xiiSimdFloat& f)
{
  m_v = _mm_add_ps(m_v, f.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdFloat& xiiSimdFloat::operator-=(const xiiSimdFloat& f)
{
  m_v = _mm_sub_ps(m_v, f.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdFloat& xiiSimdFloat::operator*=(const xiiSimdFloat& f)
{
  m_v = _mm_mul_ps(m_v, f.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdFloat& xiiSimdFloat::operator/=(const xiiSimdFloat& f)
{
  m_v = _mm_div_ps(m_v, f.m_v);
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
  return _mm_comieq_ss(m_v, f.m_v) == 1;
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator!=(const xiiSimdFloat& f) const
{
  return _mm_comineq_ss(m_v, f.m_v) == 1;
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator>=(const xiiSimdFloat& f) const
{
  return _mm_comige_ss(m_v, f.m_v) == 1;
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator>(const xiiSimdFloat& f) const
{
  return _mm_comigt_ss(m_v, f.m_v) == 1;
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator<=(const xiiSimdFloat& f) const
{
  return _mm_comile_ss(m_v, f.m_v) == 1;
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator<(const xiiSimdFloat& f) const
{
  return _mm_comilt_ss(m_v, f.m_v) == 1;
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
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::GetReciprocal<xiiMathFloatBits::FULL>() const
{
  return _mm_div_ps(_mm_set1_ps(1.0f), m_v);
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::GetReciprocal<xiiMathFloatBits::BITS_23>() const
{
  __m128 x0 = _mm_rcp_ps(m_v);

  // One iteration of Newton-Raphson
  __m128 x1 = _mm_mul_ps(x0, _mm_sub_ps(_mm_set1_ps(2.0f), _mm_mul_ps(m_v, x0)));

  return x1;
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::GetReciprocal<xiiMathFloatBits::BITS_12>() const
{
  return _mm_rcp_ps(m_v);
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::GetInvSqrt<xiiMathFloatBits::FULL>() const
{
  return _mm_div_ps(_mm_set1_ps(1.0f), _mm_sqrt_ps(m_v));
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::GetInvSqrt<xiiMathFloatBits::BITS_23>() const
{
  const __m128 x0 = _mm_rsqrt_ps(m_v);

  // One iteration of Newton-Raphson
  return _mm_mul_ps(_mm_mul_ps(_mm_set1_ps(0.5f), x0), _mm_sub_ps(_mm_set1_ps(3.0f), _mm_mul_ps(_mm_mul_ps(m_v, x0), x0)));
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::GetInvSqrt<xiiMathFloatBits::BITS_12>() const
{
  return _mm_rsqrt_ps(m_v);
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::GetSqrt<xiiMathFloatBits::FULL>() const
{
  return _mm_sqrt_ps(m_v);
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::GetSqrt<xiiMathFloatBits::BITS_23>() const
{
  return (*this) * GetInvSqrt<xiiMathFloatBits::BITS_23>();
}

template <>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::GetSqrt<xiiMathFloatBits::BITS_12>() const
{
  return (*this) * GetInvSqrt<xiiMathFloatBits::BITS_12>();
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::Max(const xiiSimdFloat& f) const
{
  return _mm_max_ps(m_v, f.m_v);
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::Min(const xiiSimdFloat& f) const
{
  return _mm_min_ps(m_v, f.m_v);
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::Abs() const
{
  return _mm_andnot_ps(_mm_set1_ps(-0.0f), m_v);
}
