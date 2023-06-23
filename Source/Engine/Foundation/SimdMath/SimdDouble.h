#pragma once

#include <Foundation/Math/Angle.h>
#include <Foundation/SimdMath/SimdTypes.h>

class XII_FOUNDATION_DLL xiiSimdDouble
{
public:
  XII_DECLARE_POD_TYPE();

  /// \brief Default constructor, leaves the data uninitialized.
  xiiSimdDouble(); // [tested]

  /// \brief Constructs from a given double.
  xiiSimdDouble(double f); // [tested]

  /// \brief Constructs from a given float.
  xiiSimdDouble(float f); // [tested]

  /// \brief Constructs from a given integer.
  xiiSimdDouble(xiiInt32 i); // [tested]

  /// \brief Constructs from a given integer.
  xiiSimdDouble(xiiUInt32 i); // [tested]

  /// \brief Constructs from given angle.
  xiiSimdDouble(xiiAngled a); // [tested]

  /// \brief Constructs from the internal implementation type.
  xiiSimdDouble(xiiInternal::QuadDouble v); // [tested]

  /// \brief Returns the stored number as a standard double.
  operator double() const; // [tested]

  static xiiSimdDouble Zero(); // [tested]

public:
  xiiSimdDouble operator+(const xiiSimdDouble& f) const; // [tested]
  xiiSimdDouble operator-(const xiiSimdDouble& f) const; // [tested]
  xiiSimdDouble operator*(const xiiSimdDouble& f) const; // [tested]
  xiiSimdDouble operator/(const xiiSimdDouble& f) const; // [tested]

  xiiSimdDouble& operator+=(const xiiSimdDouble& f); // [tested]
  xiiSimdDouble& operator-=(const xiiSimdDouble& f); // [tested]
  xiiSimdDouble& operator*=(const xiiSimdDouble& f); // [tested]
  xiiSimdDouble& operator/=(const xiiSimdDouble& f); // [tested]

  bool IsEqual(const xiiSimdDouble& rhs, const xiiSimdDouble& fEpsilon) const;

  bool operator==(const xiiSimdDouble& f) const; // [tested]
  bool operator!=(const xiiSimdDouble& f) const; // [tested]
  bool operator>(const xiiSimdDouble& f) const;  // [tested]
  bool operator>=(const xiiSimdDouble& f) const; // [tested]
  bool operator<(const xiiSimdDouble& f) const;  // [tested]
  bool operator<=(const xiiSimdDouble& f) const; // [tested]

  bool operator==(double f) const; // [tested]
  bool operator!=(double f) const; // [tested]
  bool operator>(double f) const;  // [tested]
  bool operator>=(double f) const; // [tested]
  bool operator<(double f) const;  // [tested]
  bool operator<=(double f) const; // [tested]

  template <xiiMathDoubleBits::Enum acc = xiiMathDoubleBits::FULL>
  xiiSimdDouble GetReciprocal() const; // [tested]

  template <xiiMathDoubleBits::Enum acc = xiiMathDoubleBits::FULL>
  xiiSimdDouble GetSqrt() const; // [tested]

  template <xiiMathDoubleBits::Enum acc = xiiMathDoubleBits::FULL>
  xiiSimdDouble GetInvSqrt() const; // [tested]

  xiiSimdDouble Max(const xiiSimdDouble& f) const; // [tested]
  xiiSimdDouble Min(const xiiSimdDouble& f) const; // [tested]
  xiiSimdDouble Abs() const;                       // [tested]

public:
  xiiInternal::QuadDouble m_v;
};

#if XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEDouble_inl.h>
#elif XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUDouble_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
