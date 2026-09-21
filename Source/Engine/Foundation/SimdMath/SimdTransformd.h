/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/SimdMath/SimdQuatd.h>

class XII_FOUNDATION_DLL xiiSimdTransformd
{
public:
  XII_DECLARE_POD_TYPE();

  /// Default constructor: Does not do any initialization.
  xiiSimdTransformd(); // [tested]

  /// Sets position, rotation and scale.
  explicit xiiSimdTransformd(const xiiSimdVec4d& vPosition, const xiiSimdQuatd& qRotation = xiiSimdQuatd::MakeIdentity(), const xiiSimdVec4d& vScale = xiiSimdVec4d(1.0)); // [tested]

  /// Sets rotation.
  explicit xiiSimdTransformd(const xiiSimdQuatd& qRotation); // [tested]

  /// Creates a transform from the given position, rotation and scale.
  [[nodiscard]] static xiiSimdTransformd Make(const xiiSimdVec4d& vPosition, const xiiSimdQuatd& qRotation = xiiSimdQuatd::MakeIdentity(), const xiiSimdVec4d& vScale = xiiSimdVec4d(1.0)); // [tested]

  /// Creates an identity transform.
  [[nodiscard]] static xiiSimdTransformd MakeIdentity(); // [tested]

  /// Creates a transform that is the local transformation needed to get from the parent's transform to the child's.
  [[nodiscard]] static xiiSimdTransformd MakeLocalTransform(const xiiSimdTransformd& globalTransformParent, const xiiSimdTransformd& globalTransformChild); // [tested]

  /// Creates a transform that is the global transform, that is reached by applying the child's local transform to the parent's global one.
  [[nodiscard]] static xiiSimdTransformd MakeGlobalTransform(const xiiSimdTransformd& globalTransformParent, const xiiSimdTransformd& localTransformChild); // [tested]

  /// Returns the scale component with maximum magnitude.
  xiiSimdDouble GetMaxScale() const; // [tested]

  /// Returns whether this transform contains negative scaling aka mirroring.
  bool ContainsNegativeScale() const;

  /// Returns whether this transform contains uniform scaling.
  bool ContainsUniformScale() const;

public:
  /// Equality Check with epsilon
  bool IsEqual(const xiiSimdTransformd& rhs, const xiiSimdDouble& fEpsilon) const; // [tested]

public:
  /// Inverts this transform.
  void Invert(); // [tested]

  /// Returns the inverse of this transform.
  xiiSimdTransformd GetInverse() const; // [tested]

  /// Returns the transformation as a matrix.
  xiiSimdMat4d GetAsMat4() const; // [tested]

public:
  [[nodiscard]] xiiSimdVec4d TransformPosition(const xiiSimdVec4d& v) const;  // [tested]
  [[nodiscard]] xiiSimdVec4d TransformDirection(const xiiSimdVec4d& v) const; // [tested]

  /// Concatenates the two transforms. This is the same as a matrix multiplication, thus not commutative.
  void operator*=(const xiiSimdTransformd& other); // [tested]

  /// Multiplies \a q into the rotation component, thus rotating the entire transformation.
  void operator*=(const xiiSimdQuatd& q); // [tested]

  void operator+=(const xiiSimdVec4d& v); // [tested]
  void operator-=(const xiiSimdVec4d& v); // [tested]

public:
  xiiSimdVec4d m_Position;
  xiiSimdQuatd m_Rotation;
  xiiSimdVec4d m_Scale;
};

// *** free functions ***

/// Transforms the vector v by the transform.
XII_ALWAYS_INLINE const xiiSimdVec4d operator*(const xiiSimdTransformd& t, const xiiSimdVec4d& v); // [tested]

/// Rotates the transform by the given quaternion. Multiplies q from the left with t.
XII_ALWAYS_INLINE const xiiSimdTransformd operator*(const xiiSimdQuatd& q, const xiiSimdTransformd& t); // [tested]

/// Rotates the transform by the given quaternion. Multiplies q from the right with t.
XII_ALWAYS_INLINE const xiiSimdTransformd operator*(const xiiSimdTransformd& t, const xiiSimdQuatd& q); // [tested]

/// Translates the xiiSimdTransformd by the vector. This will move the object in global space.
XII_ALWAYS_INLINE const xiiSimdTransformd operator+(const xiiSimdTransformd& t, const xiiSimdVec4d& v); // [tested]

/// Translates the xiiSimdTransformd by the vector. This will move the object in global space.
XII_ALWAYS_INLINE const xiiSimdTransformd operator-(const xiiSimdTransformd& t, const xiiSimdVec4d& v); // [tested]

/// Concatenates the two transforms. This is the same as a matrix multiplication, thus not commutative.
XII_ALWAYS_INLINE const xiiSimdTransformd operator*(const xiiSimdTransformd& lhs, const xiiSimdTransformd& rhs); // [tested]

XII_ALWAYS_INLINE bool operator==(const xiiSimdTransformd& t1, const xiiSimdTransformd& t2); // [tested]
XII_ALWAYS_INLINE bool operator!=(const xiiSimdTransformd& t1, const xiiSimdTransformd& t2); // [tested]


#include <Foundation/SimdMath/Implementation/SimdTransformd_inl.h>
