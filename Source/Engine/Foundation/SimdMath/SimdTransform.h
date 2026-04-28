/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/SimdMath/SimdQuat.h>

class XII_FOUNDATION_DLL xiiSimdTransform
{
public:
  XII_DECLARE_POD_TYPE();

  /// \brief Default constructor: Does not do any initialization.
  xiiSimdTransform(); // [tested]

  /// \brief Sets position, rotation and scale.
  explicit xiiSimdTransform(const xiiSimdVec4f& vPosition, const xiiSimdQuat& qRotation = xiiSimdQuat::MakeIdentity(), const xiiSimdVec4f& vScale = xiiSimdVec4f(1.0f)); // [tested]

  /// \brief Sets rotation.
  explicit xiiSimdTransform(const xiiSimdQuat& qRotation); // [tested]

  /// \brief Creates a transform from the given position, rotation and scale.
  [[nodiscard]] static xiiSimdTransform Make(const xiiSimdVec4f& vPosition, const xiiSimdQuat& qRotation = xiiSimdQuat::MakeIdentity(), const xiiSimdVec4f& vScale = xiiSimdVec4f(1.0f)); // [tested]

  /// \brief Creates an identity transform.
  [[nodiscard]] static xiiSimdTransform MakeIdentity(); // [tested]

  /// \brief Creates a transform that is the local transformation needed to get from the parent's transform to the child's.
  [[nodiscard]] static xiiSimdTransform MakeLocalTransform(const xiiSimdTransform& globalTransformParent, const xiiSimdTransform& globalTransformChild); // [tested]

  /// \brief Creates a transform that is the global transform, that is reached by applying the child's local transform to the parent's global one.
  [[nodiscard]] static xiiSimdTransform MakeGlobalTransform(const xiiSimdTransform& globalTransformParent, const xiiSimdTransform& localTransformChild); // [tested]

  /// \brief Returns the scale component with maximum magnitude.
  xiiSimdFloat GetMaxScale() const; // [tested]

  /// \brief Returns whether this transform contains negative scaling aka mirroring.
  bool ContainsNegativeScale() const;

  /// \brief Returns whether this transform contains uniform scaling.
  bool ContainsUniformScale() const;

public:
  /// \brief Equality Check with epsilon
  bool IsEqual(const xiiSimdTransform& rhs, const xiiSimdFloat& fEpsilon) const; // [tested]

public:
  /// \brief Inverts this transform.
  void Invert(); // [tested]

  /// \brief Returns the inverse of this transform.
  xiiSimdTransform GetInverse() const; // [tested]

  /// \brief Returns the transformation as a matrix.
  xiiSimdMat4f GetAsMat4() const; // [tested]

public:
  [[nodiscard]] xiiSimdVec4f TransformPosition(const xiiSimdVec4f& v) const;  // [tested]
  [[nodiscard]] xiiSimdVec4f TransformDirection(const xiiSimdVec4f& v) const; // [tested]

  /// \brief Concatenates the two transforms. This is the same as a matrix multiplication, thus not commutative.
  void operator*=(const xiiSimdTransform& other); // [tested]

  /// \brief Multiplies \a q into the rotation component, thus rotating the entire transformation.
  void operator*=(const xiiSimdQuat& q); // [tested]

  void operator+=(const xiiSimdVec4f& v); // [tested]
  void operator-=(const xiiSimdVec4f& v); // [tested]

public:
  xiiSimdVec4f m_Position;
  xiiSimdQuat  m_Rotation;
  xiiSimdVec4f m_Scale;
};

// *** free functions ***

/// \brief Transforms the vector v by the transform.
XII_ALWAYS_INLINE const xiiSimdVec4f operator*(const xiiSimdTransform& t, const xiiSimdVec4f& v); // [tested]

/// \brief Rotates the transform by the given quaternion. Multiplies q from the left with t.
XII_ALWAYS_INLINE const xiiSimdTransform operator*(const xiiSimdQuat& q, const xiiSimdTransform& t); // [tested]

/// \brief Rotates the transform by the given quaternion. Multiplies q from the right with t.
XII_ALWAYS_INLINE const xiiSimdTransform operator*(const xiiSimdTransform& t, const xiiSimdQuat& q); // [tested]

/// \brief Translates the xiiSimdTransform by the vector. This will move the object in global space.
XII_ALWAYS_INLINE const xiiSimdTransform operator+(const xiiSimdTransform& t, const xiiSimdVec4f& v); // [tested]

/// \brief Translates the xiiSimdTransform by the vector. This will move the object in global space.
XII_ALWAYS_INLINE const xiiSimdTransform operator-(const xiiSimdTransform& t, const xiiSimdVec4f& v); // [tested]

/// \brief Concatenates the two transforms. This is the same as a matrix multiplication, thus not commutative.
XII_ALWAYS_INLINE const xiiSimdTransform operator*(const xiiSimdTransform& lhs, const xiiSimdTransform& rhs); // [tested]

XII_ALWAYS_INLINE bool operator==(const xiiSimdTransform& t1, const xiiSimdTransform& t2); // [tested]
XII_ALWAYS_INLINE bool operator!=(const xiiSimdTransform& t1, const xiiSimdTransform& t2); // [tested]


#include <Foundation/SimdMath/Implementation/SimdTransform_inl.h>
