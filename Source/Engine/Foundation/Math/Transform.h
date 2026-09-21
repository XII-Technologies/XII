/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Vec3.h>

/// \todo Fix docs and unit tests

/// A class that represents position, rotation and scaling via a position vector, a quaternion and a scale vector.
///
/// Scale is applied first, then rotation and finally translation is added. Thus scale and rotation are always in 'local space',
/// i.e. applying a rotation to the xiiTransformTemplate will rotate objects in place around their local center.
/// Since the translation is added afterwards, the translation component is always the global center position, around which
/// objects rotate.
///
/// The functions SetLocalTransform() and SetGlobalTransform() allow to create transforms that either represent the full
/// global transformation of an object, factoring its parent's transform in, or the local transformation that will get you
/// from the parent's global transformation to the current global transformation of a child (i.e. only the difference).
/// This is particularly useful when editing entities in a hierarchical structure.
///
/// This representation cannot handle shearing, which means rotations and scalings cannot be combined correctly.
/// Many parts of game engine cannot handle shearing or non-uniform scaling across hierarchies anyway. Therefore this
/// class implements a simplified way of combining scalings when multiplying two xiiTransform's. Instead of rotating scale into
/// the proper space, the two values are simply multiplied component-wise.
///
/// In situations where this is insufficient, use a 3x3 or 4x4 matrix instead. Sometimes it is sufficient to use the matrix for
/// the computation and the result can be stored in a transform again.
template <typename Type>
class xiiTransformTemplate
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  XII_DECLARE_POD_TYPE();

  using ComponentType = Type;

  // *** Data ***
public:
  xiiVec3Template<Type> m_vPosition;
  xiiQuatTemplate<Type> m_qRotation;
  xiiVec3Template<Type> m_vScale;

  // *** Constructors ***
public:
  /// Default constructor: Does not do any initialization.
  xiiTransformTemplate() = default;

  /// Initializes the transform from the given position, rotation and scale.
  explicit xiiTransformTemplate(const xiiVec3Template<Type>& vPosition, const xiiQuatTemplate<Type>& qRotation = xiiQuatTemplate<Type>::MakeIdentity(), const xiiVec3Template<Type>& vScale = xiiVec3Template<Type>(1)); // [tested]

  /// Creates a transform from the given position, rotation and scale.
  [[nodiscard]] static xiiTransformTemplate<Type> Make(const xiiVec3Template<Type>& vPosition, const xiiQuatTemplate<Type>& qRotation = xiiQuatTemplate<Type>::MakeIdentity(), const xiiVec3Template<Type>& vScale = xiiVec3Template<Type>(1));

  /// Creates an identity transform.
  [[nodiscard]] static xiiTransformTemplate<Type> MakeIdentity();

  /// Creates a transform from the given matrix.
  ///
  /// \note This operation always succeeds, even though the matrix may be complete garbage (e.g. a zero matrix)
  /// or may not be representable as a transform (containing shearing).
  /// Also be careful with mirroring. The transform may or may not be able to represent that.
  [[nodiscard]] static xiiTransformTemplate<Type> MakeFromMat4(const xiiMat4Template<Type>& mMat);

  /// Creates a transform that is the local transformation needed to get from the parent's transform to the child's.
  [[nodiscard]] static xiiTransformTemplate<Type> MakeLocalTransform(const xiiTransformTemplate& globalTransformParent, const xiiTransformTemplate& globalTransformChild); // [tested]

  /// Creates a transform that is the global transform, that is reached by applying the child's local transform to the parent's global one.
  [[nodiscard]] static xiiTransformTemplate<Type> MakeGlobalTransform(const xiiTransformTemplate& globalTransformParent, const xiiTransformTemplate& localTransformChild); // [tested]

  /// Sets the position to be zero and the rotation to identity.
  void SetIdentity(); // [tested]

  /// Returns the scale component with maximum magnitude.
  Type GetMaxScale() const;

  /// Returns whether this transform contains negative scaling aka mirroring.
  bool ContainsNegativeScale() const;

  /// Returns whether this transform contains uniform scaling.
  bool ContainsUniformScale() const;

  /// Checks that all components are valid (no NaN, only finite numbers).
  bool IsValid() const;

  // *** Equality ***
public:
  /// Equality Check (bitwise)
  bool IsIdentical(const xiiTransformTemplate& rhs) const; // [tested]

  /// Equality Check with epsilon
  bool IsEqual(const xiiTransformTemplate& rhs, Type fEpsilon) const; // [tested]

  // *** Inverse ***
public:
  /// Inverts this transform.
  void Invert(); // [tested]

  /// Returns the inverse of this transform.
  const xiiTransformTemplate GetInverse() const; // [tested]

  [[nodiscard]] xiiVec3Template<Type> TransformPosition(const xiiVec3Template<Type>& v) const;  // [tested]
  [[nodiscard]] xiiVec3Template<Type> TransformDirection(const xiiVec3Template<Type>& v) const; // [tested]

  void operator+=(const xiiVec3Template<Type>& v); // [tested]
  void operator-=(const xiiVec3Template<Type>& v); // [tested]

  // *** Conversion operations ***
public:
  /// Returns the transformation as a matrix.
  const xiiMat4Template<Type> GetAsMat4() const; // [tested]
};

// *** free functions ***

/// Transforms the vector v by the transform.
template <typename Type>
const xiiVec3Template<Type> operator*(const xiiTransformTemplate<Type>& t, const xiiVec3Template<Type>& v); // [tested]

/// Rotates the transform by the given quaternion. Multiplies q from the left with t.
template <typename Type>
const xiiTransformTemplate<Type> operator*(const xiiQuatTemplate<Type>& q, const xiiTransformTemplate<Type>& t); // [tested]

/// Rotates the transform by the given quaternion. Multiplies q from the right with t.
template <typename Type>
const xiiTransformTemplate<Type> operator*(const xiiTransformTemplate<Type>& t, const xiiQuatTemplate<Type>& q);

/// Translates the xiiTransform by the vector. This will move the object in global space.
template <typename Type>
const xiiTransformTemplate<Type> operator+(const xiiTransformTemplate<Type>& t, const xiiVec3Template<Type>& v); // [tested]

/// Translates the xiiTransform by the vector. This will move the object in global space.
template <typename Type>
const xiiTransformTemplate<Type> operator-(const xiiTransformTemplate<Type>& t, const xiiVec3Template<Type>& v); // [tested]

/// Concatenates the two transforms. This is the same as a matrix multiplication, thus not commutative.
template <typename Type>
const xiiTransformTemplate<Type> operator*(const xiiTransformTemplate<Type>& t1, const xiiTransformTemplate<Type>& t2); // [tested]

template <typename Type>
bool operator==(const xiiTransformTemplate<Type>& t1, const xiiTransformTemplate<Type>& t2); // [tested]

#include <Foundation/Math/Implementation/Transform_inl.h>
