#pragma once

#include <Foundation/SimdMath/SimdTransformd.h>

class XII_FOUNDATION_DLL xiiSimdBSphered
{
public:
  XII_DECLARE_POD_TYPE();

  /// \brief Default constructor does not initialize any data.
  xiiSimdBSphered();

  /// \brief Creates a sphere with the given radius around the given center.
  xiiSimdBSphered(const xiiSimdVec4d& vCenter, const xiiSimdDouble& fRadius); // [tested]

public:
  /// \brief Sets the bounding sphere to invalid values.
  void SetInvalid(); // [tested]

  /// \brief Returns whether the sphere has valid values.
  bool IsValid() const; // [tested]

  /// \brief Returns whether any value is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Returns the center
  xiiSimdVec4d GetCenter() const; // [tested]

  /// \brief Returns the radius
  xiiSimdDouble GetRadius() const; // [tested]

  /// \brief Initializes the sphere to be the bounding sphere of all the given points.
  void SetFromPoints(const xiiSimdVec4d* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride = sizeof(xiiSimdVec4d)); // [tested]

  /// \brief Increases the sphere's radius to include this point.
  void ExpandToInclude(const xiiSimdVec4d& vPoint); // [tested]

  /// \brief Increases the sphere's radius to include all given points. Does NOT change its position, thus the resulting sphere might be not
  /// a very tight fit. More efficient than calling this for every point individually.
  void ExpandToInclude(const xiiSimdVec4d* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride = sizeof(xiiSimdVec4d)); // [tested]

  /// \brief Increases this sphere's radius, such that it encloses the other sphere.
  void ExpandToInclude(const xiiSimdBSphered& rhs); // [tested]

public:
  /// \brief Transforms the sphere in its local space.
  void Transform(const xiiSimdTransformd& t); // [tested]

  /// \brief Transforms the sphere in its local space.
  void Transform(const xiiSimdMat4d& mMat); // [tested]

public:
  /// \brief Computes the distance of the point to the sphere's surface. Returns negative values for points inside the sphere.
  xiiSimdDouble GetDistanceTo(const xiiSimdVec4d& vPoint) const; // [tested]

  /// \brief Returns the distance between the two spheres. Zero for spheres that are exactly touching each other, negative values for
  /// overlapping spheres.
  xiiSimdDouble GetDistanceTo(const xiiSimdBSphered& rhs) const; // [tested]

  /// \brief Returns true if the given point is inside the sphere.
  bool Contains(const xiiSimdVec4d& vPoint) const; // [tested]

  /// \brief Returns whether the other sphere is completely inside this sphere.
  bool Contains(const xiiSimdBSphered& rhs) const; // [tested]

  /// \brief Checks whether the two objects overlap.
  bool Overlaps(const xiiSimdBSphered& rhs) const; // [tested]

  /// \brief Clamps the given position to the volume of the sphere. The resulting point will always be inside the sphere, but have the
  /// closest distance to the original point.
  xiiSimdVec4d GetClampedPoint(const xiiSimdVec4d& vPoint); // [tested]

  bool operator==(const xiiSimdBSphered& rhs) const; // [tested]
  bool operator!=(const xiiSimdBSphered& rhs) const; // [tested]

public:
  xiiSimdVec4d m_CenterAndRadius;
};

#include <Foundation/SimdMath/Implementation/SimdBSphered_inl.h>
