#pragma once

XII_ALWAYS_INLINE xiiSimdDouble::xiiSimdDouble()
{
  XII_CHECK_SIMD_DOUBLE_ALIGNMENT(this);

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  m_v = _mm256_set1_pd(xiiMath::NaN<double>());
#endif
}

XII_ALWAYS_INLINE xiiSimdDouble::xiiSimdDouble(double f)
{
  XII_CHECK_SIMD_DOUBLE_ALIGNMENT(this);

  m_v = _mm256_set1_pd(f);
}

XII_ALWAYS_INLINE xiiSimdDouble::xiiSimdDouble(float f)
{
  XII_CHECK_SIMD_DOUBLE_ALIGNMENT(this);

  double fValue = static_cast<double>(f);

  m_v = _mm256_set1_pd(fValue);
}

XII_ALWAYS_INLINE xiiSimdDouble::xiiSimdDouble(xiiInt32 i)
{
  XII_CHECK_SIMD_DOUBLE_ALIGNMENT(this);

  __m128 v = _mm_cvtsi32_ss(_mm_setzero_ps(), i);
  m_v      = _mm256_cvtps_pd(_mm_shuffle_ps(v, v, XII_TO_SHUFFLE(xiiSwizzle::XXXX)));
}

XII_ALWAYS_INLINE xiiSimdDouble::xiiSimdDouble(xiiUInt32 i)
{
  XII_CHECK_SIMD_DOUBLE_ALIGNMENT(this);

#if XII_ENABLED(XII_PLATFORM_64BIT)
  __m128 v = _mm_cvtsi64_ss(_mm_setzero_ps(), i);
#else
  __m128 v = _mm_cvtsi32_ss(_mm_setzero_ps(), i);
#endif
  m_v = _mm256_cvtps_pd(_mm_shuffle_ps(v, v, XII_TO_SHUFFLE(xiiSwizzle::XXXX)));
}

XII_ALWAYS_INLINE xiiSimdDouble::xiiSimdDouble(xiiAngled a)
{
  XII_CHECK_SIMD_DOUBLE_ALIGNMENT(this);

  m_v = _mm256_set1_pd(a.GetRadian());
}

XII_ALWAYS_INLINE xiiSimdDouble::xiiSimdDouble(xiiInternal::QuadDouble v)
{
  m_v = v;
}

XII_ALWAYS_INLINE xiiSimdDouble::operator double() const
{
  double f[4];
  _mm256_store_pd(f, m_v);
  return f[0];
}

// static
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdDouble::Zero()
{
  return _mm256_setzero_pd();
}

XII_ALWAYS_INLINE xiiSimdDouble xiiSimdDouble::operator+(const xiiSimdDouble& f) const
{
  return _mm256_add_pd(m_v, f.m_v);
}

XII_ALWAYS_INLINE xiiSimdDouble xiiSimdDouble::operator-(const xiiSimdDouble& f) const
{
  return _mm256_sub_pd(m_v, f.m_v);
}

XII_ALWAYS_INLINE xiiSimdDouble xiiSimdDouble::operator*(const xiiSimdDouble& f) const
{
  return _mm256_mul_pd(m_v, f.m_v);
}

XII_ALWAYS_INLINE xiiSimdDouble xiiSimdDouble::operator/(const xiiSimdDouble& f) const
{
  return _mm256_div_pd(m_v, f.m_v);
}

