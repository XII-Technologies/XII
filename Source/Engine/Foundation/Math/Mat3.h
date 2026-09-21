/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Angle.h>
#include <Foundation/Math/Vec3.h>

/// A 3x3 component matrix class.
template <typename Type>
class xiiMat3Template
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  XII_DECLARE_POD_TYPE();

  using ComponentType = Type;

  // *** Data ***
public:
  // The elements are stored in column-major order.
  // That means first is column 0 (with elements of row 0, row 1, row 2),
  // then column 1, then column 2

  /// The matrix as a 9-element Type array (column-major)
  Type m_fElementsCM[9];

  XII_ALWAYS_INLINE Type& Element(xiiInt32 iColumn, xiiInt32 iRow) { return m_fElementsCM[iColumn * 3 + iRow]; }
  XII_ALWAYS_INLINE Type  Element(xiiInt32 iColumn, xiiInt32 iRow) const { return m_fElementsCM[iColumn * 3 + iRow]; }

  // *** Constructors ***
public:
  /// Default Constructor DOES NOT INITIALIZE the matrix, at all.
  xiiMat3Template(); // [tested]

  /// Copies 9 values from pData into the matrix. Can handle the data in row-major or column-major order.
  ///
  /// \param pData
  ///   The array of Type values from which to set the matrix data.
  /// \param layout
  ///   The layout in which pData stores the matrix. The data will get transposed, if necessary.
  ///   The data should be in column-major format, if you want to prevent unnecessary transposes.
  xiiMat3Template(const Type* const pData, xiiMatrixLayout::Enum layout); // [tested]

  /// Sets each element manually: Naming is "column-n row-m"
  xiiMat3Template(Type c1r1, Type c2r1, Type c3r1, Type c1r2, Type c2r2, Type c3r2, Type c1r3, Type c2r3, Type c3r3); // [tested]

  /// Returns a zero matrix.
  [[nodiscard]] static xiiMat3Template<Type> MakeZero();

  /// Returns an identity matrix.
  [[nodiscard]] static xiiMat3Template<Type> MakeIdentity();

  /// Creates a matrix from 9 values that are in row-major layout.
  [[nodiscard]] static xiiMat3Template<Type> MakeFromRowMajorArray(const Type* const pData);

  /// Creates a matrix from 9 values that are in column-major layout.
  [[nodiscard]] static xiiMat3Template<Type> MakeFromColumnMajorArray(const Type* const pData);

  /// Creates a matrix from 9 values. Naming is "column-n row-m"
  [[nodiscard]] static xiiMat3Template<Type> MakeFromValues(Type c1r1, Type c2r1, Type c3r1, Type c1r2, Type c2r2, Type c3r2, Type c1r3, Type c2r3, Type c3r3);

  /// Creates a matrix with all zero values, except along the diagonal, which is set to x,y,z
  [[nodiscard]] static xiiMat3Template<Type> MakeScaling(const xiiVec3Template<Type>& vScale);

  /// Creates a matrix that is a rotation matrix around the X-axis.
  [[nodiscard]] static xiiMat3Template<Type> MakeRotationX(xiiAngleTemplate<Type> angle);

  /// Creates a matrix that is a rotation matrix around the Y-axis.
  [[nodiscard]] static xiiMat3Template<Type> MakeRotationY(xiiAngleTemplate<Type> angle);

  /// Creates a matrix that is a rotation matrix around the Z-axis.
  [[nodiscard]] static xiiMat3Template<Type> MakeRotationZ(xiiAngleTemplate<Type> angle);

  /// Creates a matrix that is a rotation matrix around the given axis.
  [[nodiscard]] static xiiMat3Template<Type> MakeAxisRotation(const xiiVec3Template<Type>& vAxis, xiiAngleTemplate<Type> angle);

#if XII_ENABLED(XII_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    XII_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please check that "
                                "all code-paths properly initialize this object.");
  }
