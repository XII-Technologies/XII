/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/SimdMath/SimdFloat.h>
#include <Foundation/SimdMath/SimdVec4b.h>

/// A 4-component SIMD vector class
class XII_FOUNDATION_DLL xiiSimdVec4f
{
public:
  XII_DECLARE_POD_TYPE();

  xiiSimdVec4f(); // [tested]

  explicit xiiSimdVec4f(float fXyzw); // [tested]

  explicit xiiSimdVec4f(const xiiSimdFloat& fXyzw); // [tested]

  xiiSimdVec4f(float x, float y, float z, float w = 1.0f); // [tested]

  xiiSimdVec4f(xiiInternal::QuadFloat v); // [tested]

  /// Creates a xiiSimdVec4f that is initialized to zero.
  [[nodiscard]] static xiiSimdVec4f MakeZero(); // [tested]

  /// Creates a xiiSimdVec4f that is initialized to Not-A-Number (NaN).
  [[nodiscard]] static xiiSimdVec4f MakeNaN(); // [tested]

  void Set(float fXyzw); // [tested]

  void Set(float x, float y, float z, float w); // [tested]

  void SetX(const xiiSimdFloat& f); // [tested]
  void SetY(const xiiSimdFloat& f); // [tested]
  void SetZ(const xiiSimdFloat& f); // [tested]
  void SetW(const xiiSimdFloat& f); // [tested]

  void SetZero(); // [tested]

  template <xiiInt32 N>
  void Load(const float* pFloats); // [tested]

  template <xiiInt32 N>
  void Store(float* pFloats) const; // [tested]

public:
  template <xiiMathFloatBits::Enum acc = xiiMathFloatBits::FULL>
  xiiSimdVec4f GetReciprocal() const; // [tested]

  template <xiiMathFloatBits::Enum acc = xiiMathFloatBits::FULL>
  xiiSimdVec4f GetSqrt() const; // [tested]

  template <xiiMathFloatBits::Enum acc = xiiMathFloatBits::FULL>
  xiiSimdVec4f GetInvSqrt() const; // [tested]

  template <xiiInt32 N, xiiMathFloatBits::Enum acc = xiiMathFloatBits::FULL>
  xiiSimdFloat GetLength() const; // [tested]

  template <xiiInt32 N, xiiMathFloatBits::Enum acc = xiiMathFloatBits::FULL>
  xiiSimdFloat GetInvLength() const; // [tested]

  template <xiiInt32 N>
  xiiSimdFloat GetLengthSquared() const; // [tested]

  template <xiiInt32 N, xiiMathFloatBits::Enum acc = xiiMathFloatBits::FULL>
  xiiSimdFloat GetLengthAndNormalize(); // [tested]

  template <xiiInt32 N, xiiMathFloatBits::Enum acc = xiiMathFloatBits::FULL>
  xiiSimdVec4f GetNormalized() const; // [tested]

  template <xiiInt32 N, xiiMathFloatBits::Enum acc = xiiMathFloatBits::FULL>
  void Normalize(); // [tested]

  template <xiiInt32 N, xiiMathFloatBits::Enum acc = xiiMathFloatBits::FULL>
  void NormalizeIfNotZero(const xiiSimdFloat& fEpsilon = xiiMath::SmallEpsilon<float>()); // [tested]

  template <xiiInt32 N>
  bool IsZero() const; // [tested]

  template <xiiInt32 N>
  bool IsZero(const xiiSimdFloat& fEpsilon) const; // [tested]

  template <xiiInt32 N>
  bool IsNormalized(const xiiSimdFloat& fEpsilon = xiiMath::HugeEpsilon<float>()) const; // [tested]

  template <xiiInt32 N>
  bool IsNaN() const; // [tested]

  template <xiiInt32 N>
  bool IsValid() const; // [tested]

public:
  template <xiiInt32 N>
  xiiSimdFloat GetComponent() const; // [tested]

  xiiSimdFloat GetComponent(xiiInt32 i) const; // [tested]

  xiiSimdFloat x() const; // [tested]
  xiiSimdFloat y() const; // [tested]
  xiiSimdFloat z() const; // [tested]
  xiiSimdFloat w() const; // [tested]

  template <xiiSwizzle::Enum s>
  xiiSimdVec4f Get() const; // [tested]

  ///x = this[s0], y = this[s1], z = other[s2], w = other[s3]
  template <xiiSwizzle::Enum s>
  [[nodiscard]] xiiSimdVec4f GetCombined(const xiiSimdVec4f& other) const; // [tested]

public:
  [[nodiscard]] xiiSimdVec4f operator-() const;                      // [tested]
  [[nodiscard]] xiiSimdVec4f operator+(const xiiSimdVec4f& v) const; // [tested]
  [[nodiscard]] xiiSimdVec4f operator-(const xiiSimdVec4f& v) const; // [tested]

  [[nodiscard]] xiiSimdVec4f operator*(const xiiSimdFloat& f) const; // [tested]
  [[nodiscard]] xiiSimdVec4f operator/(const xiiSimdFloat& f) const; // [tested]