XII_ALWAYS_INLINE xiiSimdDouble& xiiSimdDouble::operator+=(const xiiSimdDouble& f)
{
  m_v = _mm256_add_pd(m_v, f.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdDouble& xiiSimdDouble::operator-=(const xiiSimdDouble& f)
{
  m_v = _mm256_sub_pd(m_v, f.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdDouble& xiiSimdDouble::operator*=(const xiiSimdDouble& f)
{
  m_v = _mm256_mul_pd(m_v, f.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdDouble& xiiSimdDouble::operator/=(const xiiSimdDouble& f)
{
  m_v = _mm256_div_pd(m_v, f.m_v);
  return *this;
}

XII_ALWAYS_INLINE bool xiiSimdDouble::IsEqual(const xiiSimdDouble& rhs, const xiiSimdDouble& epsilon) const
{
  xiiSimdDouble minusEps = rhs - epsilon;
  xiiSimdDouble plusEps  = rhs + epsilon;
  return ((*this >= minusEps) && (*this <= plusEps));
}

XII_ALWAYS_INLINE bool xiiSimdDouble::operator==(const xiiSimdDouble& f) const
{
  return _mm256_movemask_pd(_mm256_cmp_pd(m_v, f.m_v, _CMP_EQ_OQ)) > 0; // Ordered, Non Signaling
}

XII_ALWAYS_INLINE bool xiiSimdDouble::operator!=(const xiiSimdDouble& f) const
{
  return _mm256_movemask_pd(_mm256_cmp_pd(m_v, f.m_v, _CMP_NEQ_OQ)) > 0; // Ordered, Non Signaling
}

XII_ALWAYS_INLINE bool xiiSimdDouble::operator>=(const xiiSimdDouble& f) const
{
  return _mm256_movemask_pd(_mm256_cmp_pd(m_v, f.m_v, _CMP_GE_OQ)) > 0; // Ordered, Non Signaling
}

XII_ALWAYS_INLINE bool xiiSimdDouble::operator>(const xiiSimdDouble& f) const
{
  return _mm256_movemask_pd(_mm256_cmp_pd(m_v, f.m_v, _CMP_GT_OQ)) > 0; // Ordered, Non Signaling
}

XII_ALWAYS_INLINE bool xiiSimdDouble::operator<=(const xiiSimdDouble& f) const
{
  return _mm256_movemask_pd(_mm256_cmp_pd(m_v, f.m_v, _CMP_LE_OQ)) > 0; // Ordered, Non Signaling
}

XII_ALWAYS_INLINE bool xiiSimdDouble::operator<(const xiiSimdDouble& f) const
{
  return _mm256_movemask_pd(_mm256_cmp_pd(m_v, f.m_v, _CMP_LT_OQ)) > 0; // Ordered, Non Signaling
}

XII_ALWAYS_INLINE bool xiiSimdDouble::operator==(double f) const
{
  return (*this) == xiiSimdDouble(f);
}

XII_ALWAYS_INLINE bool xiiSimdDouble::operator!=(double f) const
{
  return (*this) != xiiSimdDouble(f);
}

XII_ALWAYS_INLINE bool xiiSimdDouble::operator>(double f) const
{
  return (*this) > xiiSimdDouble(f);
}

XII_ALWAYS_INLINE bool xiiSimdDouble::operator>=(double f) const
{
  return (*this) >= xiiSimdDouble(f);
}

XII_ALWAYS_INLINE bool xiiSimdDouble::operator<(double f) const
{
  return (*this) < xiiSimdDouble(f);
}

XII_ALWAYS_INLINE bool xiiSimdDouble::operator<=(double f) const
{
  return (*this) <= xiiSimdDouble(f);
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdDouble::GetReciprocal<xiiMathDoubleBits::FULL>() const
{
  return _mm256_div_pd(_mm256_set1_pd(1.0), m_v);
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdDouble::GetReciprocal<xiiMathDoubleBits::BITS_27>() const
{
#if XII_SSE_LEVEL >= XII_SSE_AVX512
  __m256d x0 = _mm256_rcp14_pd(m_v);

  // One iteration of Newton-Raphson.
  __m256d x1 = _mm256_mul_pd(x0, _mm256_sub_pd(_mm256_set1_pd(2.0), _mm256_mul_pd(m_v, x0)));

  return x1;
#elif XII_SSE_LEVEL >= XII_SSE_AVX
  // No SIMD approximation available.
  return _mm256_div_pd(_mm256_set1_pd(1.0), m_v);
#else
  XII_ASSERT_NOT_IMPLEMENTED;

  return xiiSimdDouble(xiiMath::NaN<double>());
#endif
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdDouble::GetReciprocal<xiiMathDoubleBits::BITS_14>() const
{
#if XII_SSE_LEVEL >= XII_SSE_AVX512
  return _mm256_rcp14_pd(m_v);
#elif XII_SSE_LEVEL >= XII_SSE_AVX
  // No SIMD approximation available.
  return _mm256_div_pd(_mm256_set1_pd(1.0), m_v);
#else
  XII_ASSERT_NOT_IMPLEMENTED;

  return xiiSimdDouble(xiiMath::NaN<double>());
#endif
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdDouble::GetInvSqrt<xiiMathDoubleBits::FULL>() const
{
  return _mm256_div_pd(_mm256_set1_pd(1.0), _mm256_sqrt_pd(m_v));
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdDouble::GetInvSqrt<xiiMathDoubleBits::BITS_27>() const
{
#if XII_SSE_LEVEL >= XII_SSE_AVX512
  const __m256d x0 = _mm256_mask_rsqrt14_pd(m_v, 0xF, m_v);

  // One iteration of Newton-Raphson.
  return _mm256_mul_pd(_mm256_mul_pd(_mm256_set1_pd(0.5), x0), _mm256_sub_pd(_mm256_set1_pd(3.0), _mm256_mul_pd(_mm256_mul_pd(m_v, x0), x0)));
#elif XII_SSE_LEVEL >= XII_SSE_AVX
  // No SIMD approximation available.
  return _mm256_div_pd(_mm256_set1_pd(1.0), _mm256_sqrt_pd(m_v));
#else
  XII_ASSERT_NOT_IMPLEMENTED;

  return xiiSimdDouble(xiiMath::NaN<double>());
#endif
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdDouble::GetInvSqrt<xiiMathDoubleBits::BITS_14>() const
{
#if XII_SSE_LEVEL >= XII_SSE_AVX512
  return _mm256_mask_rsqrt14_pd(m_v, 0xF, m_v);
#elif XII_SSE_LEVEL >= XII_SSE_AVX
  // No SIMD approximation available.
  return _mm256_div_pd(_mm256_set1_pd(1.0), _mm256_sqrt_pd(m_v));
#else
  XII_ASSERT_NOT_IMPLEMENTED;

  return xiiSimdDouble(xiiMath::NaN<double>());
#endif
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdDouble::GetSqrt<xiiMathDoubleBits::FULL>() const
{
  return _mm256_sqrt_pd(m_v);
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdDouble::GetSqrt<xiiMathDoubleBits::BITS_27>() const
{
  return (*this) * GetInvSqrt<xiiMathDoubleBits::BITS_27>();
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdDouble::GetSqrt<xiiMathDoubleBits::BITS_14>() const
{
  return (*this) * GetInvSqrt<xiiMathDoubleBits::BITS_14>();
}

XII_ALWAYS_INLINE xiiSimdDouble xiiSimdDouble::Max(const xiiSimdDouble& f) const
{
  return _mm256_max_pd(m_v, f.m_v);
}

XII_ALWAYS_INLINE xiiSimdDouble xiiSimdDouble::Min(const xiiSimdDouble& f) const
{
  return _mm256_min_pd(m_v, f.m_v);
}

XII_ALWAYS_INLINE xiiSimdDouble xiiSimdDouble::Abs() const
{
  return _mm256_andnot_pd(_mm256_set1_pd(-0.0), m_v);
}
