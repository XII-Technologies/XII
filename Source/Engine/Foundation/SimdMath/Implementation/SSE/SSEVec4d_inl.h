#pragma once

XII_ALWAYS_INLINE xiiSimdVec4d::xiiSimdVec4d()
{
  XII_CHECK_SIMD_DOUBLE_ALIGNMENT(this);

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  m_v = _mm256_set1_pd(xiiMath::NaN<double>());
#endif
}

XII_ALWAYS_INLINE xiiSimdVec4d::xiiSimdVec4d(double xyzw)
{
  XII_CHECK_SIMD_DOUBLE_ALIGNMENT(this);

  m_v = _mm256_set1_pd(xyzw);
}

XII_ALWAYS_INLINE xiiSimdVec4d::xiiSimdVec4d(const xiiSimdDouble& xyzw)
{
  XII_CHECK_SIMD_DOUBLE_ALIGNMENT(this);

  m_v = xyzw.m_v;
}

XII_ALWAYS_INLINE xiiSimdVec4d::xiiSimdVec4d(double x, double y, double z, double w)
{
  XII_CHECK_SIMD_DOUBLE_ALIGNMENT(this);

  m_v = _mm256_setr_pd(x, y, z, w);
}

XII_ALWAYS_INLINE void xiiSimdVec4d::Set(double xyzw)
{
  m_v = _mm256_set1_pd(xyzw);
}

XII_ALWAYS_INLINE void xiiSimdVec4d::Set(double x, double y, double z, double w)
{
  m_v = _mm256_setr_pd(x, y, z, w);
}

XII_ALWAYS_INLINE void xiiSimdVec4d::SetX(const xiiSimdDouble& f)
{
  m_v = _mm256_blend_pd(m_v, f.m_v, 0x1);
}

XII_ALWAYS_INLINE void xiiSimdVec4d::SetY(const xiiSimdDouble& f)
{
  m_v = _mm256_blend_pd(m_v, f.m_v, 0x2);
}

XII_ALWAYS_INLINE void xiiSimdVec4d::SetZ(const xiiSimdDouble& f)
{
  m_v = _mm256_blend_pd(m_v, f.m_v, 0x4);
}

XII_ALWAYS_INLINE void xiiSimdVec4d::SetW(const xiiSimdDouble& f)
{
  m_v = _mm256_blend_pd(m_v, f.m_v, 0x8);
}

