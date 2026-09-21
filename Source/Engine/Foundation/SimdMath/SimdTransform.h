/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/SimdMath/SimdQuat.h>

class XII_FOUNDATION_DLL xiiSimdTransform
{
public:
  XII_DECLARE_POD_TYPE();

  /// Default constructor: Does not do any initialization.
  xiiSimdTransform(); // [tested]

  /// Sets position, rotation and scale.
  explicit xiiSimdTransform(const xiiSimdVec4f& vPosition, const xiiSimdQuat& qRotation = xiiSimdQuat::MakeIdentity(), const xiiSimdVec4f& vScale = xiiSimdVec4f(1.0f)); // [tested]

  /// Sets rotation.
  explicit xiiSimdTransform(const xiiSimdQuat& qRotation); // [tested]

  /// Creates a transform from the given position, rotation and scale.
  [[nodiscard]] static xiiSimdTransform Make(const xiiSimdVec4f& vPosition, const xiiSimdQuat& qRotation = xiiSimdQuat::MakeIdentity(), const xiiSimdVec4f& vScale = xiiSimdVec4f(1.0f)); // [tested]

  /// Creates an identity transform.
  [[nodiscard]] static xiiSimdTransform MakeIdentity(); // [tested]

  /// Creates a transform that is the local transformation needed to get from the parent's transform to the child's.
  [[nodiscard]] static xiiSimdTransform MakeLocalTransform(const xiiSimdTransform& globalTransformParent, const xiiSimdTransform& globalTransformChild); // [tested]

  /// Creates a transform that is the global transform, that is reached by applying the child's local transform to the parent's global one.
  [[nodiscard]] static xiiSimdTransform MakeGlobalTransform(const xiiSimdTransform& globalTransformParent, const xiiSimdTransform& localTransformChild); // [tested]

  /// Returns the scale component with maximum magnitude.
  xiiSimdFloat GetMaxScale() const; // [tested]

  /// Returns whether this transform contains negative scaling aka mirroring.
  bool ContainsNegativeScale() const;

  /// Returns whether this transform contains uniform scaling.
  bool ContainsUniformScale() const;

public:
  /// Equality Check with epsilon
  bool IsEqual(const xiiSimdTransform& rhs, const xiiSimdFloat& fEpsilon) const; // [tested]

public:
  /// Inverts this transform.
  void Invert(); // [tested]

  /// Returns the inverse of this transform.
  xiiSimdTransform GetInverse() const; // [tested]

  /// Returns the transformation as a matrix.
  xiiSimdMat4f GetAsMat4() const; // [tested]

public:
  [[nodiscard]] xiiSimdVec4f TransformPosition(const xiiSimdVec4f& v) const;  // [tested]
  [[nodiscard]] xiiSimdVec4f TransformDirection(const xiiSimdVec4f& v) const; // [tested]

  /// Concatenates the two transforms. This is the same as a matrix multiplication, thus not commutative.
  void operator*=(const xiiSimdTransform& other); // [tested]

  /// Multiplies \a q into the rotation component, thus rotating the entire transformation.
  void operator*=(const xiiSimdQuat& q); // [tested]

  void operator+=(const xiiSimdVec4f& v); // [tested]
  void operator-=(const xiiSimdVec4f& v); // [tested]

public:
  xiiSimdVec4f m_Position;
  xiiSimdQuat  m_Rotation;
  xiiSimdVec4f m_Scale;
};

// *** free functions ***

/// Transforms the vector v by the transform.
XII_ALWAYS_INLINE const xiiSimdVec4f operator*(const xiiSimdTransform& t, const xiiSimdVec4f& v); // [tested]

/// Rotates the transform by the given quaternion. Multiplies q from the left with t.
XII_ALWAYS_INLINE const xiiSimdTransform operator*(const xiiSimdQuat& q, const xiiSimdTransform& t); // [tested]

/// Rotates the transform by the given quaternion. Multiplies q from the right with t.
XII_ALWAYS_INLINE const xiiSimdTransform operator*(const xiiSimdTransform& t, const xiiSimdQuat& q); // [tested]

/// Translates the xiiSimdTransform by the vector. This will move the object in global space.
XII_ALWAYS_INLINE const xiiSimdTransform operator+(const xiiSimdTransform& t, const xiiSimdVec4f& v); // [tested]

/// Translates the xiiSimdTransform by the vector. This will move the object in global space.
XII_ALWAYS_INLINE const xiiSimdTransform operator-(const xiiSimdTransform& t, const xiiSimdVec4f& v); // [tested]

/// Concatenates the two transforms. This is the same as a matrix multiplication, thus not commutative.
XII_ALWAYS_INLINE const xiiSimdTransform operator*(const xiiSimdTransform& lhs, const xiiSimdTransform& rhs); // [tested]

XII_ALWAYS_INLINE bool operator==(const xiiSimdTransform& t1, const xiiSimdTransform& t2); // [tested]
XII_ALWAYS_INLINE bool operator!=(const xiiSimdTransform& t1, const xiiSimdTransform& t2); // [tested]


#include <Foundation/SimdMath/Implementation/SimdTransform_inl.h>
