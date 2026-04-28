/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Math/Math.h>

template <xiiUInt8 DecimalBits>
const xiiFixedPoint<DecimalBits>& xiiFixedPoint<DecimalBits>::operator=(xiiInt32 iVal)
{
  m_iValue = iVal << DecimalBits;
  return *this;
}

template <xiiUInt8 DecimalBits>
const xiiFixedPoint<DecimalBits>& xiiFixedPoint<DecimalBits>::operator=(float fVal)
{
  m_iValue = (xiiInt32)xiiMath::Round(fVal * (1 << DecimalBits));
  return *this;
}

template <xiiUInt8 DecimalBits>
const xiiFixedPoint<DecimalBits>& xiiFixedPoint<DecimalBits>::operator=(double fVal)
{
  m_iValue = (xiiInt32)xiiMath::Round(fVal * (1 << DecimalBits));
  return *this;
}

template <xiiUInt8 DecimalBits>
xiiInt32 xiiFixedPoint<DecimalBits>::ToInt() const
{
  return (xiiInt32)(m_iValue >> DecimalBits);
}

template <xiiUInt8 DecimalBits>
float xiiFixedPoint<DecimalBits>::ToFloat() const
{
  return (float)((double)m_iValue / (double)(1 << DecimalBits));
}

template <xiiUInt8 DecimalBits>
double xiiFixedPoint<DecimalBits>::ToDouble() const
{
  return ((double)m_iValue / (double)(1 << DecimalBits));
}

template <xiiUInt8 DecimalBits>
void xiiFixedPoint<DecimalBits>::operator*=(const xiiFixedPoint<DecimalBits>& rhs)
{
  // lhs and rhs are in N:M format (N Bits for the Integer part, M Bits for the fractional part)
  // after multiplication, it will be in 2N:2M format

  const xiiInt64 TempLHS = m_iValue;
  const xiiInt64 TempRHS = rhs.m_iValue;

  xiiInt64 TempRes = TempLHS * TempRHS;

  // the lower DecimalBits Bits are nearly of no concern (we throw them away anyway), except for the upper most Bit
  // that is Bit '(DecimalBits - 1)' and its Bitmask is therefore '(1 << (DecimalBits - 1))'
  // If that Bit is set, then the lowest DecimalBits represent a value of more than '0.5' (of their range)
  // so '(TempRes & (1 << (DecimalBits - 1))) ' is either 0 or 1 depending on whether the lower DecimalBits Bits represent a value larger than 0.5 or
  // not we shift that Bit one to the left and add it to the original value and thus 'round up' the result
  TempRes += ((TempRes & (1 << (DecimalBits - 1))) << 1);

  TempRes >>= DecimalBits; // result format: 2N:M

  // the upper N Bits are thrown away during conversion from 64 Bit to 32 Bit
  m_iValue = (xiiInt32)TempRes;
}

template <xiiUInt8 DecimalBits>
void xiiFixedPoint<DecimalBits>::operator/=(const xiiFixedPoint<DecimalBits>& rhs)
{
  xiiInt64       TempLHS = m_iValue;
  const xiiInt64 TempRHS = rhs.m_iValue;

  TempLHS <<= 31;

  xiiInt64 TempRes = TempLHS / TempRHS;

  // same rounding concept as in multiplication
  TempRes += ((TempRes & (1 << (31 - DecimalBits - 1))) << 1);

  TempRes >>= (31 - DecimalBits);

  // here we throw away the upper 32 Bits again (not needed anymore)
  m_iValue = (xiiInt32)TempRes;
}


template <xiiUInt8 DecimalBits>
xiiFixedPoint<DecimalBits> operator+(const xiiFixedPoint<DecimalBits>& lhs, const xiiFixedPoint<DecimalBits>& rhs)
{
  xiiFixedPoint<DecimalBits> res = lhs;
  res += rhs;
  return res;
}

template <xiiUInt8 DecimalBits>
xiiFixedPoint<DecimalBits> operator-(const xiiFixedPoint<DecimalBits>& lhs, const xiiFixedPoint<DecimalBits>& rhs)
{
  xiiFixedPoint<DecimalBits> res = lhs;
  res -= rhs;
  return res;
}

template <xiiUInt8 DecimalBits>
xiiFixedPoint<DecimalBits> operator*(const xiiFixedPoint<DecimalBits>& lhs, const xiiFixedPoint<DecimalBits>& rhs)
{
  xiiFixedPoint<DecimalBits> res = lhs;
  res *= rhs;
  return res;
}

template <xiiUInt8 DecimalBits>
xiiFixedPoint<DecimalBits> operator/(const xiiFixedPoint<DecimalBits>& lhs, const xiiFixedPoint<DecimalBits>& rhs)
{
  xiiFixedPoint<DecimalBits> res = lhs;
  res /= rhs;
  return res;
}


template <xiiUInt8 DecimalBits>
xiiFixedPoint<DecimalBits> operator*(const xiiFixedPoint<DecimalBits>& lhs, xiiInt32 rhs)
{
  xiiFixedPoint<DecimalBits> ret = lhs;
  ret *= rhs;
  return ret;
}

template <xiiUInt8 DecimalBits>
xiiFixedPoint<DecimalBits> operator*(xiiInt32 lhs, const xiiFixedPoint<DecimalBits>& rhs)
{
  xiiFixedPoint<DecimalBits> ret = rhs;
  ret *= lhs;
  return ret;
}

template <xiiUInt8 DecimalBits>
xiiFixedPoint<DecimalBits> operator/(const xiiFixedPoint<DecimalBits>& lhs, xiiInt32 rhs)
{
  xiiFixedPoint<DecimalBits> ret = lhs;
  ret /= rhs;
  return ret;
}
