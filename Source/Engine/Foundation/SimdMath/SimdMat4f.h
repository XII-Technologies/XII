#pragma once

#include <Foundation/SimdMath/SimdVec4f.h>

/// \brief A 4x4 matrix class
class XII_FOUNDATION_DLL xiiSimdMat4f
{
public:
  XII_DECLARE_POD_TYPE();

  xiiSimdMat4f();

  /// \brief Returns a zero matrix.
  [[nodiscard]] static xiiSimdMat4f MakeZero();

  /// \brief Returns an identity matrix.
  [[nodiscard]] static xiiSimdMat4f MakeIdentity();

  /// \brief Creates a matrix from 16 values that are in row-major layout.
  [[nodiscard]] static xiiSimdMat4f MakeFromRowMajorArray(const float* const pData);

  /// \brief Creates a matrix from 16 values that are in column-major layout.
  [[nodiscard]] static xiiSimdMat4f MakeFromColumnMajorArray(const float* const pData);

  /// \brief Creates a matrix from 4 column vectors.
  [[nodiscard]] static xiiSimdMat4f MakeFromColumns(const xiiSimdVec4f& vCol0, const xiiSimdVec4f& vCol1, const xiiSimdVec4f& vCol2, const xiiSimdVec4f& vCol3);

  /// \brief Creates a matrix from 16 values. Naming is "column-n row-m"
  [[nodiscard]] static xiiSimdMat4f MakeFromValues(float f1r1, float f2r1, float f3r1, float f4r1, float f1r2, float f2r2, float f3r2, float f4r2, float f1r3, float f2r3, float f3r3, float f4r3, float f1r4, float f2r4, float f3r4, float f4r4);

  void GetAsArray(float* out_pData, xiiMatrixLayout::Enum layout) const; // [tested]

public:
  /// \brief Transposes this matrix.
  void Transpose(); // [tested]

  /// \brief Returns the transpose of this matrix.
  xiiSimdMat4f GetTranspose() const; // [tested]

  /// \brief Inverts this matrix. Return value indicates whether the matrix could be inverted.
  xiiResult Invert(const xiiSimdFloat& fEpsilon = xiiMath::SmallEpsilon<float>()); // [tested]

  /// \brief Returns the inverse of this matrix.
  xiiSimdMat4f GetInverse(const xiiSimdFloat& fEpsilon = xiiMath::SmallEpsilon<float>()) const; // [tested]

public:
  /// \brief Equality Check with epsilon
  bool IsEqual(const xiiSimdMat4f& rhs, const xiiSimdFloat& fEpsilon) const; // [tested]

  /// \brief Checks whether this is an identity matrix.
  bool IsIdentity(const xiiSimdFloat& fEpsilon = xiiMath::DefaultEpsilon<float>()) const; // [tested]

  /// \brief Checks whether all components are finite numbers.
  bool IsValid() const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

public:
  void SetRows(const xiiSimdVec4f& vRow0, const xiiSimdVec4f& vRow1, const xiiSimdVec4f& vRow2, const xiiSimdVec4f& vRow3); // [tested]
  void GetRows(xiiSimdVec4f& ref_vRow0, xiiSimdVec4f& ref_vRow1, xiiSimdVec4f& ref_vRow2, xiiSimdVec4f& ref_vRow3) const;   // [tested]

public:
  /// \brief Matrix-vector multiplication, assuming the 4th component of the vector is one (default behavior).
  [[nodiscard]] xiiSimdVec4f TransformPosition(const xiiSimdVec4f& v) const; // [tested]

  /// \brief Matrix-vector multiplication, assuming the 4th component of the vector is zero. So, rotation/scaling only.
  [[nodiscard]] xiiSimdVec4f TransformDirection(const xiiSimdVec4f& v) const; // [tested]

  [[nodiscard]] xiiSimdMat4f operator*(const xiiSimdMat4f& rhs) const; // [tested]
  void                       operator*=(const xiiSimdMat4f& rhs);

  [[nodiscard]] bool operator==(const xiiSimdMat4f& rhs) const; // [tested]
  [[nodiscard]] bool operator!=(const xiiSimdMat4f& rhs) const; // [tested]

public:
  xiiSimdVec4f m_col0;
  xiiSimdVec4f m_col1;
  xiiSimdVec4f m_col2;
  xiiSimdVec4f m_col3;
};

#include <Foundation/SimdMath/Implementation/SimdMat4f_inl.h>

#if XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX || XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEMat4f_inl.h>
#elif XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONMat4f_inl.h>
#elif XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUMat4f_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
