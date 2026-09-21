/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Angle.h>
#include <Foundation/SimdMath/SimdTypes.h>

class XII_FOUNDATION_DLL xiiSimdDouble
{
public:
  XII_DECLARE_POD_TYPE();

  /// Default constructor, leaves the data uninitialized.
  xiiSimdDouble(); // [tested]

  /// Constructs from a given double.
  xiiSimdDouble(double f); // [tested]

  /// Constructs from a given float.
  xiiSimdDouble(float f); // [tested]

  /// Constructs from a given integer.
  xiiSimdDouble(xiiInt32 i); // [tested]

  /// Constructs from a given integer.
  xiiSimdDouble(xiiUInt32 i); // [tested]

  /// Constructs from given angle.
  xiiSimdDouble(xiiAngled a); // [tested]

  /// Constructs from the internal implementation type.
  xiiSimdDouble(xiiInternal::QuadDouble v); // [tested]

  /// Returns the stored number as a standard double.
  operator double() const; // [tested]

  /// Creates a xiiSimdDouble that is initialized to zero.
  [[nodiscard]] static xiiSimdDouble MakeZero(); // [tested]

  /// Creates a xiiSimdDouble that is initialized to Not-A-Number (NaN).
  [[nodiscard]] static xiiSimdDouble MakeNaN(); // [tested]

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

  [[nodiscard]] xiiSimdDouble Max(const xiiSimdDouble& f) const; // [tested]
  [[nodiscard]] xiiSimdDouble Min(const xiiSimdDouble& f) const; // [tested]
  [[nodiscard]] xiiSimdDouble Abs() const;                       // [tested]

public:
  xiiInternal::QuadDouble m_v;
};

#if XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX
#  include <Foundation/SimdMath/Implementation/AVX/AVXDouble_inl.h>
#elif XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE || XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_NEON || XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUDouble_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
