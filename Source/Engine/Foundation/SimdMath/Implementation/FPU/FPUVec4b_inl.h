#pragma once

XII_ALWAYS_INLINE xiiSimdVec4b::xiiSimdVec4b() {}

XII_ALWAYS_INLINE xiiSimdVec4b::xiiSimdVec4b(bool b)
{
  m_v.x = b;
  m_v.y = b;
  m_v.z = b;
  m_v.w = b;
}

XII_ALWAYS_INLINE xiiSimdVec4b::xiiSimdVec4b(bool x, bool y, bool z, bool w)
{
  m_v.x = x;
  m_v.y = y;
  m_v.z = z;
  m_v.w = w;
}

XII_ALWAYS_INLINE xiiSimdVec4b::xiiSimdVec4b(xiiInternal::QuadBool v)
{
  m_v = v;
}

template <int N>
XII_ALWAYS_INLINE bool xiiSimdVec4b::GetComponent() const
{
  return (&m_v.x)[N];
}

XII_ALWAYS_INLINE bool xiiSimdVec4b::x() const
{
  return m_v.x;
}

XII_ALWAYS_INLINE bool xiiSimdVec4b::y() const
{
  return m_v.y;
}

XII_ALWAYS_INLINE bool xiiSimdVec4b::z() const
{
  return m_v.z;
}

XII_ALWAYS_INLINE bool xiiSimdVec4b::w() const
{
  return m_v.w;
}

template <xiiSwizzle::Enum s>
XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4b::Get() const
{
  xiiSimdVec4b result;

  const bool* v = &m_v.x;
  result.m_v.x  = v[(s & 0x3000) >> 12];
  result.m_v.y  = v[(s & 0x0300) >> 8];
  result.m_v.z  = v[(s & 0x0030) >> 4];
  result.m_v.w  = v[(s & 0x0003)];

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4b::operator&&(const xiiSimdVec4b& rhs) const
{
  xiiSimdVec4b result;
  result.m_v.x = m_v.x && rhs.m_v.x;
  result.m_v.y = m_v.y && rhs.m_v.y;
  result.m_v.z = m_v.z && rhs.m_v.z;
  result.m_v.w = m_v.w && rhs.m_v.w;

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4b::operator||(const xiiSimdVec4b& rhs) const
{
  xiiSimdVec4b result;
  result.m_v.x = m_v.x || rhs.m_v.x;
  result.m_v.y = m_v.y || rhs.m_v.y;
  result.m_v.z = m_v.z || rhs.m_v.z;
  result.m_v.w = m_v.w || rhs.m_v.w;

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4b xiiSimdVec4b::operator!() const
{
  xiiSimdVec4b result;
  result.m_v.x = !m_v.x;
  result.m_v.y = !m_v.y;
  result.m_v.z = !m_v.z;
  result.m_v.w = !m_v.w;

  return result;
}

template <int N>
XII_ALWAYS_INLINE bool xiiSimdVec4b::AllSet() const
{
  for (int i = 0; i < N; ++i)
  {
    if (!(&m_v.x)[i])
      return false;
  }

  return true;
}

template <int N>
XII_ALWAYS_INLINE bool xiiSimdVec4b::AnySet() const
{
  for (int i = 0; i < N; ++i)
  {
    if ((&m_v.x)[i])
      return true;
  }

  return false;
}

template <int N>
XII_ALWAYS_INLINE bool xiiSimdVec4b::NoneSet() const
{
  return !AnySet<N>();
}
