/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/SimdMath/SimdTransform.h>

class XII_FOUNDATION_DLL xiiSimdBSphere
{
public:
  XII_DECLARE_POD_TYPE();

  /// \brief Default constructor does not initialize any data.
  xiiSimdBSphere();

  /// \brief Creates a sphere with the given radius around the given center.
  xiiSimdBSphere(const xiiSimdVec4f& vCenter, const xiiSimdFloat& fRadius); // [tested]

  /// \brief Creates a sphere at the origin with radius zero.
  [[nodiscard]] static xiiSimdBSphere MakeZero();

  /// \brief Creates an 'invalid' sphere, with its center at the given position and a negative radius.
  ///
  /// Such a sphere can be made 'valid' through ExpandToInclude(), but be aware that the originally provided center position
  /// will always be part of the sphere.
  [[nodiscard]] static xiiSimdBSphere MakeInvalid(const xiiSimdVec4f& vCenter = xiiSimdVec4f::MakeZero()); // [tested]

  /// \brief Creates a sphere with the provided center and radius.
  [[nodiscard]] static xiiSimdBSphere MakeFromCenterAndRadius(const xiiSimdVec4f& vCenter, const xiiSimdFloat& fRadius); // [tested]

  /// \brief Creates a bounding sphere around the provided points.
  ///
  /// The center of the sphere will be at the 'center of mass' of all the points, and the radius will be the distance to the
  /// farthest point from there.
  [[nodiscard]] static xiiSimdBSphere MakeFromPoints(const xiiSimdVec4f* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride = sizeof(xiiSimdVec4f));

public:
  /// \brief Returns whether the sphere has valid values.
  bool IsValid() const; // [tested]

  /// \brief Returns whether any value is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Returns the center
  xiiSimdVec4f GetCenter() const; // [tested]

  /// \brief Returns the radius
  xiiSimdFloat GetRadius() const; // [tested]

  /// \brief Increases the sphere's radius to include this point.
  void ExpandToInclude(const xiiSimdVec4f& vPoint); // [tested]

  /// \brief Increases the sphere's radius to include all given points. Does NOT change its position, thus the resulting sphere might be not
  /// a very tight fit. More efficient than calling this for every point individually.
  void ExpandToInclude(const xiiSimdVec4f* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride = sizeof(xiiSimdVec4f)); // [tested]

  /// \brief Increases this sphere's radius, such that it encloses the other sphere.
  void ExpandToInclude(const xiiSimdBSphere& rhs); // [tested]

public:
  /// \brief Transforms the sphere in its local space.
  void Transform(const xiiSimdTransform& t); // [tested]

  /// \brief Transforms the sphere in its local space.
  void Transform(const xiiSimdMat4f& mMat); // [tested]

public:
  /// \brief Computes the distance of the point to the sphere's surface. Returns negative values for points inside the sphere.
  xiiSimdFloat GetDistanceTo(const xiiSimdVec4f& vPoint) const; // [tested]

  /// \brief Returns the distance between the two spheres. Zero for spheres that are exactly touching each other, negative values for
  /// overlapping spheres.
  xiiSimdFloat GetDistanceTo(const xiiSimdBSphere& rhs) const; // [tested]

  /// \brief Returns true if the given point is inside the sphere.
  bool Contains(const xiiSimdVec4f& vPoint) const; // [tested]

  /// \brief Returns whether the other sphere is completely inside this sphere.
  bool Contains(const xiiSimdBSphere& rhs) const; // [tested]

  /// \brief Checks whether the two objects overlap.
  bool Overlaps(const xiiSimdBSphere& rhs) const; // [tested]

  /// \brief Clamps the given position to the volume of the sphere. The resulting point will always be inside the sphere, but have the
  /// closest distance to the original point.
  [[nodiscard]] xiiSimdVec4f GetClampedPoint(const xiiSimdVec4f& vPoint); // [tested]

  [[nodiscard]] bool operator==(const xiiSimdBSphere& rhs) const; // [tested]
  [[nodiscard]] bool operator!=(const xiiSimdBSphere& rhs) const; // [tested]

public:
  xiiSimdVec4f m_CenterAndRadius;
};

#include <Foundation/SimdMath/Implementation/SimdBSphere_inl.h>
