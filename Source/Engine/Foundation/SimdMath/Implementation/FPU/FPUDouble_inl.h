#pragma once

XII_ALWAYS_INLINE xiiSimdDouble::xiiSimdDouble() {}

XII_ALWAYS_INLINE xiiSimdDouble::xiiSimdDouble(double f)
{
  m_v.Set(f);
}

XII_ALWAYS_INLINE xiiSimdDouble::xiiSimdDouble(float f)
{
  m_v.Set(static_cast<double>(f));
}

XII_ALWAYS_INLINE xiiSimdDouble::xiiSimdDouble(xiiInt32 i)
{
  m_v.Set(static_cast<double>(i));
}

XII_ALWAYS_INLINE xiiSimdDouble::xiiSimdDouble(xiiUInt32 i)
{
  m_v.Set(static_cast<double>(i));
}

XII_ALWAYS_INLINE xiiSimdDouble::xiiSimdDouble(xiiAngled a)
{
  m_v.Set(a.GetRadian());
}

XII_ALWAYS_INLINE xiiSimdDouble::xiiSimdDouble(xiiInternal::QuadDouble v)
{
  m_v = v;
}

XII_ALWAYS_INLINE xiiSimdDouble::operator double() const
{
  return m_v.x;
}

// static
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdDouble::Zero()
{
  return xiiSimdDouble(0.0);
}

XII_ALWAYS_INLINE xiiSimdDouble xiiSimdDouble::operator+(const xiiSimdDouble& f) const
{
  return m_v + f.m_v;
}

XII_ALWAYS_INLINE xiiSimdDouble xiiSimdDouble::operator-(const xiiSimdDouble& f) const
{
  return m_v - f.m_v;
}

XII_ALWAYS_INLINE xiiSimdDouble xiiSimdDouble::operator*(const xiiSimdDouble& f) const
{
  return m_v.CompMul(f.m_v);
}

XII_ALWAYS_INLINE xiiSimdDouble xiiSimdDouble::operator/(const xiiSimdDouble& f) const
{
  return m_v.CompDiv(f.m_v);
}

XII_ALWAYS_INLINE xiiSimdDouble& xiiSimdDouble::operator+=(const xiiSimdDouble& f)
{
  m_v += f.m_v;
  return *this;
}

XII_ALWAYS_INLINE xiiSimdDouble& xiiSimdDouble::operator-=(const xiiSimdDouble& f)
{
  m_v -= f.m_v;
  return *this;
}

XII_ALWAYS_INLINE xiiSimdDouble& xiiSimdDouble::operator*=(const xiiSimdDouble& f)
{
  m_v = m_v.CompMul(f.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdDouble& xiiSimdDouble::operator/=(const xiiSimdDouble& f)
{
  m_v = m_v.CompDiv(f.m_v);
  return *this;
}

XII_ALWAYS_INLINE bool xiiSimdDouble::IsEqual(const xiiSimdDouble& rhs, const xiiSimdDouble& fEpsilon) const
{
  return m_v.IsEqual(rhs.m_v, fEpsilon);
}

XII_ALWAYS_INLINE bool xiiSimdDouble::operator==(const xiiSimdDouble& f) const
{
  return m_v.x == f.m_v.x;
}

XII_ALWAYS_INLINE bool xiiSimdDouble::operator!=(const xiiSimdDouble& f) const
{
  return m_v.x != f.m_v.x;
}

XII_ALWAYS_INLINE bool xiiSimdDouble::operator>=(const xiiSimdDouble& f) const
{
  return m_v.x >= f.m_v.x;
}

XII_ALWAYS_INLINE bool xiiSimdDouble::operator>(const xiiSimdDouble& f) const
{
  return m_v.x > f.m_v.x;
}

XII_ALWAYS_INLINE bool xiiSimdDouble::operator<=(const xiiSimdDouble& f) const
{
  return m_v.x <= f.m_v.x;
}

XII_ALWAYS_INLINE bool xiiSimdDouble::operator<(const xiiSimdDouble& f) const
{
  return m_v.x < f.m_v.x;
}

XII_ALWAYS_INLINE bool xiiSimdDouble::operator==(double f) const
{
  return m_v.x == f;
}

XII_ALWAYS_INLINE bool xiiSimdDouble::operator!=(double f) const
{
  return m_v.x != f;
}

XII_ALWAYS_INLINE bool xiiSimdDouble::operator>(double f) const
{
  return m_v.x > f;
}

XII_ALWAYS_INLINE bool xiiSimdDouble::operator>=(double f) const
{
  return m_v.x >= f;
}

XII_ALWAYS_INLINE bool xiiSimdDouble::operator<(double f) const
{
  return m_v.x < f;
}

XII_ALWAYS_INLINE bool xiiSimdDouble::operator<=(double f) const
{
  return m_v.x <= f;
}

template <xiiMathDoubleBits::Enum bits>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdDouble::GetReciprocal() const
{
  return xiiSimdDouble(1.0 / m_v.x);
}

template <xiiMathDoubleBits::Enum bits>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdDouble::GetSqrt() const
{
  return xiiSimdDouble(xiiMath::Sqrt(m_v.x));
}

template <xiiMathDoubleBits::Enum bits>
XII_ALWAYS_INLINE xiiSimdDouble xiiSimdDouble::GetInvSqrt() const
{
  return xiiSimdDouble(1.0 / xiiMath::Sqrt(m_v.x));
}

XII_ALWAYS_INLINE xiiSimdDouble xiiSimdDouble::Max(const xiiSimdDouble& f) const
{
  return m_v.CompMax(f.m_v);
}

XII_ALWAYS_INLINE xiiSimdDouble xiiSimdDouble::Min(const xiiSimdDouble& f) const
{
  return m_v.CompMin(f.m_v);
}

XII_ALWAYS_INLINE xiiSimdDouble xiiSimdDouble::Abs() const
{
  return xiiSimdDouble(xiiMath::Abs(m_v.x));
}
