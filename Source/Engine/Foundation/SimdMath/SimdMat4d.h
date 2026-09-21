/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/SimdMath/SimdVec4d.h>

/// A 4x4 matrix class.
class XII_FOUNDATION_DLL xiiSimdMat4d
{
public:
  XII_DECLARE_POD_TYPE();

  xiiSimdMat4d();

  /// Returns a zero matrix.
  [[nodiscard]] static xiiSimdMat4d MakeZero();

  /// Returns an identity matrix.
  [[nodiscard]] static xiiSimdMat4d MakeIdentity();

  /// Creates a matrix from 16 values that are in row-major layout.
  [[nodiscard]] static xiiSimdMat4d MakeFromRowMajorArray(const double* const pData);

  /// Creates a matrix from 16 values that are in column-major layout.
  [[nodiscard]] static xiiSimdMat4d MakeFromColumnMajorArray(const double* const pData);

  /// Creates a matrix from 4 column vectors.
  [[nodiscard]] static xiiSimdMat4d MakeFromColumns(const xiiSimdVec4d& vCol0, const xiiSimdVec4d& vCol1, const xiiSimdVec4d& vCol2, const xiiSimdVec4d& vCol3);

  /// Creates a matrix from 16 values. Naming is "column-n row-m"
  [[nodiscard]] static xiiSimdMat4d MakeFromValues(double f1r1, double f2r1, double f3r1, double f4r1, double f1r2, double f2r2, double f3r2, double f4r2, double f1r3, double f2r3, double f3r3, double f4r3, double f1r4, double f2r4, double f3r4, double f4r4);

  void GetAsArray(double* out_pData, xiiMatrixLayout::Enum layout) const; // [tested]

public:
  /// Transposes this matrix.
  void Transpose(); // [tested]

  /// Returns the transpose of this matrix.
  xiiSimdMat4d GetTranspose() const; // [tested]

  /// Inverts this matrix. Return value indicates whether the matrix could be inverted.
  xiiResult Invert(const xiiSimdDouble& fEpsilon = xiiMath::SmallEpsilon<double>()); // [tested]

  /// Returns the inverse of this matrix.
  xiiSimdMat4d GetInverse(const xiiSimdDouble& fEpsilon = xiiMath::SmallEpsilon<double>()) const; // [tested]

public:
  /// Equality Check with epsilon
  bool IsEqual(const xiiSimdMat4d& rhs, const xiiSimdDouble& fEpsilon) const; // [tested]

  /// Checks whether this is an identity matrix.
  bool IsIdentity(const xiiSimdDouble& fEpsilon = xiiMath::DefaultEpsilon<double>()) const; // [tested]

  /// Checks whether all components are finite numbers.
  bool IsValid() const; // [tested]

  /// Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

public:
  void SetRows(const xiiSimdVec4d& vRow0, const xiiSimdVec4d& vRow1, const xiiSimdVec4d& vRow2, const xiiSimdVec4d& vRow3); // [tested]
  void GetRows(xiiSimdVec4d& ref_vRow0, xiiSimdVec4d& ref_vRow1, xiiSimdVec4d& ref_vRow2, xiiSimdVec4d& ref_vRow3) const;   // [tested]

public:
  /// Matrix-vector multiplication, assuming the 4th component of the vector is one (default behavior).
  [[nodiscard]] xiiSimdVec4d TransformPosition(const xiiSimdVec4d& v) const; // [tested]

  /// Matrix-vector multiplication, assuming the 4th component of the vector is zero. So, rotation/scaling only.
  [[nodiscard]] xiiSimdVec4d TransformDirection(const xiiSimdVec4d& v) const; // [tested]

  [[nodiscard]] xiiSimdMat4d operator*(const xiiSimdMat4d& rhs) const; // [tested]
  void                       operator*=(const xiiSimdMat4d& rhs);

  [[nodiscard]] bool operator==(const xiiSimdMat4d& rhs) const; // [tested]
  [[nodiscard]] bool operator!=(const xiiSimdMat4d& rhs) const; // [tested]

public:
  xiiSimdVec4d m_col0;
  xiiSimdVec4d m_col1;
  xiiSimdVec4d m_col2;
  xiiSimdVec4d m_col3;
};

#include <Foundation/SimdMath/Implementation/SimdMat4d_inl.h>

#if XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX
#  include <Foundation/SimdMath/Implementation/AVX/AVXMat4d_inl.h>
#elif XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE || XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_NEON || XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUMat4d_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
