/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/SimdMath/SimdMat4d.h>

class XII_FOUNDATION_DLL xiiSimdQuatd
{
public:
  XII_DECLARE_POD_TYPE();

  xiiSimdQuatd(); // [tested]

  explicit xiiSimdQuatd(const xiiSimdVec4d& v); // [tested]

  /// Static function that returns a quaternion that represents the identity rotation (none).
  [[nodiscard]] static const xiiSimdQuatd MakeIdentity(); // [tested]

  /// Sets the individual elements of the quaternion directly. Note that x,y,z do NOT represent a rotation axis, and w does NOT represent an angle.
  ///
  /// Use this function only if you have good understanding of quaternion math and know exactly what you are doing.
  [[nodiscard]] static xiiSimdQuatd MakeFromElements(xiiSimdDouble x, xiiSimdDouble y, xiiSimdDouble z, xiiSimdDouble w); // [tested]

  /// Creates a quaternion from a rotation-axis and an angle (angle is given in Radians or as a xiiAngle)
  [[nodiscard]] static xiiSimdQuatd MakeFromAxisAndAngle(const xiiSimdVec4d& vRotationAxis, const xiiSimdDouble& fAngle); // [tested]

  /// Creates a quaternion, that rotates through the shortest arc from "vDirFrom" to "vDirTo".
  [[nodiscard]] static xiiSimdQuatd MakeShortestRotation(const xiiSimdVec4d& vDirFrom, const xiiSimdVec4d& vDirTo); // [tested]

  /// Returns a quaternion that is the spherical linear interpolation of the other two.
  [[nodiscard]] static xiiSimdQuatd MakeSlerp(const xiiSimdQuatd& qFrom, const xiiSimdQuatd& qTo, const xiiSimdDouble& t); // [tested]

public:
  /// Normalizes the quaternion to unit length. ALL rotation-quaternions should be normalized at all times (automatically).
  void Normalize(); // [tested]

  /// Returns the rotation-axis and angle (in Radians), that this quaternion rotates around.
  xiiResult GetRotationAxisAndAngle(xiiSimdVec4d& ref_vAxis, xiiSimdDouble& ref_fAngle, const xiiSimdDouble& fEpsilon = xiiMath::DefaultEpsilon<double>()) const; // [tested]

  /// Returns the Quaternion as a matrix.
  xiiSimdMat4d GetAsMat4() const; // [tested]

  /// Checks whether all components are neither NaN nor infinite and that the quaternion is normalized.
  bool IsValid(const xiiSimdDouble& fEpsilon = xiiMath::DefaultEpsilon<double>()) const; // [tested]

  /// Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// Determines whether \a this and \a qOther represent the same rotation. This is a rather slow operation.
  ///
  /// Currently it fails when one of the given quaternions is identity (so no rotation, at all), as it tries to
  /// compare rotation axis' and angles, which is undefined for the identity quaternion (also there are infinite
  /// representations for 'identity', so it's difficult to check for it).
  bool IsEqualRotation(const xiiSimdQuatd& other, const xiiSimdDouble& fEpsilon) const; // [tested]

public:
  /// Returns a Quaternion that represents the negative / inverted rotation.
  [[nodiscard]] xiiSimdQuatd operator-() const; // [tested]

  /// Rotates v by q
  [[nodiscard]] xiiSimdVec4d operator*(const xiiSimdVec4d& v) const; // [tested]

  /// Concatenates the rotations of q1 and q2
  [[nodiscard]] xiiSimdQuatd operator*(const xiiSimdQuatd& q2) const; // [tested]

  bool operator==(const xiiSimdQuatd& q2) const; // [tested]
  bool operator!=(const xiiSimdQuatd& q2) const; // [tested]

public:
  xiiSimdVec4d m_v;
};

#include <Foundation/SimdMath/Implementation/SimdQuatd_inl.h>
