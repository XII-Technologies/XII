#pragma once

#include <Foundation/SimdMath/SimdMat4f.h>

class XII_FOUNDATION_DLL xiiSimdQuat
{
public:
  XII_DECLARE_POD_TYPE();

  xiiSimdQuat(); // [tested]

  xiiSimdQuat(const xiiSimdVec4f& v); // [tested]

  /// \brief Static function that returns a quaternion that represents the identity rotation (none).
  static xiiSimdQuat IdentityQuaternion(); // [tested]

public:
  /// \brief Sets the Quaternion to the identity.
  void SetIdentity(); // [tested]

  /// \brief Creates a quaternion from a rotation-axis and an angle (angle is given in Radians or as a xiiAngle)
  void SetFromAxisAndAngle(const xiiSimdVec4f& vRotationAxis, const xiiSimdFloat& fAngle); // [tested]

  /// \brief Creates a quaternion, that rotates through the shortest arc from "vDirFrom" to "vDirTo".
  void SetShortestRotation(const xiiSimdVec4f& vDirFrom, const xiiSimdVec4f& vDirTo); // [tested]

  /// \brief Sets this quaternion to be the spherical linear interpolation of the other two.
  void SetSlerp(const xiiSimdQuat& qFrom, const xiiSimdQuat& qTo, const xiiSimdFloat& t); // [tested]

public:
  /// \brief Normalizes the quaternion to unit length. ALL rotation-quaternions should be normalized at all times (automatically).
  void Normalize(); // [tested]

  /// \brief Returns the rotation-axis and angle (in Radians), that this quaternion rotates around.
  xiiResult GetRotationAxisAndAngle(xiiSimdVec4f& ref_vAxis, xiiSimdFloat& ref_fAngle, const xiiSimdFloat& fEpsilon = xiiMath::DefaultEpsilon<float>()) const; // [tested]

  /// \brief Returns the Quaternion as a matrix.
  xiiSimdMat4f GetAsMat4() const; // [tested]

  /// \brief Checks whether all components are neither NaN nor infinite and that the quaternion is normalized.
  bool IsValid(const xiiSimdFloat& fEpsilon = xiiMath::DefaultEpsilon<float>()) const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Determines whether \a this and \a qOther represent the same rotation. This is a rather slow operation.
  ///
  /// Currently it fails when one of the given quaternions is identity (so no rotation, at all), as it tries to
  /// compare rotation axis' and angles, which is undefined for the identity quaternion (also there are infinite
  /// representations for 'identity', so it's difficult to check for it).
  bool IsEqualRotation(const xiiSimdQuat& qOther, const xiiSimdFloat& fEpsilon) const; // [tested]

public:
  /// \brief Returns a Quaternion that represents the negative / inverted rotation.
  xiiSimdQuat operator-() const; // [tested]

  /// \brief Rotates v by q
  xiiSimdVec4f operator*(const xiiSimdVec4f& v) const; // [tested]

  /// \brief Concatenates the rotations of q1 and q2
  xiiSimdQuat operator*(const xiiSimdQuat& q2) const; // [tested]

  bool operator==(const xiiSimdQuat& q2) const; // [tested]
  bool operator!=(const xiiSimdQuat& q2) const; // [tested]

public:
  xiiSimdVec4f m_v;
};

#include <Foundation/SimdMath/Implementation/SimdQuat_inl.h>
