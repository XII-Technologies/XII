#pragma once

#include <Foundation/SimdMath/SimdDouble.h>
#include <Foundation/SimdMath/SimdVec4b.h>

/// \brief A 4-component SIMD vector class
class XII_FOUNDATION_DLL xiiSimdVec4d
{
public:
  XII_DECLARE_POD_TYPE();

  xiiSimdVec4d(); // [tested]

  explicit xiiSimdVec4d(double xyzw); // [tested]

  explicit xiiSimdVec4d(const xiiSimdDouble& xyzw); // [tested]

  xiiSimdVec4d(double x, double y, double z, double w = 1.0); // [tested]

  xiiSimdVec4d(xiiInternal::QuadDouble v); // [tested]

  void Set(double xyzw); // [tested]

  void Set(double x, double y, double z, double w); // [tested]

  void SetX(const xiiSimdDouble& f); // [tested]
  void SetY(const xiiSimdDouble& f); // [tested]
  void SetZ(const xiiSimdDouble& f); // [tested]
  void SetW(const xiiSimdDouble& f); // [tested]

  void SetZero(); // [tested]

  template <xiiInt32 N>
  void Load(const double* pValues); // [tested]

  template <xiiInt32 N>
  void Store(double* pValues) const; // [tested]

public:
  template <xiiMathDoubleBits::Enum acc = xiiMathDoubleBits::FULL>
  xiiSimdVec4d GetReciprocal() const; // [tested]

  template <xiiMathDoubleBits::Enum acc = xiiMathDoubleBits::FULL>
  xiiSimdVec4d GetSqrt() const; // [tested]

  template <xiiMathDoubleBits::Enum acc = xiiMathDoubleBits::FULL>
  xiiSimdVec4d GetInvSqrt() const; // [tested]

  template <xiiInt32 N, xiiMathDoubleBits::Enum acc = xiiMathDoubleBits::FULL>
  xiiSimdDouble GetLength() const; // [tested]

  template <xiiInt32 N, xiiMathDoubleBits::Enum acc = xiiMathDoubleBits::FULL>
  xiiSimdDouble GetInvLength() const; // [tested]

  template <xiiInt32 N>
  xiiSimdDouble GetLengthSquared() const; // [tested]

  template <xiiInt32 N, xiiMathDoubleBits::Enum acc = xiiMathDoubleBits::FULL>
  xiiSimdDouble GetLengthAndNormalize(); // [tested]

  template <xiiInt32 N, xiiMathDoubleBits::Enum acc = xiiMathDoubleBits::FULL>
  xiiSimdVec4d GetNormalized() const; // [tested]

  template <xiiInt32 N, xiiMathDoubleBits::Enum acc = xiiMathDoubleBits::FULL>
  void Normalize(); // [tested]

  template <xiiInt32 N, xiiMathDoubleBits::Enum acc = xiiMathDoubleBits::FULL>
  void NormalizeIfNotZero(const xiiSimdDouble& fEpsilon = xiiMath::SmallEpsilon<double>()); // [tested]

  template <xiiInt32 N>
  bool IsZero() const; // [tested]

  template <xiiInt32 N>
  bool IsZero(const xiiSimdDouble& fEpsilon) const; // [tested]

  template <xiiInt32 N>
  bool IsNormalized(const xiiSimdDouble& fEpsilon = xiiMath::HugeEpsilon<double>()) const; // [tested]

  template <xiiInt32 N>
  bool IsNaN() const; // [tested]

  template <xiiInt32 N>
  bool IsValid() const; // [tested]

public:
  template <xiiInt32 N>
  xiiSimdDouble GetComponent() const; // [tested]

  xiiSimdDouble GetComponent(xiiInt32 i) const; // [tested]

  xiiSimdDouble x() const; // [tested]
  xiiSimdDouble y() const; // [tested]
  xiiSimdDouble z() const; // [tested]
  xiiSimdDouble w() const; // [tested]

  template <xiiSwizzle::Enum s>
  xiiSimdVec4d Get() const; // [tested]

  ///\brief x = this[s0], y = this[s1], z = other[s2], w = other[s3]
  template <xiiSwizzle::Enum s>
  xiiSimdVec4d GetCombined(const xiiSimdVec4d& other) const; // [tested]

public:
  xiiSimdVec4d operator-() const;                      // [tested]
  xiiSimdVec4d operator+(const xiiSimdVec4d& v) const; // [tested]
  xiiSimdVec4d operator-(const xiiSimdVec4d& v) const; // [tested]

