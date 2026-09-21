/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Angle.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>

/// A 4x4 component matrix class.
template <typename Type>
class xiiMat4Template
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  XII_DECLARE_POD_TYPE();

  using ComponentType = Type;

  // *** Data ***
public:
  // The elements are stored in column-major order.
  // That means first is column 0 (with elements of row 0, row 1, row 2, row 3),
  // then column 1, then column 2 and finally column 3

  /// The matrix as a 16-element Type array (column-major)
  Type m_fElementsCM[16];

  XII_ALWAYS_INLINE Type& Element(xiiInt32 iColumn, xiiInt32 iRow) { return m_fElementsCM[iColumn * 4 + iRow]; }
  XII_ALWAYS_INLINE Type  Element(xiiInt32 iColumn, xiiInt32 iRow) const { return m_fElementsCM[iColumn * 4 + iRow]; }

  // *** Constructors ***
public:
  /// Default Constructor DOES NOT INITIALIZE the matrix, at all.
  xiiMat4Template(); // [tested]

  /// Copies 16 values from pData into the matrix. Can handle the data in row-major or column-major order.
  ///
  /// \param pData
  ///   The array of Type values from which to set the matrix data.
  /// \param layout
  ///   The layout in which pData stores the matrix. The data will get transposed, if necessary.
  ///   The data should be in column-major format, if you want to prevent unnecessary transposes.
  xiiMat4Template(const Type* const pData, xiiMatrixLayout::Enum layout); // [tested]

  /// Sets each element manually: Naming is "column-n row-m"
  xiiMat4Template(Type c1r1, Type c2r1, Type c3r1, Type c4r1, Type c1r2, Type c2r2, Type c3r2, Type c4r2, Type c1r3, Type c2r3, Type c3r3, Type c4r3, Type c1r4, Type c2r4, Type c3r4, Type c4r4); // [tested]

  /// Creates a transformation matrix from a rotation and a translation.
  xiiMat4Template(const xiiMat3Template<Type>& mRotation, const xiiVec3Template<Type>& vTranslation); // [tested]

#if XII_ENABLED(XII_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    XII_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please "
                                "check that all code-paths properly initialize this object.");
  }
#endif

  /// Returns a zero matrix.
  [[nodiscard]] static xiiMat4Template<Type> MakeZero();

  /// Returns an identity matrix.
  [[nodiscard]] static xiiMat4Template<Type> MakeIdentity();

  /// Creates a matrix from 16 values that are in row-major layout.
  [[nodiscard]] static xiiMat4Template<Type> MakeFromRowMajorArray(const Type* const pData);

  /// Creates a matrix from 16 values that are in column-major layout.
  [[nodiscard]] static xiiMat4Template<Type> MakeFromColumnMajorArray(const Type* const pData);

  /// Creates a matrix from 16 values. Naming is "column-n row-m"
  [[nodiscard]] static xiiMat4Template<Type> MakeFromValues(Type c1r1, Type c2r1, Type c3r1, Type c4r1, Type c1r2, Type c2r2, Type c3r2, Type c4r2, Type c1r3, Type c2r3, Type c3r3, Type c4r3, Type c1r4, Type c2r4, Type c3r4, Type c4r4);

  /// Creates a matrix with all zero values, except the last column, which is set to x, y, z, 1
  [[nodiscard]] static xiiMat4Template<Type> MakeTranslation(const xiiVec3Template<Type>& vTranslation);

  /// Creates a transformation matrix from a rotation and a translation.
  [[nodiscard]] static xiiMat4Template<Type> MakeTransformation(const xiiMat3Template<Type>& mRotation, const xiiVec3Template<Type>& vTranslation);

  /// Creates a matrix with all zero values, except along the diagonal, which is set to x, y, z, 1
  [[nodiscard]] static xiiMat4Template<Type> MakeScaling(const xiiVec3Template<Type>& vScale);

  /// Creates a matrix that is a rotation matrix around the X-axis.
  [[nodiscard]] static xiiMat4Template<Type> MakeRotationX(xiiAngleTemplate<Type> angle);

  /// Creates a matrix that is a rotation matrix around the Y-axis.
  [[nodiscard]] static xiiMat4Template<Type> MakeRotationY(xiiAngleTemplate<Type> angle);

  /// Creates a matrix that is a rotation matrix around the Z-axis.
  [[nodiscard]] static xiiMat4Template<Type> MakeRotationZ(xiiAngleTemplate<Type> angle);

  /// Creates a matrix that is a rotation matrix around the given axis.
  [[nodiscard]] static xiiMat4Template<Type> MakeAxisRotation(const xiiVec3Template<Type>& vAxis, xiiAngleTemplate<Type> angle);

  /// Copies the 16 values of this matrix into the given array. 'layout' defines whether the data should end up in column-major or
  /// row-major format.
  void GetAsArray(Type* out_pData, xiiMatrixLayout::Enum layout) const; // [tested]

  /// Sets a transformation matrix from a rotation and a translation.
  void SetTransformationMatrix(const xiiMat3Template<Type>& mRotation, const xiiVec3Template<Type>& vTranslation); // [tested]

  // *** Special matrix constructors ***
