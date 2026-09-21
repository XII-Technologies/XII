/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Declarations.h>

/// Implements fixed point arithmetic for fractional values.
///
/// Advantages over float and double are mostly that the computations are entirely integer-based and therefore
/// have a predictable (i.e. deterministic) result, independent from floating point settings, SSE support and
/// differences among CPUs.
/// Additionally fixed point arithmetic should be quite fast, compare to traditional floating point arithmetic
/// (not comparing it to SSE though).
/// With the template argument 'DecimalBits' you can specify how many bits are used for the fractional part.
/// I.e. a simple integer has zero DecimalBits. For a precision of about 1/1000 you need at least 10 DecimalBits
/// (1 << 10) == 1024.
/// Conversion between integer and fixed point is very fast (a shift), in contrast to float/int conversion.
///
/// If you are using xiiFixedPoint to get guaranteed deterministic behavior, you should minimize the usage of
/// xiiFixedPoint <-> float conversions. You can set xiiFixedPoint variables from float constants, but you should
/// never put data into xiiFixedPoint variables that was computed using floating point arithmetic (even if the
/// computations are simple and look harmless). Instead do all those computations with xiiFixedPoint variables.
template <xiiUInt8 DecimalBits>
class xiiFixedPoint
{
public:
  /// Default constructor does not do any initialization.
  XII_ALWAYS_INLINE xiiFixedPoint() = default; // [tested]

  /// Construct from an integer.
  /* implicit */ xiiFixedPoint(xiiInt32 iIntVal) { *this = iIntVal; } // [tested]

  /// Construct from a float.
  /* implicit */ xiiFixedPoint(float fVal) { *this = fVal; } // [tested]

  /// Construct from a double.
  /* implicit */ xiiFixedPoint(double fVal) { *this = fVal; } // [tested]

  /// Assignment from an integer.
  const xiiFixedPoint<DecimalBits>& operator=(xiiInt32 iVal); // [tested]

  /// Assignment from a float.
  const xiiFixedPoint<DecimalBits>& operator=(float fVal); // [tested]

  /// Assignment from a double.
  const xiiFixedPoint<DecimalBits>& operator=(double fVal); // [tested]

  /// Implicit conversion to int (the fractional part is dropped).
  xiiInt32 ToInt() const; // [tested]

  /// Implicit conversion to float.
  float ToFloat() const; // [tested]

  /// Implicit conversion to double.
  double ToDouble() const; // [tested]

  /// 'Equality' comparison.
  bool operator==(const xiiFixedPoint<DecimalBits>& rhs) const { return m_iValue == rhs.m_iValue; } // [tested]

  /// Comparison operator.
  std::strong_ordering operator<=>(const xiiFixedPoint<DecimalBits>& rhs) const { return m_iValue <=> rhs.m_iValue; } // [tested]

  const xiiFixedPoint<DecimalBits> operator-() const { return xiiFixedPoint<DecimalBits>(-m_iValue, true); }

  /// += operator
  void operator+=(const xiiFixedPoint<DecimalBits>& rhs) { m_iValue += rhs.m_iValue; } // [tested]

  /// -= operator
  void operator-=(const xiiFixedPoint<DecimalBits>& rhs) { m_iValue -= rhs.m_iValue; } // [tested]

  /// *= operator
  void operator*=(const xiiFixedPoint<DecimalBits>& rhs); // [tested]

  /// /= operator
  void operator/=(const xiiFixedPoint<DecimalBits>& rhs); // [tested]

  /// *= operator with integers (more efficient)
  void operator*=(xiiInt32 rhs) { m_iValue *= rhs; } // [tested]

  /// /= operator with integers (more efficient)
  void operator/=(xiiInt32 rhs) { m_iValue /= rhs; } // [tested]

  /// Returns the underlying integer value. Mostly useful for serialization (or tests).
  xiiInt32 GetRawValue() const { return m_iValue; }

  /// Sets the underlying integer value. Mostly useful for serialization (or tests).
  void SetRawValue(xiiInt32 iVal) { m_iValue = iVal; }

private:
  xiiInt32 m_iValue;
};

template <xiiUInt8 DecimalBits>
float ToFloat(xiiFixedPoint<DecimalBits> f)
{
  return f.ToFloat();
}

// Additional operators:
// xiiFixedPoint operator+ (xiiFixedPoint, xiiFixedPoint); // [tested]
// xiiFixedPoint operator- (xiiFixedPoint, xiiFixedPoint); // [tested]
// xiiFixedPoint operator* (xiiFixedPoint, xiiFixedPoint); // [tested]
// xiiFixedPoint operator/ (xiiFixedPoint, xiiFixedPoint); // [tested]
// xiiFixedPoint operator* (int, xiiFixedPoint); // [tested]
// xiiFixedPoint operator* (xiiFixedPoint, int); // [tested]
// xiiFixedPoint operator/ (xiiFixedPoint, int); // [tested]

#include <Foundation/Math/Implementation/FixedPoint_inl.h>