#endif

  /// Copies the 9 values of this matrix into the given array. 'layout' defines whether the data should end up in column-major or row-major
  /// format.
  void GetAsArray(Type* out_pData, xiiMatrixLayout::Enum layout) const; // [tested]

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
  const xiiMat3Template<Type> GetTranspose() const; // [tested]

  /// Inverts this matrix. Return value indicates whether the matrix could be Inverted.
  xiiResult Invert(Type fEpsilon = xiiMath::SmallEpsilon<Type>()); // [tested]

  /// Returns the inverse of this matrix.
  const xiiMat3Template<Type> GetInverse(Type fEpsilon = xiiMath::SmallEpsilon<Type>()) const; // [tested]

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
  /// Returns all 3 components of the i-th row.
  xiiVec3Template<Type> GetRow(xiiUInt32 uiRow) const; // [tested]

  /// Sets all 3 components of the i-th row.
  void SetRow(xiiUInt32 uiRow, const xiiVec3Template<Type>& vRow); // [tested]

  /// Returns all 3 components of the i-th column.
  xiiVec3Template<Type> GetColumn(xiiUInt32 uiColumn) const; // [tested]

  /// Sets all 3 components of the i-th column.
  void SetColumn(xiiUInt32 uiColumn, const xiiVec3Template<Type>& vColumn); // [tested]

  /// Returns all 3 components on the diagonal of the matrix.
  xiiVec3Template<Type> GetDiagonal() const; // [tested]

  /// Sets all 3 components on the diagonal of the matrix.
  void SetDiagonal(const xiiVec3Template<Type>& vDiag); // [tested]

  /// Returns the 3 scaling factors that are encoded in the matrix.
  const xiiVec3Template<Type> GetScalingFactors() const; // [tested]

  /// Tries to set the three scaling factors in the matrix. Returns XII_FAILURE if the matrix columns cannot be normalized and thus no rescaling
  /// is possible.
  xiiResult SetScalingFactors(const xiiVec3Template<Type>& vXYZ, Type fEpsilon = xiiMath::DefaultEpsilon<Type>()); // [tested]

  /// Computes the determinant of the matrix.
  Type GetDeterminant() const;

  // *** Operators ***
public:
  /// Matrix-vector multiplication, assuming the 4th component of the vector is zero. So, rotation/scaling only. Useful as an optimization.
  const xiiVec3Template<Type> TransformDirection(const xiiVec3Template<Type>& v) const; // [tested]

  /// Component-wise multiplication (commutative)
  void operator*=(Type f);

  /// Component-wise division.
  void operator/=(Type f); // [tested]

  /// Equality Check.
  bool IsIdentical(const xiiMat3Template<Type>& rhs) const; // [tested]

  /// Equality Check with epsilon.
  bool IsEqual(const xiiMat3Template<Type>& rhs, Type fEpsilon) const; // [tested]
};

// *** free functions ***

/// Matrix-Matrix multiplication
template <typename Type>
const xiiMat3Template<Type> operator*(const xiiMat3Template<Type>& m1, const xiiMat3Template<Type>& m2); // [tested]

/// Matrix-vector multiplication
template <typename Type>
const xiiVec3Template<Type> operator*(const xiiMat3Template<Type>& m, const xiiVec3Template<Type>& v); // [tested]

/// Component-wise multiplication (commutative)
template <typename Type>
const xiiMat3Template<Type> operator*(const xiiMat3Template<Type>& m1, Type f); // [tested]

/// Component-wise multiplication (commutative)
template <typename Type>
const xiiMat3Template<Type> operator*(Type f, const xiiMat3Template<Type>& m1); // [tested]

/// Component-wise division
template <typename Type>
const xiiMat3Template<Type> operator/(const xiiMat3Template<Type>& m1, Type f); // [tested]

/// Adding two matrices (component-wise)
template <typename Type>
const xiiMat3Template<Type> operator+(const xiiMat3Template<Type>& m1, const xiiMat3Template<Type>& m2); // [tested]

/// Subtracting two matrices (component-wise)
template <typename Type>
const xiiMat3Template<Type> operator-(const xiiMat3Template<Type>& m1, const xiiMat3Template<Type>& m2); // [tested]

/// Comparison Operator ==
template <typename Type>
bool operator==(const xiiMat3Template<Type>& lhs, const xiiMat3Template<Type>& rhs); // [tested]

#include <Foundation/Math/Implementation/Mat3_inl.h>
