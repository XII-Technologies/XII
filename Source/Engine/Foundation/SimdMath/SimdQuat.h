/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/SimdMath/SimdMat4f.h>

class XII_FOUNDATION_DLL xiiSimdQuat
{
public:
  XII_DECLARE_POD_TYPE();

  xiiSimdQuat(); // [tested]

  explicit xiiSimdQuat(const xiiSimdVec4f& v); // [tested]

  /// Static function that returns a quaternion that represents the identity rotation (none).
  [[nodiscard]] static const xiiSimdQuat MakeIdentity(); // [tested]

  /// Sets the individual elements of the quaternion directly. Note that x,y,z do NOT represent a rotation axis, and w does NOT represent an angle.
  ///
  /// Use this function only if you have good understanding of quaternion math and know exactly what you are doing.
  [[nodiscard]] static xiiSimdQuat MakeFromElements(xiiSimdFloat x, xiiSimdFloat y, xiiSimdFloat z, xiiSimdFloat w); // [tested]

  /// Creates a quaternion from a rotation-axis and an angle (angle is given in Radians or as a xiiAngle)
  [[nodiscard]] static xiiSimdQuat MakeFromAxisAndAngle(const xiiSimdVec4f& vRotationAxis, const xiiSimdFloat& fAngle); // [tested]

  /// Creates a quaternion, that rotates through the shortest arc from "vDirFrom" to "vDirTo".
  [[nodiscard]] static xiiSimdQuat MakeShortestRotation(const xiiSimdVec4f& vDirFrom, const xiiSimdVec4f& vDirTo); // [tested]

  /// Returns a quaternion that is the spherical linear interpolation of the other two.
  [[nodiscard]] static xiiSimdQuat MakeSlerp(const xiiSimdQuat& qFrom, const xiiSimdQuat& qTo, const xiiSimdFloat& t); // [tested]

public:
  /// Normalizes the quaternion to unit length. ALL rotation-quaternions should be normalized at all times (automatically).
  void Normalize(); // [tested]

  /// Returns the rotation-axis and angle (in Radians), that this quaternion rotates around.
  xiiResult GetRotationAxisAndAngle(xiiSimdVec4f& ref_vAxis, xiiSimdFloat& ref_fAngle, const xiiSimdFloat& fEpsilon = xiiMath::DefaultEpsilon<float>()) const; // [tested]

  /// Returns the Quaternion as a matrix.
  xiiSimdMat4f GetAsMat4() const; // [tested]

  /// Checks whether all components are neither NaN nor infinite and that the quaternion is normalized.
  bool IsValid(const xiiSimdFloat& fEpsilon = xiiMath::DefaultEpsilon<float>()) const; // [tested]

  /// Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// Determines whether \a this and \a qOther represent the same rotation. This is a rather slow operation.
  ///
  /// Currently it fails when one of the given quaternions is identity (so no rotation, at all), as it tries to
  /// compare rotation axis' and angles, which is undefined for the identity quaternion (also there are infinite
  /// representations for 'identity', so it's difficult to check for it).
  bool IsEqualRotation(const xiiSimdQuat& qOther, const xiiSimdFloat& fEpsilon) const; // [tested]

public:
  /// Returns a Quaternion that represents the negative / inverted rotation.
  [[nodiscard]] xiiSimdQuat operator-() const; // [tested]

  /// Rotates v by q
  [[nodiscard]] xiiSimdVec4f operator*(const xiiSimdVec4f& v) const; // [tested]

  /// Concatenates the rotations of q1 and q2
  [[nodiscard]] xiiSimdQuat operator*(const xiiSimdQuat& q2) const; // [tested]

  bool operator==(const xiiSimdQuat& q2) const; // [tested]
  bool operator!=(const xiiSimdQuat& q2) const; // [tested]

public:
  xiiSimdVec4f m_v;
};

#include <Foundation/SimdMath/Implementation/SimdQuat_inl.h>
