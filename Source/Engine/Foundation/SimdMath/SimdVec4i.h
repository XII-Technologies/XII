#pragma once

#include <Foundation/SimdMath/SimdVec4f.h>

class xiiSimdVec4u;

/// \brief A SIMD 4-component vector class of signed 32b integers
class XII_FOUNDATION_DLL xiiSimdVec4i
{
public:
  XII_DECLARE_POD_TYPE();

  xiiSimdVec4i(); // [tested]

  explicit xiiSimdVec4i(xiiInt32 iXyzw); // [tested]

  xiiSimdVec4i(xiiInt32 x, xiiInt32 y, xiiInt32 z, xiiInt32 w = 1); // [tested]

  xiiSimdVec4i(xiiInternal::QuadInt v); // [tested]

  void Set(xiiInt32 iXyzw); // [tested]

  void Set(xiiInt32 x, xiiInt32 y, xiiInt32 z, xiiInt32 w); // [tested]

  void SetZero(); // [tested]

  template <xiiInt32 N>
  void Load(const xiiInt32* pInts); // [tested]

  template <xiiInt32 N>
  void Store(xiiInt32* pInts) const; // [tested]

public:
  explicit xiiSimdVec4i(const xiiSimdVec4u& u); // [tested]

public:
  xiiSimdVec4f ToFloat() const; // [tested]

  static xiiSimdVec4i Truncate(const xiiSimdVec4f& f); // [tested]

public:
  template <int N>
  xiiInt32 GetComponent() const; // [tested]

  xiiInt32 x() const; // [tested]
  xiiInt32 y() const; // [tested]
  xiiInt32 z() const; // [tested]
  xiiInt32 w() const; // [tested]

  template <xiiSwizzle::Enum s>
  xiiSimdVec4i Get() const; // [tested]

public:
  xiiSimdVec4i operator-() const;                      // [tested]
  xiiSimdVec4i operator+(const xiiSimdVec4i& v) const; // [tested]
  xiiSimdVec4i operator-(const xiiSimdVec4i& v) const; // [tested]

  xiiSimdVec4i CompMul(const xiiSimdVec4i& v) const; // [tested]
  xiiSimdVec4i CompDiv(const xiiSimdVec4i& v) const; // [tested]

  xiiSimdVec4i operator|(const xiiSimdVec4i& v) const; // [tested]
  xiiSimdVec4i operator&(const xiiSimdVec4i& v) const; // [tested]
  xiiSimdVec4i operator^(const xiiSimdVec4i& v) const; // [tested]
  xiiSimdVec4i operator~() const;                      // [tested]

  xiiSimdVec4i operator<<(xiiUInt32 uiShift) const;     // [tested]
  xiiSimdVec4i operator>>(xiiUInt32 uiShift) const;     // [tested]
  xiiSimdVec4i operator<<(const xiiSimdVec4i& v) const; // [tested]
  xiiSimdVec4i operator>>(const xiiSimdVec4i& v) const; // [tested]

  xiiSimdVec4i& operator+=(const xiiSimdVec4i& v); // [tested]
  xiiSimdVec4i& operator-=(const xiiSimdVec4i& v); // [tested]

  xiiSimdVec4i& operator|=(const xiiSimdVec4i& v); // [tested]
  xiiSimdVec4i& operator&=(const xiiSimdVec4i& v); // [tested]
  xiiSimdVec4i& operator^=(const xiiSimdVec4i& v); // [tested]

  xiiSimdVec4i& operator<<=(xiiUInt32 uiShift); // [tested]
  xiiSimdVec4i& operator>>=(xiiUInt32 uiShift); // [tested]

  xiiSimdVec4i CompMin(const xiiSimdVec4i& v) const; // [tested]
  xiiSimdVec4i CompMax(const xiiSimdVec4i& v) const; // [tested]
  xiiSimdVec4i Abs() const;                          // [tested]

  xiiSimdVec4b operator==(const xiiSimdVec4i& v) const; // [tested]
  xiiSimdVec4b operator!=(const xiiSimdVec4i& v) const; // [tested]
  xiiSimdVec4b operator<=(const xiiSimdVec4i& v) const; // [tested]
  xiiSimdVec4b operator<(const xiiSimdVec4i& v) const;  // [tested]
  xiiSimdVec4b operator>=(const xiiSimdVec4i& v) const; // [tested]
  xiiSimdVec4b operator>(const xiiSimdVec4i& v) const;  // [tested]

  static xiiSimdVec4i ZeroVector(); // [tested]

  static xiiSimdVec4i Select(const xiiSimdVec4b& vCmp, const xiiSimdVec4i& vIfTrue, const xiiSimdVec4i& vIfFalse); // [tested]

public:
  xiiInternal::QuadInt m_v;
};

#if XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX || XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEVec4i_inl.h>
#elif XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONVec4i_inl.h>
#elif XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUVec4i_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
