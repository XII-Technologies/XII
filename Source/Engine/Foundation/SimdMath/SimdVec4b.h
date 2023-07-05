#pragma once

#include <Foundation/SimdMath/SimdSwizzle.h>
#include <Foundation/SimdMath/SimdTypes.h>

class XII_FOUNDATION_DLL xiiSimdVec4b
{
public:
  XII_DECLARE_POD_TYPE();

  xiiSimdVec4b();                               // [tested]
  xiiSimdVec4b(bool b);                         // [tested]
  xiiSimdVec4b(bool x, bool y, bool z, bool w); // [tested]
  xiiSimdVec4b(xiiInternal::QuadBool b);        // [tested]

public:
  template <xiiInt32 N>
  bool GetComponent() const; // [tested]

  bool x() const; // [tested]
  bool y() const; // [tested]
  bool z() const; // [tested]
  bool w() const; // [tested]

  template <xiiSwizzle::Enum s>
  xiiSimdVec4b Get() const; // [tested]

public:
  xiiSimdVec4b operator&&(const xiiSimdVec4b& rhs) const; // [tested]
  xiiSimdVec4b operator||(const xiiSimdVec4b& rhs) const; // [tested]
  xiiSimdVec4b operator!() const;                         // [tested]

  xiiSimdVec4b operator==(const xiiSimdVec4b& rhs) const; // [tested]
  xiiSimdVec4b operator!=(const xiiSimdVec4b& rhs) const; // [tested]

  template <xiiInt32 N = 4>
  bool AllSet() const; // [tested]

  template <xiiInt32 N = 4>
  bool AnySet() const; // [tested]

  template <xiiInt32 N = 4>
  bool NoneSet() const; // [tested]

  static xiiSimdVec4b Select(const xiiSimdVec4b& vCmp, const xiiSimdVec4b& vIfTrue, const xiiSimdVec4b& vIfFalse); // [tested]

public:
  xiiInternal::QuadBool m_v;
};

#if XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX || XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEVec4b_inl.h>
#elif XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONVec4b_inl.h>
#elif XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUVec4b_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