public:
  /// Sets all elements to zero.
  void SetZero(); // [tested]

  /// Sets all elements to zero, except the diagonal, which is set to one.
  void SetIdentity(); // [tested]

  // *** Common Matrix Operations ***
public:
  /// Transposes this matrix.
  void Transpose(); // [tested]

  /// Returns the transpose of this matrix.
  const xiiMat4Template<Type> GetTranspose() const; // [tested]

  /// Inverts this matrix. Return value indicates whether the matrix could be inverted.
  xiiResult Invert(Type fEpsilon = xiiMath::SmallEpsilon<Type>()); // [tested]

  /// Returns the inverse of this matrix.
  const xiiMat4Template<Type> GetInverse(Type fEpsilon = xiiMath::SmallEpsilon<Type>()) const; // [tested]

  // *** Checks ***
public:
  /// Checks whether all elements are zero.
  bool IsZero(Type fEpsilon = xiiMath::DefaultEpsilon<Type>()) const; // [tested]

  /// Checks whether this is an identity matrix.
  bool IsIdentity(Type fEpsilon = xiiMath::DefaultEpsilon<Type>()) const; // [tested]

  /// Checks whether all components are finite numbers.
  bool IsValid() const; // [tested]

  /// Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  // *** Special Accessors ***
public:
  /// Returns all 4 components of the i-th row.
  xiiVec4Template<Type> GetRow(xiiUInt32 uiRow) const; // [tested]

  /// Sets all 4 components of the i-th row.
  void SetRow(xiiUInt32 uiRow, const xiiVec4Template<Type>& vRow); // [tested]

  /// Returns all 4 components of the i-th column.
  xiiVec4Template<Type> GetColumn(xiiUInt32 uiColumn) const; // [tested]

  /// Sets all 4 components of the i-th column.
  void SetColumn(xiiUInt32 uiColumn, const xiiVec4Template<Type>& vColumn); // [tested]

  /// Returns all 4 components on the diagonal of the matrix.
  xiiVec4Template<Type> GetDiagonal() const; // [tested]

  /// Sets all 4 components on the diagonal of the matrix.
  void SetDiagonal(const xiiVec4Template<Type>& vDiag); // [tested]

  /// Returns the first 3 components of the last column.
  const xiiVec3Template<Type> GetTranslationVector() const; // [tested]

  /// Sets the first 3 components of the last column.
  void SetTranslationVector(const xiiVec3Template<Type>& v); // [tested]

  /// Sets the 3x3 rotational part of the matrix.
  void SetRotationalPart(const xiiMat3Template<Type>& mRotation); // [tested]

  /// Returns the 3x3 rotational and scaling part of the matrix.
  const xiiMat3Template<Type> GetRotationalPart() const; // [tested]

  /// Returns the 3 scaling factors that are encoded in the matrix.
  const xiiVec3Template<Type> GetScalingFactors() const; // [tested]

  /// Tries to set the three scaling factors in the matrix. Returns XII_FAILURE if the matrix columns cannot be normalized and thus no
  /// rescaling is possible.
  xiiResult SetScalingFactors(const xiiVec3Template<Type>& vXYZ, Type fEpsilon = xiiMath::DefaultEpsilon<Type>()); // [tested]

  // *** Operators ***
