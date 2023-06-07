#pragma once

XII_ALWAYS_INLINE xiiSimdVec4u::xiiSimdVec4u()
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  m_v = _mm_set1_epi32(0xCDCDCDCD);
#endif
}

XII_ALWAYS_INLINE xiiSimdVec4u::xiiSimdVec4u(xiiUInt32 uiXyzw)
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);

  m_v = _mm_set1_epi32(uiXyzw);
}

XII_ALWAYS_INLINE xiiSimdVec4u::xiiSimdVec4u(xiiUInt32 x, xiiUInt32 y, xiiUInt32 z, xiiUInt32 w)
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);

  m_v = _mm_setr_epi32(x, y, z, w);
}

XII_ALWAYS_INLINE xiiSimdVec4u::xiiSimdVec4u(xiiInternal::QuadInt v)
{
  m_v = v;
}

XII_ALWAYS_INLINE void xiiSimdVec4u::Set(xiiUInt32 uiXyzw)
{
  m_v = _mm_set1_epi32(uiXyzw);
}

XII_ALWAYS_INLINE void xiiSimdVec4u::Set(xiiUInt32 x, xiiUInt32 y, xiiUInt32 z, xiiUInt32 w)
{
  m_v = _mm_setr_epi32(x, y, z, w);
}

XII_ALWAYS_INLINE void xiiSimdVec4u::SetZero()
{
  m_v = _mm_setzero_si128();
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
  __m128  two16 = _mm_set1_ps((float)0x10000); // 2^16
  __m128i high  = _mm_srli_epi32(m_v, 16);
  __m128i low   = _mm_srli_epi32(_mm_slli_epi32(m_v, 16), 16);
  __m128  fHigh = _mm_mul_ps(_mm_cvtepi32_ps(high), two16);
  __m128  fLow  = _mm_cvtepi32_ps(low);

  return _mm_add_ps(fHigh, fLow);
}

// static
XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::Truncate(const xiiSimdVec4f& f)
{
  alignas(16) const float fmax[4]          = {2.14748364e+009f, 2.14748364e+009f, 2.14748364e+009f, 2.14748364e+009f};
  alignas(16) const float fmax_unsigned[4] = {4.29496729e+009f, 4.29496729e+009f, 4.29496729e+009f, 4.29496729e+009f};
  __m128i                 zero             = _mm_setzero_si128();
  __m128i                 mask             = _mm_cmpgt_epi32(_mm_castps_si128(f.m_v), zero);
  __m128                  min              = _mm_and_ps(_mm_castsi128_ps(mask), f.m_v);
  __m128                  max              = _mm_min_ps(min, _mm_load_ps(fmax_unsigned)); // clamped in 0 - 4.29496729+009

  __m128 diff = _mm_sub_ps(max, _mm_load_ps(fmax));
  mask        = _mm_cmpgt_epi32(_mm_castps_si128(diff), zero);
  diff        = _mm_and_ps(_mm_castsi128_ps(mask), diff);

  __m128i res1 = _mm_cvttps_epi32(diff);
  __m128i res2 = _mm_cvttps_epi32(max);
  return _mm_add_epi32(res1, res2);
}

