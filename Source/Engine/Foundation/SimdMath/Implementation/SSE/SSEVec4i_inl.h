#pragma once

XII_ALWAYS_INLINE xiiSimdVec4i::xiiSimdVec4i()
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  m_v = _mm_set1_epi32(0xCDCDCDCD);
#endif
}

XII_ALWAYS_INLINE xiiSimdVec4i::xiiSimdVec4i(xiiInt32 xyzw)
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);

  m_v = _mm_set1_epi32(xyzw);
}

XII_ALWAYS_INLINE xiiSimdVec4i::xiiSimdVec4i(xiiInt32 x, xiiInt32 y, xiiInt32 z, xiiInt32 w)
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);

  m_v = _mm_setr_epi32(x, y, z, w);
}

XII_ALWAYS_INLINE xiiSimdVec4i::xiiSimdVec4i(xiiInternal::QuadInt v)
{
  m_v = v;
}

XII_ALWAYS_INLINE void xiiSimdVec4i::Set(xiiInt32 xyzw)
{
  m_v = _mm_set1_epi32(xyzw);
}

XII_ALWAYS_INLINE void xiiSimdVec4i::Set(xiiInt32 x, xiiInt32 y, xiiInt32 z, xiiInt32 w)
{
  m_v = _mm_setr_epi32(x, y, z, w);
}

