#pragma once

#include <Foundation/Math/Declarations.h>

/// \brief Implements fixed point arithmetic for fractional values.
///
/// Advantages over float and double are mostly that the computations are entirely integer-based and therefore
/// have a predictable (i.e. deterministic) result, independent from floating point settings, SSE support and
/// differences among CPUs.
/// Additionally fixed point arithmetic should be quite fast, compare to traditional floating point arithmetic
/// (not comparing it to SSE though).
/// With the template argument 'DecimalBits' you can specify how many bits are used for the fractional part.
/// I.e. a simple integer has zero DecimalBits. For a precision of about 1/1000 you need at least 10 DecimalBits
/// (1 << 10) == 1024.
/// Conversion between integer and fixed point is very fast (a shift), in contrast to float/xiiInt32 conversion.
///
/// If you are using xiiFixedPoint to get guaranteed deterministic behavior, you should minimize the usage of
/// xiiFixedPoint <-> float conversions. You can set xiiFixedPoint variables from float constants, but you should
/// never put data into xiiFixedPoint variables that was computed using floating point arithmetic (even if the
/// computations are simple and look harmless). Instead do all those computations with xiiFixedPoint variables.
template <xiiUInt8 DecimalBits>
class xiiFixedPoint
{
public:
  /// \brief Default constructor does not do any initialization.
  XII_ALWAYS_INLINE xiiFixedPoint() = default; // [tested]

  /// \brief Construct from an integer.
  /* implicit */ xiiFixedPoint(xiiInt32 iIntVal) { *this = iIntVal; } // [tested]

  /// \brief Construct from a float.
  /* implicit */ xiiFixedPoint(float fVal) { *this = fVal; } // [tested]

  /// \brief Construct from a double.
  /* implicit */ xiiFixedPoint(double fVal) { *this = fVal; } // [tested]

  /// \brief Assignment from an integer.
  const xiiFixedPoint<DecimalBits>& operator=(xiiInt32 iVal); // [tested]

  /// \brief Assignment from a float.
  const xiiFixedPoint<DecimalBits>& operator=(float fVal); // [tested]

  /// \brief Assignment from a double.
  const xiiFixedPoint<DecimalBits>& operator=(double fVal); // [tested]

  /// \brief Implicit conversion to xiiInt32 (the fractional part is dropped).
  xiiInt32 ToInt() const; // [tested]

  /// \brief Implicit conversion to float.
  float ToFloat() const; // [tested]

  /// \brief Implicit conversion to double.
  double ToDouble() const; // [tested]

  /// \brief 'Equality' comparison.
  bool operator==(const xiiFixedPoint<DecimalBits>& rhs) const { return m_iValue == rhs.m_iValue; } // [tested]

  /// \brief 'Less than' comparison.
  bool operator<(const xiiFixedPoint<DecimalBits>& rhs) const { return m_iValue < rhs.m_iValue; } // [tested]

  /// \brief 'Greater than' comparison.
  bool operator>(const xiiFixedPoint<DecimalBits>& rhs) const { return m_iValue > rhs.m_iValue; } // [tested]

  /// \brief 'Less than or equal' comparison.
  bool operator<=(const xiiFixedPoint<DecimalBits>& rhs) const { return m_iValue <= rhs.m_iValue; } // [tested]

  /// \brief 'Greater than or equal' comparison.
  bool operator>=(const xiiFixedPoint<DecimalBits>& rhs) const { return m_iValue >= rhs.m_iValue; } // [tested]


  const xiiFixedPoint<DecimalBits> operator-() const { return xiiFixedPoint<DecimalBits>(-m_iValue, true); }

  /// \brief += operator
  void operator+=(const xiiFixedPoint<DecimalBits>& rhs) { m_iValue += rhs.m_iValue; } // [tested]

  /// \brief -= operator
  void operator-=(const xiiFixedPoint<DecimalBits>& rhs) { m_iValue -= rhs.m_iValue; } // [tested]

  /// \brief *= operator
  void operator*=(const xiiFixedPoint<DecimalBits>& rhs); // [tested]

  /// \brief /= operator
  void operator/=(const xiiFixedPoint<DecimalBits>& rhs); // [tested]

  /// \brief *= operator with integers (more efficient)
  void operator*=(xiiInt32 rhs) { m_iValue *= rhs; } // [tested]

  /// \brief /= operator with integers (more efficient)
  void operator/=(xiiInt32 rhs) { m_iValue /= rhs; } // [tested]

  /// \brief Returns the underlying integer value. Mostly useful for serialization (or tests).
  xiiInt32 GetRawValue() const { return m_iValue; }

  /// \brief Sets the underlying integer value. Mostly useful for serialization (or tests).
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
#if 0
xiiFixedPoint operator+ (xiiFixedPoint, xiiFixedPoint); // [tested]
xiiFixedPoint operator- (xiiFixedPoint, xiiFixedPoint); // [tested]
xiiFixedPoint operator* (xiiFixedPoint, xiiFixedPoint); // [tested]
xiiFixedPoint operator/ (xiiFixedPoint, xiiFixedPoint); // [tested]
xiiFixedPoint operator* (xiiInt32, xiiFixedPoint); // [tested]
xiiFixedPoint operator* (xiiFixedPoint, xiiInt32); // [tested]
xiiFixedPoint operator/ (xiiFixedPoint, xiiInt32); // [tested]
#endif

#include <Foundation/Math/Implementation/FixedPoint_inl.h>
