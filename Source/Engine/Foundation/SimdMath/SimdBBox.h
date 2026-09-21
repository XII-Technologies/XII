/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/SimdMath/SimdBSphere.h>

class xiiSimdBBox
{
public:
  XII_DECLARE_POD_TYPE();

  /// Default constructor does not initialize anything.
  xiiSimdBBox();

  /// Constructs the box with the given minimum and maximum values.
  xiiSimdBBox(const xiiSimdVec4f& vMin, const xiiSimdVec4f& vMax); // [tested]

  /// Creates a box that is located at the origin and has zero size. This is a 'valid' box.
  [[nodiscard]] static xiiSimdBBox MakeZero();

  /// Creates a box that is in an invalid state. ExpandToInclude can then be used to make it into a bounding box for objects.
  [[nodiscard]] static xiiSimdBBox MakeInvalid(); // [tested]

  /// Creates a box from a center point and half-extents for each axis.
  [[nodiscard]] static xiiSimdBBox MakeFromCenterAndHalfExtents(const xiiSimdVec4f& vCenter, const xiiSimdVec4f& vHalfExtents); // [tested]

  /// Creates a box with the given minimum and maximum values.
  [[nodiscard]] static xiiSimdBBox MakeFromMinMax(const xiiSimdVec4f& vMin, const xiiSimdVec4f& vMax); // [tested]

  /// Creates a box around the given set of points. If uiNumPoints is zero, the returned box is invalid (same as MakeInvalid() returns).
  [[nodiscard]] static xiiSimdBBox MakeFromPoints(const xiiSimdVec4f* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride = sizeof(xiiSimdVec4f)); // [tested]

public:
  /// Checks whether the box is in an invalid state.
  bool IsValid() const; // [tested]

  /// Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// Returns the center position of the box.
  xiiSimdVec4f GetCenter() const; // [tested]

  /// Returns the extents of the box along each axis.
  xiiSimdVec4f GetExtents() const; // [tested]

  /// Returns the half extents of the box along each axis.
  xiiSimdVec4f GetHalfExtents() const; // [tested]

  /// Expands the box such that the given point is inside it.
  void ExpandToInclude(const xiiSimdVec4f& vPoint); // [tested]

  /// Expands the box such that all the given points are inside it.
  void ExpandToInclude(const xiiSimdVec4f* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride = sizeof(xiiSimdVec4f)); // [tested]

  /// Expands the box such that the given box is inside it.
  void ExpandToInclude(const xiiSimdBBox& rhs); // [tested]

  /// If the box is not cubic all extents are set to the value of the maximum extent, such that the box becomes cubic.
  void ExpandToCube(); // [tested]


  /// Checks whether the given point is inside the box.
  bool Contains(const xiiSimdVec4f& vPoint) const; // [tested]

  /// Checks whether the given box is completely inside this box.
  bool Contains(const xiiSimdBBox& rhs) const; // [tested]

  /// Checks whether the given sphere is completely inside this box.
  bool Contains(const xiiSimdBSphere& sphere) const; // [tested]

  /// Checks whether this box overlaps with the given box.
  bool Overlaps(const xiiSimdBBox& rhs) const; // [tested]

  /// Checks whether the given sphere overlaps with this box.
  bool Overlaps(const xiiSimdBSphere& sphere) const; // [tested]


  /// Will increase the size of the box in all directions by the given amount (per axis).
  void Grow(const xiiSimdVec4f& vDiff); // [tested]

  /// Moves the box by the given vector.
  void Translate(const xiiSimdVec4f& vDiff); // [tested]

  /// Transforms the corners of the box and recomputes the aabb of those transformed points.
  void Transform(const xiiSimdTransform& transform); // [tested]

  /// Transforms the corners of the box and recomputes the aabb of those transformed points.
  void Transform(const xiiSimdMat4f& mMat); // [tested]


  /// The given point is clamped to the volume of the box, i.e. it will be either inside the box or on its surface and it will have the closest
  /// possible distance to the original point.
  xiiSimdVec4f GetClampedPoint(const xiiSimdVec4f& vPoint) const; // [tested]

  /// Returns the squared minimum distance from the box's surface to the point. Zero if the point is inside the box.
  xiiSimdFloat GetDistanceSquaredTo(const xiiSimdVec4f& vPoint) const; // [tested]

  /// Returns the minimum distance from the box's surface to the point. Zero if the point is inside the box.
  xiiSimdFloat GetDistanceTo(const xiiSimdVec4f& vPoint) const; // [tested]


  bool operator==(const xiiSimdBBox& rhs) const; // [tested]
  bool operator!=(const xiiSimdBBox& rhs) const; // [tested]

public:
  xiiSimdVec4f m_Min;
  xiiSimdVec4f m_Max;
};

#include <Foundation/SimdMath/Implementation/SimdBBox_inl.h>
