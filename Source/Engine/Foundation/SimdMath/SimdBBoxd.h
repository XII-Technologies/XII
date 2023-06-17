#pragma once

#include <Foundation/SimdMath/SimdBSphered.h>

class xiiSimdBBoxd
{
public:
  XII_DECLARE_POD_TYPE();

  /// \brief Default constructor does not initialize anything.
  xiiSimdBBoxd();

  /// \brief Constructs the box with the given minimum and maximum values.
  xiiSimdBBoxd(const xiiSimdVec4d& vMin, const xiiSimdVec4d& vMax); // [tested]

public:
  /// \brief Resets the box to an invalid state. ExpandToInclude can then be used to make it into a bounding box for objects.
  void SetInvalid(); // [tested]

  /// \brief Sets the box from a center point and half-extents for each axis.
  void SetCenterAndHalfExtents(const xiiSimdVec4d& vCenter, const xiiSimdVec4d& vHalfExtents); // [tested]

  /// \brief Creates a new bounding-box around the given set of points.
  void SetFromPoints(const xiiSimdVec4d* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride = sizeof(xiiSimdVec4d)); // [tested]

  /// \brief Checks whether the box is in an invalid state.
  bool IsValid() const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Returns the center position of the box.
  xiiSimdVec4d GetCenter() const; // [tested]

  /// \brief Returns the extents of the box along each axis.
  xiiSimdVec4d GetExtents() const; // [tested]

  /// \brief Returns the half extents of the box along each axis.
  xiiSimdVec4d GetHalfExtents() const; // [tested]

  /// \brief Expands the box such that the given point is inside it.
  void ExpandToInclude(const xiiSimdVec4d& vPoint); // [tested]

  /// \brief Expands the box such that all the given points are inside it.
  void ExpandToInclude(const xiiSimdVec4d* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride = sizeof(xiiSimdVec4d)); // [tested]

  /// \brief Expands the box such that the given box is inside it.
  void ExpandToInclude(const xiiSimdBBoxd& rhs); // [tested]

  /// \brief If the box is not cubic all extents are set to the value of the maximum extent, such that the box becomes cubic.
  void ExpandToCube(); // [tested]


  /// \brief Checks whether the given point is inside the box.
  bool Contains(const xiiSimdVec4d& vPoint) const; // [tested]

  /// \brief Checks whether the given box is completely inside this box.
  bool Contains(const xiiSimdBBoxd& rhs) const; // [tested]

  /// \brief Checks whether the given sphere is completely inside this box.
  bool Contains(const xiiSimdBSphered& sphere) const; // [tested]

  /// \brief Checks whether this box overlaps with the given box.
  bool Overlaps(const xiiSimdBBoxd& rhs) const; // [tested]

  /// \brief Checks whether the given sphere overlaps with this box.
  bool Overlaps(const xiiSimdBSphered& sphere) const; // [tested]


  /// \brief Will increase the size of the box in all directions by the given amount (per axis).
  void Grow(const xiiSimdVec4d& vDiff); // [tested]

  /// \brief Moves the box by the given vector.
  void Translate(const xiiSimdVec4d& vDiff); // [tested]

  /// \brief Transforms the corners of the box and recomputes the aabb of those transformed points.
  void Transform(const xiiSimdTransformd& transform); // [tested]

  /// \brief Transforms the corners of the box and recomputes the aabb of those transformed points.
  void Transform(const xiiSimdMat4d& mMat); // [tested]


  /// \brief The given point is clamped to the volume of the box, i.e. it will be either inside the box or on its surface and it will have the closest
  /// possible distance to the original point.
  xiiSimdVec4d GetClampedPoint(const xiiSimdVec4d& vPoint) const; // [tested]

  /// \brief Returns the squared minimum distance from the box's surface to the point. Zero if the point is inside the box.
  xiiSimdDouble GetDistanceSquaredTo(const xiiSimdVec4d& vPoint) const; // [tested]

  /// \brief Returns the minimum distance from the box's surface to the point. Zero if the point is inside the box.
  xiiSimdDouble GetDistanceTo(const xiiSimdVec4d& vPoint) const; // [tested]


  bool operator==(const xiiSimdBBoxd& rhs) const; // [tested]
  bool operator!=(const xiiSimdBBoxd& rhs) const; // [tested]

public:
  xiiSimdVec4d m_Min;
  xiiSimdVec4d m_Max;
};

#include <Foundation/SimdMath/Implementation/SimdBBoxd_inl.h>