  xiiSimdVec4d operator*(const xiiSimdDouble& f) const; // [tested]
  xiiSimdVec4d operator/(const xiiSimdDouble& f) const; // [tested]

  xiiSimdVec4d CompMul(const xiiSimdVec4d& v) const; // [tested]

  template <xiiMathDoubleBits::Enum acc = xiiMathDoubleBits::FULL>
  xiiSimdVec4d CompDiv(const xiiSimdVec4d& v) const; // [tested]

  xiiSimdVec4d CompMin(const xiiSimdVec4d& rhs) const; // [tested]
  xiiSimdVec4d CompMax(const xiiSimdVec4d& rhs) const; // [tested]

  xiiSimdVec4d Abs() const;      // [tested]
  xiiSimdVec4d Round() const;    // [tested]
  xiiSimdVec4d Floor() const;    // [tested]
  xiiSimdVec4d Ceil() const;     // [tested]
  xiiSimdVec4d Trunc() const;    // [tested]
  xiiSimdVec4d Fraction() const; // [tested]

  xiiSimdVec4d FlipSign(const xiiSimdVec4b& cmp) const; // [tested]

  static xiiSimdVec4d Select(const xiiSimdVec4b& cmp, const xiiSimdVec4d& ifTrue, const xiiSimdVec4d& ifFalse); // [tested]

  static xiiSimdVec4d Lerp(const xiiSimdVec4d& a, const xiiSimdVec4d& b, const xiiSimdVec4d& t);

  xiiSimdVec4d& operator+=(const xiiSimdVec4d& v); // [tested]
  xiiSimdVec4d& operator-=(const xiiSimdVec4d& v); // [tested]

  xiiSimdVec4d& operator*=(const xiiSimdDouble& f); // [tested]
  xiiSimdVec4d& operator/=(const xiiSimdDouble& f); // [tested]

  xiiSimdVec4b IsEqual(const xiiSimdVec4d& rhs, const xiiSimdDouble& fEpsilon) const; // [tested]

  xiiSimdVec4b operator==(const xiiSimdVec4d& v) const; // [tested]
  xiiSimdVec4b operator!=(const xiiSimdVec4d& v) const; // [tested]
  xiiSimdVec4b operator<=(const xiiSimdVec4d& v) const; // [tested]
  xiiSimdVec4b operator<(const xiiSimdVec4d& v) const;  // [tested]
  xiiSimdVec4b operator>=(const xiiSimdVec4d& v) const; // [tested]
  xiiSimdVec4b operator>(const xiiSimdVec4d& v) const;  // [tested]

  template <xiiInt32 N>
  xiiSimdDouble HorizontalSum() const; // [tested]

  template <xiiInt32 N>
  xiiSimdDouble HorizontalMin() const; // [tested]

  template <xiiInt32 N>
  xiiSimdDouble HorizontalMax() const; // [tested]

  template <xiiInt32 N>
  xiiSimdDouble Dot(const xiiSimdVec4d& v) const; // [tested]

  ///\brief 3D cross product, w is ignored.
  xiiSimdVec4d CrossRH(const xiiSimdVec4d& v) const; // [tested]

  ///\brief Generates an arbitrary vector such that Dot<3>(GetOrthogonalVector()) == 0
  xiiSimdVec4d GetOrthogonalVector() const; // [tested]

  static xiiSimdVec4d ZeroVector(); // [tested]

  static xiiSimdVec4d MulAdd(const xiiSimdVec4d& a, const xiiSimdVec4d& b, const xiiSimdVec4d& c);  // [tested]
  static xiiSimdVec4d MulAdd(const xiiSimdVec4d& a, const xiiSimdDouble& b, const xiiSimdVec4d& c); // [tested]

  static xiiSimdVec4d MulSub(const xiiSimdVec4d& a, const xiiSimdVec4d& b, const xiiSimdVec4d& c);  // [tested]
  static xiiSimdVec4d MulSub(const xiiSimdVec4d& a, const xiiSimdDouble& b, const xiiSimdVec4d& c); // [tested]

  static xiiSimdVec4d CopySign(const xiiSimdVec4d& magnitude, const xiiSimdVec4d& sign); // [tested]

public:
  xiiInternal::QuadDouble m_v;
};

#include <Foundation/SimdMath/Implementation/SimdVec4d_inl.h>

#if XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEVec4d_inl.h>
#elif XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUVec4d_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
