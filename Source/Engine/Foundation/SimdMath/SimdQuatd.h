#pragma once

#include <Foundation/SimdMath/SimdMat4d.h>

class XII_FOUNDATION_DLL xiiSimdQuatd
{
public:
  XII_DECLARE_POD_TYPE();

  xiiSimdQuatd(); // [tested]

  xiiSimdQuatd(const xiiSimdVec4d& v); // [tested]

  /// \brief Static function that returns a quaternion that represents the identity rotation (none).
  static xiiSimdQuatd IdentityQuaternion(); // [tested]

public:
  /// \brief Sets the Quaternion to the identity.
  void SetIdentity(); // [tested]

  /// \brief Creates a quaternion from a rotation-axis and an angle (angle is given in Radians or as an xiiAngle)
  void SetFromAxisAndAngle(const xiiSimdVec4d& vRotationAxis, const xiiSimdDouble& fAngle); // [tested]

  /// \brief Creates a quaternion, that rotates through the shortest arc from "vDirFrom" to "vDirTo".
  void SetShortestRotation(const xiiSimdVec4d& vDirFrom, const xiiSimdVec4d& vDirTo); // [tested]

  /// \brief Sets this quaternion to be the spherical linear interpolation of the other two.
  void SetSlerp(const xiiSimdQuatd& qFrom, const xiiSimdQuatd& qTo, const xiiSimdDouble& t); // [tested]

public:
  /// \brief Normalizes the quaternion to unit length. ALL rotation-quaternions should be normalized at all times (automatically).
  void Normalize(); // [tested]

  /// \brief Returns the rotation-axis and angle (in Radians), that this quaternion rotates around.
  xiiResult GetRotationAxisAndAngle(xiiSimdVec4d& ref_vAxis, xiiSimdDouble& ref_fAngle, const xiiSimdDouble& fEpsilon = xiiMath::DefaultEpsilon<double>()) const; // [tested]

  /// \brief Returns the Quaternion as a matrix.
  xiiSimdMat4d GetAsMat4() const; // [tested]

  /// \brief Checks whether all components are neither NaN nor infinite and that the quaternion is normalized.
  bool IsValid(const xiiSimdDouble& fEpsilon = xiiMath::DefaultEpsilon<double>()) const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Determines whether \a this and \a qOther represent the same rotation. This is a rather slow operation.
  ///
  /// Currently it fails when one of the given quaternions is identity (so no rotation, at all), as it tries to
  /// compare rotation axis' and angles, which is undefined for the identity quaternion (also there are infinite
  /// representations for 'identity', so it's difficult to check for it).
  bool IsEqualRotation(const xiiSimdQuatd& qOther, const xiiSimdDouble& fEpsilon) const; // [tested]

public:
  /// \brief Returns a Quaternion that represents the negative / inverted rotation.
  xiiSimdQuatd operator-() const; // [tested]

  /// \brief Rotates v by q
  xiiSimdVec4d operator*(const xiiSimdVec4d& v) const; // [tested]

  /// \brief Concatenates the rotations of q1 and q2
  xiiSimdQuatd operator*(const xiiSimdQuatd& q2) const; // [tested]

  bool operator==(const xiiSimdQuatd& q2) const; // [tested]
  bool operator!=(const xiiSimdQuatd& q2) const; // [tested]

public:
  xiiSimdVec4d m_v;
};

#include <Foundation/SimdMath/Implementation/SimdQuatd_inl.h>
