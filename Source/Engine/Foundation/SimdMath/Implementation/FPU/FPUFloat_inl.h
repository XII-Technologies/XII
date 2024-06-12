#pragma once

XII_ALWAYS_INLINE xiiSimdFloat::xiiSimdFloat() {}

XII_ALWAYS_INLINE xiiSimdFloat::xiiSimdFloat(float f)
{
  m_v.Set(f);
}

XII_ALWAYS_INLINE xiiSimdFloat::xiiSimdFloat(xiiInt32 i)
{
  m_v.Set((float)i);
}

XII_ALWAYS_INLINE xiiSimdFloat::xiiSimdFloat(xiiUInt32 i)
{
  m_v.Set((float)i);
}

XII_ALWAYS_INLINE xiiSimdFloat::xiiSimdFloat(xiiAngle a)
{
  m_v.Set(a.GetRadian());
}

XII_ALWAYS_INLINE xiiSimdFloat::xiiSimdFloat(xiiInternal::QuadFloat v)
{
  m_v = v;
}

XII_ALWAYS_INLINE xiiSimdFloat::operator float() const
{
  return m_v.x;
}

// static
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::MakeZero()
{
  return xiiSimdFloat(0.0f);
}

// static
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::MakeNaN()
{
  return xiiSimdFloat(xiiMath::NaN<float>());
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::operator+(const xiiSimdFloat& f) const
{
  return m_v + f.m_v;
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::operator-(const xiiSimdFloat& f) const
{
  return m_v - f.m_v;
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::operator*(const xiiSimdFloat& f) const
{
  return m_v.CompMul(f.m_v);
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::operator/(const xiiSimdFloat& f) const
{
  return m_v.CompDiv(f.m_v);
}

XII_ALWAYS_INLINE xiiSimdFloat& xiiSimdFloat::operator+=(const xiiSimdFloat& f)
{
  m_v += f.m_v;
  return *this;
}

XII_ALWAYS_INLINE xiiSimdFloat& xiiSimdFloat::operator-=(const xiiSimdFloat& f)
{
  m_v -= f.m_v;
  return *this;
}

XII_ALWAYS_INLINE xiiSimdFloat& xiiSimdFloat::operator*=(const xiiSimdFloat& f)
{
  m_v = m_v.CompMul(f.m_v);
  return *this;
}

XII_ALWAYS_INLINE xiiSimdFloat& xiiSimdFloat::operator/=(const xiiSimdFloat& f)
{
  m_v = m_v.CompDiv(f.m_v);
  return *this;
}

XII_ALWAYS_INLINE bool xiiSimdFloat::IsEqual(const xiiSimdFloat& rhs, const xiiSimdFloat& fEpsilon) const
{
  return m_v.IsEqual(rhs.m_v, fEpsilon);
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator==(const xiiSimdFloat& f) const
{
  return m_v.x == f.m_v.x;
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator!=(const xiiSimdFloat& f) const
{
  return m_v.x != f.m_v.x;
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator>=(const xiiSimdFloat& f) const
{
  return m_v.x >= f.m_v.x;
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator>(const xiiSimdFloat& f) const
{
  return m_v.x > f.m_v.x;
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator<=(const xiiSimdFloat& f) const
{
  return m_v.x <= f.m_v.x;
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator<(const xiiSimdFloat& f) const
{
  return m_v.x < f.m_v.x;
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator==(float f) const
{
  return m_v.x == f;
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator!=(float f) const
{
  return m_v.x != f;
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator>(float f) const
{
  return m_v.x > f;
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator>=(float f) const
{
  return m_v.x >= f;
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator<(float f) const
{
  return m_v.x < f;
}

XII_ALWAYS_INLINE bool xiiSimdFloat::operator<=(float f) const
{
  return m_v.x <= f;
}

template <xiiMathFloatBits::Enum bits>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::GetReciprocal() const
{
  return xiiSimdFloat(1.0f / m_v.x);
}

template <xiiMathFloatBits::Enum bits>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::GetSqrt() const
{
  return xiiSimdFloat(xiiMath::Sqrt(m_v.x));
}

template <xiiMathFloatBits::Enum bits>
XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::GetInvSqrt() const
{
  return xiiSimdFloat(1.0f / xiiMath::Sqrt(m_v.x));
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::Max(const xiiSimdFloat& f) const
{
  return m_v.CompMax(f.m_v);
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::Min(const xiiSimdFloat& f) const
{
  return m_v.CompMin(f.m_v);
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdFloat::Abs() const
{
  return xiiSimdFloat(xiiMath::Abs(m_v.x));
}
