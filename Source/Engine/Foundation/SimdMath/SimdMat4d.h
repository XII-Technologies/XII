#pragma once

#include <Foundation/SimdMath/SimdVec4d.h>

/// \brief A 4x4 matrix class.
class XII_FOUNDATION_DLL xiiSimdMat4d
{
public:
  XII_DECLARE_POD_TYPE();

  xiiSimdMat4d();

  xiiSimdMat4d(const double* const pData, xiiMatrixLayout::Enum layout); // [tested]

  xiiSimdMat4d(const xiiSimdVec4d& vCol0, const xiiSimdVec4d& vCol1, const xiiSimdVec4d& vCol2, const xiiSimdVec4d& vCol3); // [tested]

  /// \brief Sets each element manually: Naming is "column-n row-m"
  xiiSimdMat4d(double f1r1, double f2r1, double f3r1, double f4r1, double f1r2, double f2r2, double f3r2, double f4r2, double f1r3, double f2r3, double f3r3, double f4r3, double f1r4, double f2r4, double f3r4, double f4r4); // [tested]

  void SetFromArray(const double* const pData, xiiMatrixLayout::Enum layout); // [tested]

  void GetAsArray(double* out_pData, xiiMatrixLayout::Enum layout) const; // [tested]

  /// \brief Sets all elements to zero, except the diagonal, which is set to one.
  void SetIdentity(); // [tested]

  /// \brief Sets all elements to zero.
  void SetZero(); // [tested]

  /// \brief Returns an Identity Matrix.
  static xiiSimdMat4d IdentityMatrix(); // [tested]

  /// \brief Returns a Matrix where all elements are zero.
  static xiiSimdMat4d ZeroMatrix(); // [tested]

public:
  /// \brief Transposes this matrix.
  void Transpose(); // [tested]

  /// \brief Returns the transpose of this matrix.
  xiiSimdMat4d GetTranspose() const; // [tested]

  /// \brief Inverts this matrix. Return value indicates whether the matrix could be inverted.
  xiiResult Invert(const xiiSimdDouble& fEpsilon = xiiMath::SmallEpsilon<double>()); // [tested]

  /// \brief Returns the inverse of this matrix.
  xiiSimdMat4d GetInverse(const xiiSimdDouble& fEpsilon = xiiMath::SmallEpsilon<double>()) const; // [tested]

public:
  /// \brief Equality Check with epsilon
  bool IsEqual(const xiiSimdMat4d& rhs, const xiiSimdDouble& fEpsilon) const; // [tested]

  /// \brief Checks whether this is an identity matrix.
  bool IsIdentity(const xiiSimdDouble& fEpsilon = xiiMath::DefaultEpsilon<double>()) const; // [tested]

  /// \brief Checks whether all components are finite numbers.
  bool IsValid() const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

public:
  void SetRows(const xiiSimdVec4d& vRow0, const xiiSimdVec4d& vRow1, const xiiSimdVec4d& vRow2, const xiiSimdVec4d& vRow3); // [tested]
  void GetRows(xiiSimdVec4d& ref_vRow0, xiiSimdVec4d& ref_vRow1, xiiSimdVec4d& ref_vRow2, xiiSimdVec4d& ref_vRow3) const;   // [tested]

public:
  /// \brief Matrix-vector multiplication, assuming the 4th component of the vector is one (default behavior).
  xiiSimdVec4d TransformPosition(const xiiSimdVec4d& v) const; // [tested]

  /// \brief Matrix-vector multiplication, assuming the 4th component of the vector is zero. So, rotation/scaling only.
  xiiSimdVec4d TransformDirection(const xiiSimdVec4d& v) const; // [tested]

  xiiSimdMat4d operator*(const xiiSimdMat4d& rhs) const; // [tested]
  void         operator*=(const xiiSimdMat4d& rhs);

  bool operator==(const xiiSimdMat4d& rhs) const; // [tested]
  bool operator!=(const xiiSimdMat4d& rhs) const; // [tested]

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