public:
  /// Matrix-vector multiplication, assuming the 4th component of the vector is one (default behavior).
  const xiiVec3Template<Type> TransformPosition(const xiiVec3Template<Type>& v) const; // [tested]

  /// Matrix-vector multiplication, assuming the 4th component of the vector is one (default behavior).
  void TransformPosition(xiiVec3Template<Type>* pV, xiiUInt32 uiNumVectors, xiiUInt32 uiStride = sizeof(xiiVec3Template<Type>)) const; // [tested]

  /// Matrix-vector multiplication, assuming the 4th component of the vector is zero. So, rotation/scaling only. Useful as an
  /// optimization.
  const xiiVec3Template<Type> TransformDirection(const xiiVec3Template<Type>& v) const; // [tested]

  /// Matrix-vector multiplication, assuming the 4th component of the vector is zero. So, rotation/scaling only. Useful as an
  /// optimization.
  void TransformDirection(xiiVec3Template<Type>* pV, xiiUInt32 uiNumVectors, xiiUInt32 uiStride = sizeof(xiiVec3Template<Type>)) const; // [tested]

  /// Matrix-vector multiplication.
  const xiiVec4Template<Type> Transform(const xiiVec4Template<Type>& v) const; // [tested]

  /// Matrix-vector multiplication.
  void Transform(xiiVec4Template<Type>* pV, xiiUInt32 uiNumVectors, xiiUInt32 uiStride = sizeof(xiiVec4Template<Type>)) const; // [tested]

  /// Component-wise multiplication (commutative)
  void operator*=(Type f); // [tested]

  /// Component-wise division
  void operator/=(Type f); // [tested]

  /// Equality Check
  bool IsIdentical(const xiiMat4Template<Type>& rhs) const; // [tested]

  /// Equality Check with epsilon
  bool IsEqual(const xiiMat4Template<Type>& rhs, Type fEpsilon) const; // [tested]
};

// *** free functions ***

/// Matrix-Matrix multiplication
template <typename Type>
const xiiMat4Template<Type> operator*(const xiiMat4Template<Type>& m1, const xiiMat4Template<Type>& m2); // [tested]

/// Matrix-vector multiplication
template <typename Type>
const xiiVec3Template<Type> operator*(const xiiMat4Template<Type>& m, const xiiVec3Template<Type>& v); // [tested]

/// Matrix-vector multiplication
template <typename Type>
const xiiVec4Template<Type> operator*(const xiiMat4Template<Type>& m, const xiiVec4Template<Type>& v); // [tested]

/// Component-wise multiplication (commutative)
template <typename Type>
const xiiMat4Template<Type> operator*(const xiiMat4Template<Type>& m1, Type f); // [tested]

/// Component-wise multiplication (commutative)
template <typename Type>
const xiiMat4Template<Type> operator*(Type f, const xiiMat4Template<Type>& m1); // [tested]

/// Component-wise division
template <typename Type>
const xiiMat4Template<Type> operator/(const xiiMat4Template<Type>& m1, Type f); // [tested]

/// Adding two matrices (component-wise)
template <typename Type>
const xiiMat4Template<Type> operator+(const xiiMat4Template<Type>& m1, const xiiMat4Template<Type>& m2); // [tested]

/// Subtracting two matrices (component-wise)
template <typename Type>
const xiiMat4Template<Type> operator-(const xiiMat4Template<Type>& m1, const xiiMat4Template<Type>& m2); // [tested]

/// Comparison Operator ==
template <typename Type>
bool operator==(const xiiMat4Template<Type>& lhs, const xiiMat4Template<Type>& rhs); // [tested]

#include <Foundation/Math/Implementation/Mat4_inl.h>