  [[nodiscard]] xiiSimdVec4f CompMul(const xiiSimdVec4f& v) const; // [tested]

  template <xiiMathFloatBits::Enum acc = xiiMathFloatBits::FULL>
  [[nodiscard]] xiiSimdVec4f CompDiv(const xiiSimdVec4f& v) const; // [tested]

  [[nodiscard]] xiiSimdVec4f CompMin(const xiiSimdVec4f& rhs) const; // [tested]
  [[nodiscard]] xiiSimdVec4f CompMax(const xiiSimdVec4f& rhs) const; // [tested]

  [[nodiscard]] xiiSimdVec4f Abs() const;      // [tested]
  [[nodiscard]] xiiSimdVec4f Round() const;    // [tested]
  [[nodiscard]] xiiSimdVec4f Floor() const;    // [tested]
  [[nodiscard]] xiiSimdVec4f Ceil() const;     // [tested]
  [[nodiscard]] xiiSimdVec4f Trunc() const;    // [tested]
  [[nodiscard]] xiiSimdVec4f Fraction() const; // [tested]

  [[nodiscard]] xiiSimdVec4f FlipSign(const xiiSimdVec4b& vCmp) const; // [tested]

  [[nodiscard]] static xiiSimdVec4f Select(const xiiSimdVec4b& vCmp, const xiiSimdVec4f& vTrue, const xiiSimdVec4f& vFalse); // [tested]

  [[nodiscard]] static xiiSimdVec4f Lerp(const xiiSimdVec4f& a, const xiiSimdVec4f& b, const xiiSimdVec4f& t);

  xiiSimdVec4f& operator+=(const xiiSimdVec4f& v); // [tested]
  xiiSimdVec4f& operator-=(const xiiSimdVec4f& v); // [tested]

  xiiSimdVec4f& operator*=(const xiiSimdFloat& f); // [tested]
  xiiSimdVec4f& operator/=(const xiiSimdFloat& f); // [tested]

  xiiSimdVec4b IsEqual(const xiiSimdVec4f& rhs, const xiiSimdFloat& fEpsilon) const; // [tested]

  [[nodiscard]] xiiSimdVec4b operator==(const xiiSimdVec4f& v) const; // [tested]
  [[nodiscard]] xiiSimdVec4b operator!=(const xiiSimdVec4f& v) const; // [tested]
  [[nodiscard]] xiiSimdVec4b operator<=(const xiiSimdVec4f& v) const; // [tested]
  [[nodiscard]] xiiSimdVec4b operator<(const xiiSimdVec4f& v) const;  // [tested]
  [[nodiscard]] xiiSimdVec4b operator>=(const xiiSimdVec4f& v) const; // [tested]
  [[nodiscard]] xiiSimdVec4b operator>(const xiiSimdVec4f& v) const;  // [tested]

  template <xiiInt32 N>
  [[nodiscard]] xiiSimdFloat HorizontalSum() const; // [tested]

  template <xiiInt32 N>
  [[nodiscard]] xiiSimdFloat HorizontalMin() const; // [tested]

  template <xiiInt32 N>
  [[nodiscard]] xiiSimdFloat HorizontalMax() const; // [tested]

  template <xiiInt32 N>
  [[nodiscard]] xiiSimdFloat Dot(const xiiSimdVec4f& v) const; // [tested]

  ///3D cross product, w is ignored.
  [[nodiscard]] xiiSimdVec4f CrossRH(const xiiSimdVec4f& v) const; // [tested]

  ///Generates an arbitrary vector such that Dot<3>(GetOrthogonalVector()) == 0
  [[nodiscard]] xiiSimdVec4f GetOrthogonalVector() const; // [tested]

  [[nodiscard]] static xiiSimdVec4f MulAdd(const xiiSimdVec4f& a, const xiiSimdVec4f& b, const xiiSimdVec4f& c); // [tested]
  [[nodiscard]] static xiiSimdVec4f MulAdd(const xiiSimdVec4f& a, const xiiSimdFloat& b, const xiiSimdVec4f& c); // [tested]

  [[nodiscard]] static xiiSimdVec4f MulSub(const xiiSimdVec4f& a, const xiiSimdVec4f& b, const xiiSimdVec4f& c); // [tested]
  [[nodiscard]] static xiiSimdVec4f MulSub(const xiiSimdVec4f& a, const xiiSimdFloat& b, const xiiSimdVec4f& c); // [tested]

  [[nodiscard]] static xiiSimdVec4f CopySign(const xiiSimdVec4f& vMagnitude, const xiiSimdVec4f& vSign); // [tested]

public:
  xiiInternal::QuadFloat m_v;
};

#include <Foundation/SimdMath/Implementation/SimdVec4f_inl.h>

#if XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX || XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEVec4f_inl.h>
#elif XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONVec4f_inl.h>
#elif XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUVec4f_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