template <int N>
XII_ALWAYS_INLINE xiiUInt32 xiiSimdVec4u::GetComponent() const
{
#if XII_SSE_LEVEL >= XII_SSE_41
  return _mm_extract_epi32(m_v, N);
#else
  return m_v.m128i_i32[N];
#endif
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
  return _mm_shuffle_epi32(m_v, XII_TO_SHUFFLE(s));
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::operator+(const xiiSimdVec4u& v) const
{
  return _mm_add_epi32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::operator-(const xiiSimdVec4u& v) const
{
  return _mm_sub_epi32(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::CompMul(const xiiSimdVec4u& v) const
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

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::operator|(const xiiSimdVec4u& v) const
{
  return _mm_or_si128(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::operator&(const xiiSimdVec4u& v) const
{
  return _mm_and_si128(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::operator^(const xiiSimdVec4u& v) const
{
  return _mm_xor_si128(m_v, v.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::operator~() const
{
  __m128i ones = _mm_cmpeq_epi8(_mm_setzero_si128(), _mm_setzero_si128());
  return _mm_xor_si128(ones, m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::operator<<(xiiUInt32 uiShift) const
{
  return _mm_slli_epi32(m_v, uiShift);
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::operator>>(xiiUInt32 uiShift) const
{
  return _mm_srli_epi32(m_v, uiShift);
}

XII_ALWAYS_INLINE xiiSimdVec4u& xiiSimdVec4u::operator+=(const xiiSimdVec4u& v)
{
  m_v = _mm_add_epi32(m_v, v.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4u& xiiSimdVec4u::operator-=(const xiiSimdVec4u& v)
{
  m_v = _mm_sub_epi32(m_v, v.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4u& xiiSimdVec4u::operator|=(const xiiSimdVec4u& v)
{
  m_v = _mm_or_si128(m_v, v.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4u& xiiSimdVec4u::operator&=(const xiiSimdVec4u& v)
{
  m_v = _mm_and_si128(m_v, v.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4u& xiiSimdVec4u::operator^=(const xiiSimdVec4u& v)
{
  m_v = _mm_xor_si128(m_v, v.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4u& xiiSimdVec4u::operator<<=(xiiUInt32 uiShift)
{
  m_v = _mm_slli_epi32(m_v, uiShift);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4u& xiiSimdVec4u::operator>>=(xiiUInt32 uiShift)
{
  m_v = _mm_srli_epi32(m_v, uiShift);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::CompMin(const xiiSimdVec4u& v) const
{
#if XII_SSE_LEVEL >= XII_SSE_41
  return _mm_min_epu32(m_v, v.m_v);
#else
  __m128i mask = _mm_cmplt_epi32(m_v, v.m_v);
  return _mm_or_si128(_mm_and_si128(mask, m_v), _mm_andnot_si128(mask, v.m_v));
#endif
}

XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::CompMax(const xiiSimdVec4u& v) const
{
#if XII_SSE_LEVEL >= XII_SSE_41
  return _mm_max_epu32(m_v, v.m_v);
#else
  __m128i mask = _mm_cmpgt_epi32(m_v, v.m_v);
  return _mm_or_si128(_mm_and_si128(mask, m_v), _mm_andnot_si128(mask, v.m_v));
#endif
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4u::operator==(const xiiSimdVec4u& v) const
{
  return _mm_castsi128_ps(_mm_cmpeq_epi32(m_v, v.m_v));
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4u::operator!=(const xiiSimdVec4u& v) const
{
  return !(*this == v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4u::operator<=(const xiiSimdVec4u& v) const
{
#if XII_SSE_LEVEL >= XII_SSE_41
  __m128i minValue = _mm_min_epu32(m_v, v.m_v);
  return _mm_castsi128_ps(_mm_cmpeq_epi32(minValue, m_v));
#else
  return !(*this > v);
#endif
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4u::operator<(const xiiSimdVec4u& v) const
{
  __m128i signBit = _mm_set1_epi32(0x80000000);
  __m128i a       = _mm_sub_epi32(m_v, signBit);
  __m128i b       = _mm_sub_epi32(v.m_v, signBit);
  return _mm_castsi128_ps(_mm_cmplt_epi32(a, b));
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4u::operator>=(const xiiSimdVec4u& v) const
{
#if XII_SSE_LEVEL >= XII_SSE_41
  __m128i maxValue = _mm_max_epu32(m_v, v.m_v);
  return _mm_castsi128_ps(_mm_cmpeq_epi32(maxValue, m_v));
#else
  return !(*this < v);
#endif
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4u::operator>(const xiiSimdVec4u& v) const
{
  __m128i signBit = _mm_set1_epi32(0x80000000);
  __m128i a       = _mm_sub_epi32(m_v, signBit);
  __m128i b       = _mm_sub_epi32(v.m_v, signBit);
  return _mm_castsi128_ps(_mm_cmpgt_epi32(a, b));
}

// static
XII_ALWAYS_INLINE xiiSimdVec4u xiiSimdVec4u::ZeroVector()
{
  return _mm_setzero_si128();
}

// not needed atm
#if 0
void xiiSimdVec4u::Transpose(xiiSimdVec4u& v0, xiiSimdVec4u& v1, xiiSimdVec4u& v2, xiiSimdVec4u& v3)
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
