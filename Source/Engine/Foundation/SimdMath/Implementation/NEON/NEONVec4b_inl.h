#pragma once

XII_ALWAYS_INLINE xiiSimdVec4b::xiiSimdVec4b()
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);
}

XII_ALWAYS_INLINE xiiSimdVec4b::xiiSimdVec4b(bool b)
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);

  m_v = vmovq_n_u32(b ? 0xFFFFFFFF : 0);
}

XII_ALWAYS_INLINE xiiSimdVec4b::xiiSimdVec4b(bool x, bool y, bool z, bool w)
{
  XII_CHECK_SIMD_FLOAT_ALIGNMENT(this);

  alignas(16) xiiUInt32 mask[4] = {x ? 0xFFFFFFFF : 0, y ? 0xFFFFFFFF : 0, z ? 0xFFFFFFFF : 0, w ? 0xFFFFFFFF : 0};
  m_v = vld1q_u32(mask);
}

XII_ALWAYS_INLINE xiiSimdVec4b::xiiSimdVec4b(xiiInternal::QuadBool v)
{
  m_v = v;
}

template <int N>
XII_ALWAYS_INLINE bool xiiSimdVec4b::GetComponent() const
{
  return vgetq_lane_u32(m_v, N) & 1;
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
  return __builtin_shufflevector(m_v, m_v, XII_TO_SHUFFLE(s));
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4b::operator&&(const xiiSimdVec4b& rhs) const
{
  return vandq_u32(m_v, rhs.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4b::operator||(const xiiSimdVec4b& rhs) const
{
  return vorrq_u32(m_v, rhs.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4b::operator!() const
{
  return vmvnq_u32(m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4b::operator==(const xiiSimdVec4b& rhs) const
{
  return vceqq_u32(m_v, rhs.m_v);
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4b::operator!=(const xiiSimdVec4b& rhs) const
{
  return veorq_u32(m_v, rhs.m_v);
}

template <int N>
XII_ALWAYS_INLINE bool xiiSimdVec4b::AllSet() const
{
  const int mask = XII_BIT(N) - 1;
  return (xiiInternal::NeonMoveMask(m_v) & mask) == mask;
}

template <int N>
XII_ALWAYS_INLINE bool xiiSimdVec4b::AnySet() const
{
  const int mask = XII_BIT(N) - 1;
  return (xiiInternal::NeonMoveMask(m_v) & mask) != 0;
}

template <int N>
XII_ALWAYS_INLINE bool xiiSimdVec4b::NoneSet() const
{
  const int mask = XII_BIT(N) - 1;
  return (xiiInternal::NeonMoveMask(m_v) & mask) == 0;
}

// static
XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4b::Select(const xiiSimdVec4b& vCmp, const xiiSimdVec4b& vTrue, const xiiSimdVec4b& vFalse)
{
  return vbslq_u32(vCmp.m_v, vTrue.m_v, vFalse.m_v);
}