XII_ALWAYS_INLINE void xiiSimdVec4d::SetZero()
{
  m_v = _mm256_setzero_pd();
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4d::Load<1>(const double* pValues)
{
  m_v = _mm256_castpd128_pd256(_mm_load_sd(pValues));
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4d::Load<2>(const double* pValues)
{
  m_v = _mm256_castpd128_pd256(_mm_load_pd(pValues));
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4d::Load<3>(const double* pValues)
{
  m_v = _mm256_maskload_pd(pValues, _mm256_set_epi64x(0, -1, -1, -1));
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4d::Load<4>(const double* pValues)
{
  m_v = _mm256_loadu_pd(pValues);
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4d::Store<1>(double* pValues) const
{
  _mm256_maskstore_pd(pValues, _mm256_set_epi64x(0, 0, 0, -1), m_v);
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4d::Store<2>(double* pValues) const
{
  _mm256_maskstore_pd(pValues, _mm256_set_epi64x(0, 0, -1, -1), m_v);
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4d::Store<3>(double* pValues) const
{
  _mm256_maskstore_pd(pValues, _mm256_set_epi64x(0, -1, -1, -1), m_v);
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4d::Store<4>(double* pValues) const
{
  _mm256_storeu_pd(pValues, m_v);
}

#if XII_SSE_LEVEL >= XII_SSE_AVX512

template <>
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::GetReciprocal<xiiMathDoubleBits::BITS_14>() const
{
  return _mm256_rcp14_pd(m_v);
}

template <>
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::GetReciprocal<xiiMathDoubleBits::BITS_27>() const
{
  __m256d x0 = _mm256_rcp14_pd(m_v);

  // One Newton-Raphson iteration.
  return _mm256_mul_pd(x0, _mm256_sub_pd(_mm256_set1_pd(2.0), _mm256_mul_pd(m_v, x0)));
}

#endif

template <>
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::GetReciprocal<xiiMathDoubleBits::FULL>() const
{
  return _mm256_div_pd(_mm256_set1_pd(1.0), m_v);
}

#if XII_SSE_LEVEL >= XII_SSE_AVX512

template <>
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::GetSqrt<xiiMathDoubleBits::BITS_14>() const
{
  return _mm256_mul_pd(m_v, _mm256_maskz_rsqrt14_pd(0xf, m_v));
}

template <>
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::GetSqrt<xiiMathDoubleBits::BITS_27>() const
{
  __m256d x0 = _mm256_maskz_rsqrt14_pd(0xF, m_v);

  // One Newton-Raphson iteration.
  __m256d x1 = _mm256_mul_pd(_mm256_mul_pd(_mm256_set1_pd(0.5), x0), _mm256_sub_pd(_mm256_set1_pd(3.0), _mm256_mul_pd(_mm256_mul_pd(m_v, x0), x0)));

  return _mm256_mul_pd(m_v, x1);
}

#endif

template <>
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::GetSqrt<xiiMathDoubleBits::FULL>() const
{
  return _mm256_sqrt_pd(m_v);
}

#if XII_SSE_LEVEL >= XII_SSE_AVX512

template <>
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::GetInvSqrt<xiiMathDoubleBits::BITS_14>() const
{
  return _mm256_maskz_rsqrt14_pd(0xF, m_v);
}

template <>
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::GetInvSqrt<xiiMathDoubleBits::BITS_27>() const
{
  __m256d x0 = _mm256_maskz_rsqrt14_pd(0xF, m_v);

  // One Newton-Raphson iteration.
  return _mm256_mul_pd(_mm256_mul_pd(_mm256_set1_pd(0.5), x0), _mm256_sub_pd(_mm256_set1_pd(3.0), _mm256_mul_pd(_mm256_mul_pd(m_v, x0), x0)));
}

#endif

template <>
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::GetInvSqrt<xiiMathDoubleBits::FULL>() const
{
  return _mm256_div_pd(_mm256_set1_pd(1.0), _mm256_sqrt_pd(m_v));
}

template <xiiInt32 N, xiiMathDoubleBits::Enum acc>
void xiiSimdVec4d::NormalizeIfNotZero(const xiiSimdDouble& fEpsilon)
{
  xiiSimdDouble sqLength  = GetLengthSquared<N>();
  __m256d       isNotZero = _mm256_cmp_pd(sqLength.m_v, fEpsilon.m_v, _CMP_GT_OQ);
  m_v                     = _mm256_mul_pd(m_v, sqLength.GetInvSqrt<acc>().m_v);
  m_v                     = _mm256_and_pd(isNotZero, m_v);
}

template <xiiInt32 N>
XII_ALWAYS_INLINE bool xiiSimdVec4d::IsZero() const
{
  const xiiInt32 mask = XII_BIT(N) - 1;
  return (_mm256_movemask_pd(_mm256_cmp_pd(m_v, _mm256_setzero_pd(), _CMP_EQ_OQ)) & mask) == mask;
}

template <xiiInt32 N>
XII_ALWAYS_INLINE bool xiiSimdVec4d::IsZero(const xiiSimdDouble& fEpsilon) const
{
  const xiiInt32 mask   = XII_BIT(N) - 1;
  __m256d        absVal = Abs().m_v;
  return (_mm256_movemask_pd(_mm256_cmp_pd(absVal, fEpsilon.m_v, _CMP_LT_OQ)) & mask) == mask;
}

template <xiiInt32 N>
inline bool xiiSimdVec4d::IsNaN() const
{
  // NAN -> (exponent = all 1, mantissa = non-zero)s
  // Mantissa Mask  ->  XII_BIT(MANTISSA_BITS) - 1  ->   XII_BIT(52) - 1
  // Exponent Mask  ->  MAX_BITS XOR MANTISSA_BTIS  ->  (XII_BIT(63) - 1) ^ ((XII_BIT(52) - 1)

  alignas(32) const xiiUInt64 s_exponentMask[4] = {0x7FF0000000000000ull, 0x7FF0000000000000ull, 0x7FF0000000000000ull, 0x7FF0000000000000ull};
  alignas(32) const xiiUInt64 s_mantissaMask[4] = {0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu};

  __m256d exponentMask = _mm256_load_pd(reinterpret_cast<const double*>(s_exponentMask));
  __m256d mantissaMask = _mm256_load_pd(reinterpret_cast<const double*>(s_mantissaMask));

  __m256d exponentAll1 = _mm256_cmp_pd(_mm256_and_pd(m_v, exponentMask), exponentMask, _CMP_EQ_OQ);
  __m256d mantissaNon0 = _mm256_cmp_pd(_mm256_and_pd(m_v, mantissaMask), _mm256_setzero_pd(), _CMP_NEQ_OQ);

  const xiiInt32 mask = XII_BIT(N) - 1;
  return (_mm256_movemask_pd(_mm256_and_pd(exponentAll1, mantissaNon0)) & mask) != 0;
}

template <xiiInt32 N>
XII_ALWAYS_INLINE bool xiiSimdVec4d::IsValid() const
{
  // Check the 8 exponent bits.
  // NAN -> (exponent = all 1, mantissa = non-zero)
  // INF -> (exponent = all 1, mantissa = zero)

  alignas(32) const xiiUInt64 s_exponentMask[4] = {0x7FF0000000000000ull, 0x7FF0000000000000ull, 0x7FF0000000000000ull, 0x7FF0000000000000ull};

  __m256d exponentMask = _mm256_load_pd(reinterpret_cast<const double*>(s_exponentMask));

  __m256d exponentNot1 = _mm256_cmp_pd(_mm256_and_pd(m_v, exponentMask), exponentMask, _CMP_NEQ_OQ);

  const xiiInt32 mask = XII_BIT(N) - 1;
  return (_mm256_movemask_pd(exponentNot1) & mask) == mask;
}

template <xiiInt32 N>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::GetComponent() const
{
  return _mm256_permute4x64_pd(m_v, XII_SHUFFLE(N, N, N, N));
}

XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::x() const
{
  return GetComponent<0>();
}

XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::y() const
{
  return GetComponent<1>();
}

XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::z() const
{
  return GetComponent<2>();
}

XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::w() const
{
  return GetComponent<3>();
}

template <xiiSwizzle::Enum s>
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::Get() const
{
  return _mm256_permute4x64_pd(m_v, XII_TO_SHUFFLE(s));
}

template <xiiSwizzle::Enum s>
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::GetCombined(const xiiSimdVec4d& other) const
{
  // \todo Foundation: Evaluate a possible AVX-512 intrinsic.
  __m256d permuteThis  = _mm256_permute4x64_pd(m_v, XII_TO_SHUFFLE(s));
  __m256d permuteOther = _mm256_permute4x64_pd(other.m_v, XII_TO_SHUFFLE(s));
  return _mm256_permute2f128_pd(permuteOther, permuteThis, 0x012);
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::operator-() const
{
  return _mm256_sub_pd(_mm256_setzero_pd(), m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::operator+(const xiiSimdVec4d& v) const
{
  return _mm256_add_pd(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::operator-(const xiiSimdVec4d& v) const
{
  return _mm256_sub_pd(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::operator*(const xiiSimdDouble& f) const
{
  return _mm256_mul_pd(m_v, f.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::operator/(const xiiSimdDouble& f) const
{
  return _mm256_div_pd(m_v, f.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::CompMul(const xiiSimdVec4d& v) const
{
  return _mm256_mul_pd(m_v, v.m_v);
}

#if XII_SSE_LEVEL >= XII_SSE_AVX512

template <>
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::CompDiv<xiiMathDoubleBits::BITS_14>(const xiiSimdVec4d& v) const
{
  return _mm256_mul_pd(m_v, _mm256_rcp14_pd(v.m_v));
}

template <>
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::CompDiv<xiiMathDoubleBits::BITS_27>(const xiiSimdVec4d& v) const
{
  __m256d x0 = _mm256_rcp14_pd(v.m_v);

  // One iteration of Newton-Raphson
  __m256d x1 = _mm256_mul_pd(x0, _mm256_sub_pd(_mm256_set1_pd(2.0), _mm256_mul_pd(v.m_v, x0)));

  return _mm256_mul_pd(m_v, x1);
}

#endif

template <>
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::CompDiv<xiiMathDoubleBits::FULL>(const xiiSimdVec4d& v) const
{
  return _mm256_div_pd(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::CompMin(const xiiSimdVec4d& v) const
{
  return _mm256_min_pd(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::CompMax(const xiiSimdVec4d& v) const
{
  return _mm256_max_pd(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::Abs() const
{
  return _mm256_andnot_pd(_mm256_set1_pd(-0.0), m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::Round() const
{
#if XII_SSE_LEVEL >= XII_SSE_AVX
  return _mm256_round_pd(m_v, _MM_FROUND_NINT);
#else
  XII_ASSERT_NOT_IMPLEMENTED;
#endif
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::Floor() const
{
#if XII_SSE_LEVEL >= XII_SSE_AVX
  return _mm256_round_pd(m_v, _MM_FROUND_FLOOR);
#else
  XII_ASSERT_NOT_IMPLEMENTED;
#endif
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::Ceil() const
{
#if XII_SSE_LEVEL >= XII_SSE_AVX
  return _mm256_round_pd(m_v, _MM_FROUND_CEIL);
#else
  XII_ASSERT_NOT_IMPLEMENTED;
#endif
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::Trunc() const
{
#if XII_SSE_LEVEL >= XII_SSE_AVX
  return _mm256_round_pd(m_v, _MM_FROUND_TRUNC);
#else
  XII_ASSERT_NOT_IMPLEMENTED;
#endif
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::FlipSign(const xiiSimdVec4b& cmp) const
{
  __m256d mask = _mm256_setr_pd(cmp.x() ? -0.0 : 0.0, cmp.y() ? -0.0 : 0.0, cmp.z() ? -0.0 : 0.0, cmp.w() ? -0.0 : 0.0);
  return _mm256_xor_pd(m_v, _mm256_and_pd(mask, _mm256_set1_pd(-0.0)));
}

// static
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::Select(const xiiSimdVec4b& cmp, const xiiSimdVec4d& ifTrue, const xiiSimdVec4d& ifFalse)
{
  __m256d mask = _mm256_setr_pd(cmp.x() ? -0.0 : 0.0, cmp.y() ? -0.0 : 0.0, cmp.z() ? -0.0 : 0.0, cmp.w() ? -0.0 : 0.0);
  return _mm256_blendv_pd(ifFalse.m_v, ifTrue.m_v, mask);
}

XII_ALWAYS_INLINE xiiSimdVec4d& xiiSimdVec4d::operator+=(const xiiSimdVec4d& v)
{
  m_v = _mm256_add_pd(m_v, v.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4d& xiiSimdVec4d::operator-=(const xiiSimdVec4d& v)
{
  m_v = _mm256_sub_pd(m_v, v.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4d& xiiSimdVec4d::operator*=(const xiiSimdDouble& f)
{
  m_v = _mm256_mul_pd(m_v, f.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4d& xiiSimdVec4d::operator/=(const xiiSimdDouble& f)
{
  m_v = _mm256_div_pd(m_v, f.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4d::operator==(const xiiSimdVec4d& v) const
{
  return _mm256_cvtpd_ps(_mm256_cmp_pd(m_v, v.m_v, _CMP_EQ_OQ));
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4d::operator!=(const xiiSimdVec4d& v) const
{
  return _mm256_cvtpd_ps(_mm256_cmp_pd(m_v, v.m_v, _CMP_NEQ_OQ));
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4d::operator<=(const xiiSimdVec4d& v) const
{
  return _mm256_cvtpd_ps(_mm256_cmp_pd(m_v, v.m_v, _CMP_LE_OQ));
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4d::operator<(const xiiSimdVec4d& v) const
{
  return _mm256_cvtpd_ps(_mm256_cmp_pd(m_v, v.m_v, _CMP_LT_OQ));
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4d::operator>=(const xiiSimdVec4d& v) const
{
  return _mm256_cvtpd_ps(_mm256_cmp_pd(m_v, v.m_v, _CMP_GE_OQ));
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4d::operator>(const xiiSimdVec4d& v) const
{
  return _mm256_cvtpd_ps(_mm256_cmp_pd(m_v, v.m_v, _CMP_GT_OQ));
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::HorizontalSum<2>() const
{
#if XII_SSE_LEVEL >= XII_SSE_AVX2
  __m256d a = _mm256_hadd_pd(m_v, m_v);
  return _mm256_permute4x64_pd(a, XII_TO_SHUFFLE(xiiSwizzle::XXXX));
#else
  return GetComponentM<0>() + GetComponent<1>();
#endif
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::HorizontalSum<3>() const
{
  return HorizontalSum<2>() + GetComponent<2>();
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::HorizontalSum<4>() const
{
#if XII_SSE_LEVEL >= XII_SSE_AVX
  // Adapted from Peter Cordes answer:
  //  https://stackoverflow.com/questions/49941645/get-sum-of-values-stored-in-m256d-with-sse-avx/49943540#49943540

  __m128d vlow  = _mm256_castpd256_pd128(m_v);
  __m128d vhigh = _mm256_extractf128_pd(m_v, 1); // High 128

  vlow = _mm_add_pd(vlow, vhigh); // Reduce down to 128

  __m128d high64 = _mm_unpackhi_pd(vlow, vlow);
  return _mm_cvtsd_f64(_mm_add_sd(vlow, high64)); // Reduce to scalar
#else
  return (GetComponent<0>() + GetComponent<1>()) + (GetComponent<2>() + GetComponent<3>());
#endif
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::HorizontalMin<2>() const
{
  return _mm256_min_pd(GetComponent<0>().m_v, GetComponent<1>().m_v);
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::HorizontalMin<3>() const
{
  return _mm256_min_pd(_mm256_min_pd(GetComponent<0>().m_v, GetComponent<1>().m_v), GetComponent<2>().m_v);
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::HorizontalMin<4>() const
{
  __m256d xyxyzwzw = _mm256_min_pd(_mm256_permute4x64_pd(m_v, XII_TO_SHUFFLE(xiiSwizzle::ZWXY)), m_v);
  __m256d zwzwxyxy = _mm256_permute4x64_pd(xyxyzwzw, XII_TO_SHUFFLE(xiiSwizzle::YXWZ));
  return _mm256_min_pd(xyxyzwzw, zwzwxyxy);
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::HorizontalMax<2>() const
{
  return _mm256_max_pd(GetComponent<0>().m_v, GetComponent<1>().m_v);
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::HorizontalMax<3>() const
{
  return _mm256_max_pd(_mm256_max_pd(GetComponent<0>().m_v, GetComponent<1>().m_v), GetComponent<2>().m_v);
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::HorizontalMax<4>() const
{
  __m256d xyxyzwzw = _mm256_max_pd(_mm256_permute4x64_pd(m_v, XII_TO_SHUFFLE(xiiSwizzle::ZWXY)), m_v);
  __m256d zwzwxyxy = _mm256_permute4x64_pd(xyxyzwzw, XII_TO_SHUFFLE(xiiSwizzle::YXWZ));
  return _mm256_max_pd(xyxyzwzw, zwzwxyxy);
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::Dot<1>(const xiiSimdVec4d& v) const
{
  return CompMul(v).HorizontalSum<1>();
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::Dot<2>(const xiiSimdVec4d& v) const
{
  return CompMul(v).HorizontalSum<2>();
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::Dot<3>(const xiiSimdVec4d& v) const
{
  return CompMul(v).HorizontalSum<3>();
}

template <>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdVec4d::Dot<4>(const xiiSimdVec4d& v) const
{
  return CompMul(v).HorizontalSum<4>();
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::CrossRH(const xiiSimdVec4d& v) const
{
  __m256d a = _mm256_mul_pd(m_v, _mm256_permute4x64_pd(v.m_v, XII_TO_SHUFFLE(xiiSwizzle::YZXW)));
  __m256d b = _mm256_mul_pd(v.m_v, _mm256_permute4x64_pd(m_v, XII_TO_SHUFFLE(xiiSwizzle::YZXW)));
  __m256d c = _mm256_sub_pd(a, b);

  return _mm256_permute4x64_pd(c, XII_TO_SHUFFLE(xiiSwizzle::YZXW));
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::GetOrthogonalVector() const
{
  // See http://blog.selfshadow.com/2011/10/17/perp-vectors/ - this is Stark's first variant, SIMDified.
  return CrossRH(_mm256_and_pd(m_v, _mm256_cmp_pd(m_v, HorizontalMin<3>().m_v, _CMP_EQ_OQ)));
}

// static
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::ZeroVector()
{
  return _mm256_setzero_pd();
}

// static
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::MulAdd(const xiiSimdVec4d& a, const xiiSimdVec4d& b, const xiiSimdVec4d& c)
{
#if XII_SSE_LEVEL >= XII_SSE_AVX2
  return _mm256_fmadd_pd(a.m_v, b.m_v, c.m_v);
#else
  return a.CompMul(b) + c;
#endif
}

// static
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::MulAdd(const xiiSimdVec4d& a, const xiiSimdDouble& b, const xiiSimdVec4d& c)
{
#if XII_SSE_LEVEL >= XII_SSE_AVX2
  return _mm256_fmadd_pd(a.m_v, b.m_v, c.m_v);
#else
  return a * b + c;
#endif
}

// static
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::MulSub(const xiiSimdVec4d& a, const xiiSimdVec4d& b, const xiiSimdVec4d& c)
{
#if XII_SSE_LEVEL >= XII_SSE_AVX2
  return _mm256_fmsub_pd(a.m_v, b.m_v, c.m_v);
#else
  return a.CompMul(b) - c;
#endif
}

// static
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::MulSub(const xiiSimdVec4d& a, const xiiSimdDouble& b, const xiiSimdVec4d& c)
{
#if XII_SSE_LEVEL >= XII_SSE_AVX2
  return _mm256_fmsub_pd(a.m_v, b.m_v, c.m_v);
#else
  return a * b - c;
#endif
}

// static
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdVec4d::CopySign(const xiiSimdVec4d& magnitude, const xiiSimdVec4d& sign)
{
  __m256d minusZero = _mm256_set1_pd(-0.0);
  return _mm256_or_pd(_mm256_andnot_pd(minusZero, magnitude.m_v), _mm256_and_pd(minusZero, sign.m_v));
}
