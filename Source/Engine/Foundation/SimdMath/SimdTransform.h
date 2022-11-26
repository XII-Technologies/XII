#pragma once

#include <Foundation/SimdMath/SimdQuat.h>

class XII_FOUNDATION_DLL xiiSimdTransform
{
public:
  XII_DECLARE_POD_TYPE();

  /// \brief Default constructor: Does not do any initialization.
  xiiSimdTransform(); // [tested]

  /// \brief Sets position, rotation and scale.
  explicit xiiSimdTransform(const xiiSimdVec4f& position, const xiiSimdQuat& rotation = xiiSimdQuat::IdentityQuaternion(),
                            const xiiSimdVec4f& scale = xiiSimdVec4f(1.0f)); // [tested]

  /// \brief Sets rotation.
  explicit xiiSimdTransform(const xiiSimdQuat& rotation); // [tested]

  /// \brief Sets the position to be zero and the rotation to identity.
  void SetIdentity(); // [tested]

  /// \brief Returns an Identity Transform.
  static xiiSimdTransform IdentityTransform(); // [tested]

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

public:
  /// \brief Sets this transform to be the local transformation needed to get from the parent's transform to the child's.
  void SetLocalTransform(const xiiSimdTransform& GlobalTransformParent, const xiiSimdTransform& GlobalTransformChild); // [tested]

  /// \brief Sets this transform to the global transform, that is reached by applying the child's local transform to the parent's global
  /// one.
  void SetGlobalTransform(const xiiSimdTransform& GlobalTransformParent, const xiiSimdTransform& LocalTransformChild); // [tested]

  /// \brief Returns the transformation as a matrix.
  xiiSimdMat4f GetAsMat4() const; // [tested]

public:
  xiiSimdVec4f TransformPosition(const xiiSimdVec4f& v) const;  // [tested]
  xiiSimdVec4f TransformDirection(const xiiSimdVec4f& v) const; // [tested]

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
