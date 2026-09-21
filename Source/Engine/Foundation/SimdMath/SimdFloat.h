/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Angle.h>
#include <Foundation/SimdMath/SimdTypes.h>

class XII_FOUNDATION_DLL xiiSimdFloat
{
public:
  XII_DECLARE_POD_TYPE();

  /// Default constructor, leaves the data uninitialized.
  xiiSimdFloat(); // [tested]

  /// Constructs from a given float.
  xiiSimdFloat(float f); // [tested]

  /// Constructs from a given integer.
  xiiSimdFloat(xiiInt32 i); // [tested]

  /// Constructs from a given integer.
  xiiSimdFloat(xiiUInt32 i); // [tested]

  /// Constructs from given angle.
  xiiSimdFloat(xiiAngle a); // [tested]

  /// Constructs from the internal implementation type.
  xiiSimdFloat(xiiInternal::QuadFloat v); // [tested]

  /// Returns the stored number as a standard float.
  operator float() const; // [tested]

  /// Creates a xiiSimdFloat that is initialized to zero.
  [[nodiscard]] static xiiSimdFloat MakeZero(); // [tested]

  /// Creates a xiiSimdFloat that is initialized to Not-A-Number (NaN).
  [[nodiscard]] static xiiSimdFloat MakeNaN(); // [tested]

public:
  xiiSimdFloat operator+(const xiiSimdFloat& f) const; // [tested]
  xiiSimdFloat operator-(const xiiSimdFloat& f) const; // [tested]
  xiiSimdFloat operator*(const xiiSimdFloat& f) const; // [tested]
  xiiSimdFloat operator/(const xiiSimdFloat& f) const; // [tested]

  xiiSimdFloat& operator+=(const xiiSimdFloat& f); // [tested]
  xiiSimdFloat& operator-=(const xiiSimdFloat& f); // [tested]
  xiiSimdFloat& operator*=(const xiiSimdFloat& f); // [tested]
  xiiSimdFloat& operator/=(const xiiSimdFloat& f); // [tested]

  bool IsEqual(const xiiSimdFloat& rhs, const xiiSimdFloat& fEpsilon) const;

  bool operator==(const xiiSimdFloat& f) const; // [tested]
  bool operator!=(const xiiSimdFloat& f) const; // [tested]
  bool operator>(const xiiSimdFloat& f) const;  // [tested]
  bool operator>=(const xiiSimdFloat& f) const; // [tested]
  bool operator<(const xiiSimdFloat& f) const;  // [tested]
  bool operator<=(const xiiSimdFloat& f) const; // [tested]

  bool operator==(float f) const; // [tested]
  bool operator!=(float f) const; // [tested]
  bool operator>(float f) const;  // [tested]
  bool operator>=(float f) const; // [tested]
  bool operator<(float f) const;  // [tested]
  bool operator<=(float f) const; // [tested]

  template <xiiMathFloatBits::Enum acc = xiiMathFloatBits::FULL>
  xiiSimdFloat GetReciprocal() const; // [tested]

  template <xiiMathFloatBits::Enum acc = xiiMathFloatBits::FULL>
  xiiSimdFloat GetSqrt() const; // [tested]

  template <xiiMathFloatBits::Enum acc = xiiMathFloatBits::FULL>
  xiiSimdFloat GetInvSqrt() const; // [tested]

  [[nodiscard]] xiiSimdFloat Max(const xiiSimdFloat& f) const; // [tested]
  [[nodiscard]] xiiSimdFloat Min(const xiiSimdFloat& f) const; // [tested]
  [[nodiscard]] xiiSimdFloat Abs() const;                      // [tested]

public:
  xiiInternal::QuadFloat m_v;
};

#if XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX || XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEFloat_inl.h>
#elif XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONFloat_inl.h>
#elif XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUFloat_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
