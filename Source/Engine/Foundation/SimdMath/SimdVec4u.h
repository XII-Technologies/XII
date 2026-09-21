/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/SimdMath/SimdVec4i.h>

/// A SIMD 4-component vector class of unsigned 32b integers
class XII_FOUNDATION_DLL xiiSimdVec4u
{
public:
  XII_DECLARE_POD_TYPE();

  xiiSimdVec4u(); // [tested]

  explicit xiiSimdVec4u(xiiUInt32 uiXyzw); // [tested]

  xiiSimdVec4u(xiiUInt32 x, xiiUInt32 y, xiiUInt32 z, xiiUInt32 w = 1); // [tested]

  xiiSimdVec4u(xiiInternal::QuadUInt v); // [tested]

  /// Creates a xiiSimdVec4u that is initialized to zero.
  [[nodiscard]] static xiiSimdVec4u MakeZero(); // [tested]

  void Set(xiiUInt32 uiXyzw); // [tested]

  void Set(xiiUInt32 x, xiiUInt32 y, xiiUInt32 z, xiiUInt32 w); // [tested]

  void SetZero(); // [tested]

public:
  explicit xiiSimdVec4u(const xiiSimdVec4i& i); // [tested]

public:
  xiiSimdVec4f ToFloat() const; // [tested]

  [[nodiscard]] static xiiSimdVec4u Truncate(const xiiSimdVec4f& f); // [tested]

public:
  template <xiiInt32 N>
  xiiUInt32 GetComponent() const; // [tested]

  xiiUInt32 x() const; // [tested]
  xiiUInt32 y() const; // [tested]
  xiiUInt32 z() const; // [tested]
  xiiUInt32 w() const; // [tested]

  template <xiiSwizzle::Enum s>
  xiiSimdVec4u Get() const; // [tested]

public:
  [[nodiscard]] xiiSimdVec4u operator+(const xiiSimdVec4u& v) const; // [tested]
  [[nodiscard]] xiiSimdVec4u operator-(const xiiSimdVec4u& v) const; // [tested]

  [[nodiscard]] xiiSimdVec4u CompMul(const xiiSimdVec4u& v) const; // [tested]

  [[nodiscard]] xiiSimdVec4u operator|(const xiiSimdVec4u& v) const; // [tested]
  [[nodiscard]] xiiSimdVec4u operator&(const xiiSimdVec4u& v) const; // [tested]
  [[nodiscard]] xiiSimdVec4u operator^(const xiiSimdVec4u& v) const; // [tested]
  [[nodiscard]] xiiSimdVec4u operator~() const;                      // [tested]

  [[nodiscard]] xiiSimdVec4u operator<<(xiiUInt32 uiShift) const; // [tested]
  [[nodiscard]] xiiSimdVec4u operator>>(xiiUInt32 uiShift) const; // [tested]

  xiiSimdVec4u& operator+=(const xiiSimdVec4u& v); // [tested]
  xiiSimdVec4u& operator-=(const xiiSimdVec4u& v); // [tested]

  xiiSimdVec4u& operator|=(const xiiSimdVec4u& v); // [tested]
  xiiSimdVec4u& operator&=(const xiiSimdVec4u& v); // [tested]
  xiiSimdVec4u& operator^=(const xiiSimdVec4u& v); // [tested]

  xiiSimdVec4u& operator<<=(xiiUInt32 uiShift); // [tested]
  xiiSimdVec4u& operator>>=(xiiUInt32 uiShift); // [tested]

  [[nodiscard]] xiiSimdVec4u CompMin(const xiiSimdVec4u& v) const; // [tested]
  [[nodiscard]] xiiSimdVec4u CompMax(const xiiSimdVec4u& v) const; // [tested]

  xiiSimdVec4b operator==(const xiiSimdVec4u& v) const; // [tested]
  xiiSimdVec4b operator!=(const xiiSimdVec4u& v) const; // [tested]
  xiiSimdVec4b operator<=(const xiiSimdVec4u& v) const; // [tested]
  xiiSimdVec4b operator<(const xiiSimdVec4u& v) const;  // [tested]
  xiiSimdVec4b operator>=(const xiiSimdVec4u& v) const; // [tested]
  xiiSimdVec4b operator>(const xiiSimdVec4u& v) const;  // [tested]

public:
  xiiInternal::QuadUInt m_v;
};

#if XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX || XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEVec4u_inl.h>
#elif XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONVec4u_inl.h>
#elif XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUVec4u_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
