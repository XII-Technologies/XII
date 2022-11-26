#pragma once

XII_ALWAYS_INLINE xiiSimdVec4b::xiiSimdVec4b()
{
  XII_CHECK_SIMD_ALIGNMENT(this);
}

XII_ALWAYS_INLINE xiiSimdVec4b::xiiSimdVec4b(bool b)
{
  XII_CHECK_SIMD_ALIGNMENT(this);

  xiiUInt32 mask = b ? 0xFFFFFFFF : 0;
  __m128    tmp  = _mm_load_ss((float*)&mask);
  m_v            = _mm_shuffle_ps(tmp, tmp, XII_TO_SHUFFLE(xiiSwizzle::XXXX));
}

XII_ALWAYS_INLINE xiiSimdVec4b::xiiSimdVec4b(bool x, bool y, bool z, bool w)
{
  XII_CHECK_SIMD_ALIGNMENT(this);

  alignas(16) xiiUInt32 mask[4] = {x ? 0xFFFFFFFF : 0, y ? 0xFFFFFFFF : 0, z ? 0xFFFFFFFF : 0, w ? 0xFFFFFFFF : 0};
  m_v                           = _mm_load_ps((float*)mask);
}

XII_ALWAYS_INLINE xiiSimdVec4b::xiiSimdVec4b(xiiInternal::QuadBool v)
{
  m_v = v;
}

template <int N>
XII_ALWAYS_INLINE bool xiiSimdVec4b::GetComponent() const
{
  return _mm_movemask_ps(_mm_shuffle_ps(m_v, m_v, XII_SHUFFLE(N, N, N, N))) != 0;
}

XII_ALWAYS_INLINE bool xiiSimdVec4b::x() const
{
  return GetComponent<0>();
}

XII_ALWAYS_INLINE bool xiiSimdVec4b::y() const
{
  return GetComponent<1>();
}

XII_ALWAYS_INLINE bool xiiSimdVec4b::z() const
{
  return GetComponent<2>();
}

XII_ALWAYS_INLINE bool xiiSimdVec4b::w() const
{
  return GetComponent<3>();
}

template <xiiSwizzle::Enum s>
XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4b::Get() const
{
  return _mm_shuffle_ps(m_v, m_v, XII_TO_SHUFFLE(s));
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4b::operator&&(const xiiSimdVec4b& rhs) const
{
  return _mm_and_ps(m_v, rhs.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4b::operator||(const xiiSimdVec4b& rhs) const
{
  return _mm_or_ps(m_v, rhs.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4b::operator!() const
{
  __m128 allTrue = _mm_cmpeq_ps(_mm_setzero_ps(), _mm_setzero_ps());
  return _mm_xor_ps(m_v, allTrue);
}

template <int N>
XII_ALWAYS_INLINE bool xiiSimdVec4b::AllSet() const
{
  const int mask = XII_BIT(N) - 1;
  return (_mm_movemask_ps(m_v) & mask) == mask;
}

template <int N>
XII_ALWAYS_INLINE bool xiiSimdVec4b::AnySet() const
{
  const int mask = XII_BIT(N) - 1;
  return (_mm_movemask_ps(m_v) & mask) != 0;
}

template <int N>
XII_ALWAYS_INLINE bool xiiSimdVec4b::NoneSet() const
{
  const int mask = XII_BIT(N) - 1;
  return (_mm_movemask_ps(m_v) & mask) == 0;
}