XII_ALWAYS_INLINE void xiiSimdVec4i::SetZero()
{
  m_v = _mm_setzero_si128();
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4i::Load<1>(const xiiInt32* pInts)
{
  m_v = _mm_loadu_si32(pInts);
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4i::Load<2>(const xiiInt32* pInts)
{
  m_v = _mm_loadu_si64(pInts);
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4i::Load<3>(const xiiInt32* pInts)
{
  m_v = _mm_setr_epi32(pInts[0], pInts[1], pInts[2], 0);
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4i::Load<4>(const xiiInt32* pInts)
{
  m_v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pInts));
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4i::Store<1>(xiiInt32* pInts) const
{
  _mm_storeu_si32(pInts, m_v);
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4i::Store<2>(xiiInt32* pInts) const
{
  _mm_storeu_si64(pInts, m_v);
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4i::Store<3>(xiiInt32* pInts) const
{
  _mm_storeu_si64(pInts, m_v);
  _mm_storeu_si32(pInts + 2, _mm_castps_si128(_mm_movehl_ps(_mm_castsi128_ps(m_v), _mm_castsi128_ps(m_v))));
}

template <>
XII_ALWAYS_INLINE void xiiSimdVec4i::Store<4>(xiiInt32* pInts) const
{
  _mm_storeu_si128(reinterpret_cast<__m128i*>(pInts), m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdVec4i::ToFloat() const
{
  return _mm_cvtepi32_ps(m_v);
}

// static
XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::Truncate(const xiiSimdVec4f& f)
{
  return _mm_cvttps_epi32(f.m_v);
}

template <xiiInt32 N>
XII_ALWAYS_INLINE xiiInt32 xiiSimdVec4i::GetComponent() const
{
#if XII_SSE_LEVEL >= XII_SSE_41
  return _mm_extract_epi32(m_v, N);
#else
  return m_v.m128i_i32[N];
#endif
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
  return _mm_shuffle_epi32(m_v, XII_TO_SHUFFLE(s));
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator-() const
{
  return _mm_sub_epi32(_mm_setzero_si128(), m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator+(const xiiSimdVec4i& v) const
{
  return _mm_add_epi32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator-(const xiiSimdVec4i& v) const
{
  return _mm_sub_epi32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::CompMul(const xiiSimdVec4i& v) const
{
#if XII_SSE_LEVEL >= XII_SSE_41
  return _mm_mullo_epi32(m_v, v.m_v);
#else
  XII_ASSERT_NOT_IMPLEMENTED; // not sure whether this code works so better assert
  __m128i tmp1 = _mm_mul_epu32(m_v, v.m_v);
  __m128i tmp2 = _mm_mul_epu32(_mm_srli_si128(m_v, 4), _mm_srli_si128(v.m_v, 4));
  return _mm_unpacklo_epi32(_mm_shuffle_epi32(tmp1, XII_SHUFFLE(0, 2, 0, 0)), _mm_shuffle_epi32(tmp2, XII_SHUFFLE(0, 2, 0, 0)));
#endif
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::CompDiv(const xiiSimdVec4i& v) const
{
#if XII_ENABLED(XII_COMPILER_MSVC)
  return _mm_div_epi32(m_v, v.m_v);
#else
  int a[4];
  int b[4];
  Store<4>(a);
  v.Store<4>(b);

  for (xiiUInt32 i = 0; i < 4; ++i)
  {
    a[i] = a[i] / b[i];
  }

  xiiSimdVec4i r;
  r.Load<4>(a);
  return r;
#endif
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator|(const xiiSimdVec4i& v) const
{
  return _mm_or_si128(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator&(const xiiSimdVec4i& v) const
{
  return _mm_and_si128(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator^(const xiiSimdVec4i& v) const
{
  return _mm_xor_si128(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator~() const
{
  __m128i ones = _mm_cmpeq_epi8(_mm_setzero_si128(), _mm_setzero_si128());
  return _mm_xor_si128(ones, m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator<<(xiiUInt32 uiShift) const
{
  return _mm_slli_epi32(m_v, uiShift);
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::operator>>(xiiUInt32 uiShift) const
{
  return _mm_srai_epi32(m_v, uiShift);
}

XII_FORCE_INLINE xiiSimdVec4i xiiSimdVec4i::operator<<(const xiiSimdVec4i& v) const
{
  int a[4];
  int b[4];
  Store<4>(a);
  v.Store<4>(b);

  for (xiiUInt32 i = 0; i < 4; ++i)
  {
    a[i] = a[i] << b[i];
  }

  xiiSimdVec4i r;
  r.Load<4>(a);
  return r;
}

XII_FORCE_INLINE xiiSimdVec4i xiiSimdVec4i::operator>>(const xiiSimdVec4i& v) const
{
  int a[4];
  int b[4];
  Store<4>(a);
  v.Store<4>(b);

  for (xiiUInt32 i = 0; i < 4; ++i)
  {
    a[i] = a[i] >> b[i];
  }

  xiiSimdVec4i r;
  r.Load<4>(a);
  return r;
}

XII_ALWAYS_INLINE xiiSimdVec4i& xiiSimdVec4i::operator+=(const xiiSimdVec4i& v)
{
  m_v = _mm_add_epi32(m_v, v.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4i& xiiSimdVec4i::operator-=(const xiiSimdVec4i& v)
{
  m_v = _mm_sub_epi32(m_v, v.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4i& xiiSimdVec4i::operator|=(const xiiSimdVec4i& v)
{
  m_v = _mm_or_si128(m_v, v.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4i& xiiSimdVec4i::operator&=(const xiiSimdVec4i& v)
{
  m_v = _mm_and_si128(m_v, v.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4i& xiiSimdVec4i::operator^=(const xiiSimdVec4i& v)
{
  m_v = _mm_xor_si128(m_v, v.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4i& xiiSimdVec4i::operator<<=(xiiUInt32 uiShift)
{
  m_v = _mm_slli_epi32(m_v, uiShift);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4i& xiiSimdVec4i::operator>>=(xiiUInt32 uiShift)
{
  m_v = _mm_srai_epi32(m_v, uiShift);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::CompMin(const xiiSimdVec4i& v) const
{
#if XII_SSE_LEVEL >= XII_SSE_41
  return _mm_min_epi32(m_v, v.m_v);
#else
  __m128i mask = _mm_cmplt_epi32(m_v, v.m_v);
  return _mm_or_si128(_mm_and_si128(mask, m_v), _mm_andnot_si128(mask, v.m_v));
#endif
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::CompMax(const xiiSimdVec4i& v) const
{
#if XII_SSE_LEVEL >= XII_SSE_41
  return _mm_max_epi32(m_v, v.m_v);
#else
  __m128i mask = _mm_cmpgt_epi32(m_v, v.m_v);
  return _mm_or_si128(_mm_and_si128(mask, m_v), _mm_andnot_si128(mask, v.m_v));
#endif
}

XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::Abs() const
{
#if XII_SSE_LEVEL >= XII_SSE_31
  return _mm_abs_epi32(m_v);
#else
  __m128i negMask = _mm_cmplt_epi32(m_v, _mm_setzero_si128());
  __m128i neg     = _mm_sub_epi32(_mm_setzero_si128(), m_v);
  return _mm_or_si128(_mm_and_si128(negMask, neg), _mm_andnot_si128(negMask, m_v));
#endif
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4i::operator==(const xiiSimdVec4i& v) const
{
  return _mm_castsi128_ps(_mm_cmpeq_epi32(m_v, v.m_v));
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
  return _mm_castsi128_ps(_mm_cmplt_epi32(m_v, v.m_v));
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4i::operator>=(const xiiSimdVec4i& v) const
{
  return !(*this < v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4i::operator>(const xiiSimdVec4i& v) const
{
  return _mm_castsi128_ps(_mm_cmpgt_epi32(m_v, v.m_v));
}

// static
XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::ZeroVector()
{
  return _mm_setzero_si128();
}

// static
XII_ALWAYS_INLINE xiiSimdVec4i xiiSimdVec4i::Select(const xiiSimdVec4b& cmp, const xiiSimdVec4i& ifTrue, const xiiSimdVec4i& ifFalse)
{
#if XII_SSE_LEVEL >= XII_SSE_41
  return _mm_castps_si128(_mm_blendv_ps(_mm_castsi128_ps(ifFalse.m_v), _mm_castsi128_ps(ifTrue.m_v), cmp.m_v));
#else
  return _mm_castps_si128(_mm_or_ps(_mm_andnot_ps(cmp.m_v, _mm_castsi128_ps(ifFalse.m_v)), _mm_and_ps(cmp.m_v, _mm_castsi128_ps(ifTrue.m_v))));
#endif
}

// not needed atm
#if 0
void xiiSimdVec4i::Transpose(xiiSimdVec4i& v0, xiiSimdVec4i& v1, xiiSimdVec4i& v2, xiiSimdVec4i& v3)
{
  __m128i T0 = _mm_unpacklo_epi32(v0.m_v, v1.m_v);
  __m128i T1 = _mm_unpacklo_epi32(v2.m_v, v3.m_v);
  __m128i T2 = _mm_unpackhi_epi32(v0.m_v, v1.m_v);
  __m128i T3 = _mm_unpackhi_epi32(v2.m_v, v3.m_v);

  v0.m_v = _mm_unpacklo_epi64(T0, T1);
  v1.m_v = _mm_unpackhi_epi64(T0, T1);
  v2.m_v = _mm_unpacklo_epi64(T2, T3);
  v3.m_v = _mm_unpackhi_epi64(T2, T3);
}
#endif
