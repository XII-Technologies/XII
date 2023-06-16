#pragma once

#include <Foundation/SimdMath/SimdQuatd.h>

class XII_FOUNDATION_DLL xiiSimdTransformd
{
public:
  XII_DECLARE_POD_TYPE();

  /// \brief Default constructor: Does not do any initialization.
  xiiSimdTransformd(); // [tested]

  /// \brief Sets position, rotation and scale.
  explicit xiiSimdTransformd(const xiiSimdVec4d& vPosition, const xiiSimdQuatd& qRotation = xiiSimdQuatd::IdentityQuaternion(), const xiiSimdVec4d& vScale = xiiSimdVec4d(1.0f)); // [tested]

  /// \brief Sets rotation.
  explicit xiiSimdTransformd(const xiiSimdQuatd& qRotation); // [tested]

  /// \brief Sets the position to be zero and the rotation to identity.
  void SetIdentity(); // [tested]

  /// \brief Returns an Identity Transform.
  static xiiSimdTransformd IdentityTransform(); // [tested]

  /// \brief Returns the scale component with maximum magnitude.
  xiiSimdDouble GetMaxScale() const; // [tested]

  /// \brief Returns whether this transform contains negative scaling aka mirroring.
  bool ContainsNegativeScale() const;

  /// \brief Returns whether this transform contains uniform scaling.
  bool ContainsUniformScale() const;

public:
  /// \brief Equality Check with epsilon
  bool IsEqual(const xiiSimdTransformd& rhs, const xiiSimdDouble& fEpsilon) const; // [tested]

public:
  /// \brief Inverts this transform.
  void Invert(); // [tested]

  /// \brief Returns the inverse of this transform.
  xiiSimdTransformd GetInverse() const; // [tested]

public:
  /// \brief Sets this transform to be the local transformation needed to get from the parent's transform to the child's.
  void SetLocalTransform(const xiiSimdTransformd& globalTransformParent, const xiiSimdTransformd& globalTransformChild); // [tested]

  /// \brief Sets this transform to the global transform, that is reached by applying the child's local transform to the parent's global
  /// one.
  void SetGlobalTransform(const xiiSimdTransformd& globalTransformParent, const xiiSimdTransformd& localTransformChild); // [tested]

  /// \brief Returns the transformation as a matrix.
  xiiSimdMat4d GetAsMat4() const; // [tested]

public:
  xiiSimdVec4d TransformPosition(const xiiSimdVec4d& v) const;  // [tested]
  xiiSimdVec4d TransformDirection(const xiiSimdVec4d& v) const; // [tested]

  /// \brief Concatenates the two transforms. This is the same as a matrix multiplication, thus not commutative.
  void operator*=(const xiiSimdTransformd& other); // [tested]

  /// \brief Multiplies \a q into the rotation component, thus rotating the entire transformation.
  void operator*=(const xiiSimdQuatd& q); // [tested]

  void operator+=(const xiiSimdVec4d& v); // [tested]
  void operator-=(const xiiSimdVec4d& v); // [tested]

public:
  xiiSimdVec4d m_Position;
  xiiSimdQuatd m_Rotation;
  xiiSimdVec4d m_Scale;
};

// *** free functions ***

/// \brief Transforms the vector v by the transform.
XII_ALWAYS_INLINE const xiiSimdVec4d operator*(const xiiSimdTransformd& t, const xiiSimdVec4d& v); // [tested]

/// \brief Rotates the transform by the given quaternion. Multiplies q from the left with t.
XII_ALWAYS_INLINE const xiiSimdTransformd operator*(const xiiSimdQuatd& q, const xiiSimdTransformd& t); // [tested]

/// \brief Rotates the transform by the given quaternion. Multiplies q from the right with t.
XII_ALWAYS_INLINE const xiiSimdTransformd operator*(const xiiSimdTransformd& t, const xiiSimdQuatd& q); // [tested]

/// \brief Translates the xiiSimdTransformd by the vector. This will move the object in global space.
XII_ALWAYS_INLINE const xiiSimdTransformd operator+(const xiiSimdTransformd& t, const xiiSimdVec4d& v); // [tested]

/// \brief Translates the xiiSimdTransformd by the vector. This will move the object in global space.
XII_ALWAYS_INLINE const xiiSimdTransformd operator-(const xiiSimdTransformd& t, const xiiSimdVec4d& v); // [tested]

/// \brief Concatenates the two transforms. This is the same as a matrix multiplication, thus not commutative.
XII_ALWAYS_INLINE const xiiSimdTransformd operator*(const xiiSimdTransformd& lhs, const xiiSimdTransformd& rhs); // [tested]

XII_ALWAYS_INLINE bool operator==(const xiiSimdTransformd& t1, const xiiSimdTransformd& t2); // [tested]
XII_ALWAYS_INLINE bool operator!=(const xiiSimdTransformd& t1, const xiiSimdTransformd& t2); // [tested]


#include <Foundation/SimdMath/Implementation/SimdTransformd_inl.h>
