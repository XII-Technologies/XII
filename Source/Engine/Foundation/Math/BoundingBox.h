/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Vec3.h>

/// An axis-aligned bounding box implementation.
///
/// This class allows to construct AABBs and also provides a large set of functions to work with them,
/// e.g. for overlap queries and ray casts.

template <typename Type>
class xiiBoundingBoxTemplate
{
public:
  // Means this object can be copied using memcpy instead of copy construction.
  XII_DECLARE_POD_TYPE();

  using ComponentType = Type;

public:
  /// Default constructor does not initialize anything.
  xiiBoundingBoxTemplate();

  /// Constructs the box with the given minimum and maximum values.
  xiiBoundingBoxTemplate(const xiiVec3Template<Type>& vMin, const xiiVec3Template<Type>& vMax); // [tested]

#if XII_ENABLED(XII_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    XII_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please check that "
                                "all code-paths properly initialize this object.");
  }
#endif

  /// Creates a box that is located at the origin and has zero size. This is a 'valid' box.
  [[nodiscard]] static xiiBoundingBoxTemplate<Type> MakeZero();

  /// Creates a box that is in an invalid state. ExpandToInclude can then be used to make it into a bounding box for objects.
  [[nodiscard]] static xiiBoundingBoxTemplate<Type> MakeInvalid(); // [tested]

  /// Creates a box from a center point and half-extents for each axis.
  [[nodiscard]] static xiiBoundingBoxTemplate<Type> MakeFromCenterAndHalfExtents(const xiiVec3Template<Type>& vCenter, const xiiVec3Template<Type>& vHalfExtents); // [tested]

  /// Creates a box with the given minimum and maximum values.
  [[nodiscard]] static xiiBoundingBoxTemplate<Type> MakeFromMinMax(const xiiVec3Template<Type>& vMin, const xiiVec3Template<Type>& vMax); // [tested]

  /// Creates a box around the given set of points. If uiNumPoints is zero, the returned box is invalid (same as MakeInvalid() returns).
  [[nodiscard]] static xiiBoundingBoxTemplate<Type> MakeFromPoints(const xiiVec3Template<Type>* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride = sizeof(xiiVec3Template<Type>)); // [tested]

  /// Checks whether the box is in an invalid state.
  bool IsValid() const; // [tested]

  /// Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// Writes the 8 different corners of the box to the given array.
  void GetCorners(xiiVec3Template<Type>* out_pCorners) const; // [tested]

  /// Returns the center position of the box.
  const xiiVec3Template<Type> GetCenter() const; // [tested]

  /// Returns the extents of the box along each axis.
  const xiiVec3Template<Type> GetExtents() const; // [tested]

  /// Returns the half extents of the box along each axis.
  const xiiVec3Template<Type> GetHalfExtents() const; // [tested]

  /// Expands the box such that the given point is inside it.
  void ExpandToInclude(const xiiVec3Template<Type>& vPoint); // [tested]

  /// Expands the box such that the given box is inside it.
  void ExpandToInclude(const xiiBoundingBoxTemplate& rhs); // [tested]

  /// Expands the box such that all the given points are inside it.
  void ExpandToInclude(const xiiVec3Template<Type>* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride = sizeof(xiiVec3Template<Type>)); // [tested]

  /// If the box is not cubic all extents are set to the value of the maximum extent, such that the box becomes cubic.
  void ExpandToCube(); // [tested]

  /// Will increase the size of the box in all directions by the given amount (per axis).
  void Grow(const xiiVec3Template<Type>& vDiff); // [tested]

  /// Checks whether the given point is inside the box.
  bool Contains(const xiiVec3Template<Type>& vPoint) const; // [tested]

  /// Checks whether the given box is completely inside this box.
  bool Contains(const xiiBoundingBoxTemplate& rhs) const; // [tested]

  /// Checks whether all the given points are inside this box.
  bool Contains(const xiiVec3Template<Type>* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride = sizeof(xiiVec3Template<Type>)) const; // [tested]

  /// Checks whether the given sphere is completely inside this box.
  bool Contains(const xiiBoundingSphereTemplate<Type>& sphere) const; // [tested]

  /// Checks whether this box overlaps with the given box.
  bool Overlaps(const xiiBoundingBoxTemplate& rhs) const; // [tested]

  /// Checks whether any of the given points is inside this box.
  bool Overlaps(const xiiVec3Template<Type>* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride = sizeof(xiiVec3Template<Type>)) const; // [tested]

  /// Checks whether the given sphere overlaps with this box.
  bool Overlaps(const xiiBoundingSphereTemplate<Type>& sphere) const; // [tested]

  /// Checks whether this box and the other box are exactly identical.
  bool IsIdentical(const xiiBoundingBoxTemplate& rhs) const; // [tested]

  /// Checks whether this box and the other box are equal within some threshold.
  bool IsEqual(const xiiBoundingBoxTemplate& rhs, Type fEpsilon = xiiMath::DefaultEpsilon<Type>()) const; // [tested]

  /// Moves the box by the given vector.
  void Translate(const xiiVec3Template<Type>& vDiff); // [tested]

  /// Scales the box along each axis, but keeps its center constant.
  void ScaleFromCenter(const xiiVec3Template<Type>& vScale); // [tested]

  /// Scales the box's corners by the given factors, thus also moves the box around.
  void ScaleFromOrigin(const xiiVec3Template<Type>& vScale); // [tested]

  /// Transforms the corners of the box in its local space. The center of the box does not change, unless the transform contains a translation.
  void TransformFromCenter(const xiiMat4Template<Type>& mTransform); // [tested]

  /// Transforms the corners of the box and recomputes the AABB of those transformed points. Rotations and scalings will influence the center position of the box.
  void TransformFromOrigin(const xiiMat4Template<Type>& mTransform); // [tested]

  /// The given point is clamped to the volume of the box, i.e. it will be either inside the box or on its surface and it will have the closest
  /// possible distance to the original point.
  const xiiVec3Template<Type> GetClampedPoint(const xiiVec3Template<Type>& vPoint) const; // [tested]

  /// Returns the squared minimum distance from the box's surface to the point. Zero if the point is inside the box.
  Type GetDistanceSquaredTo(const xiiVec3Template<Type>& vPoint) const; // [tested]

  /// Returns the minimum squared distance between the two boxes. Zero if the boxes overlap.
  Type GetDistanceSquaredTo(const xiiBoundingBoxTemplate& rhs) const; // [tested]

  /// Returns the minimum distance from the box's surface to the point. Zero if the point is inside the box.
  Type GetDistanceTo(const xiiVec3Template<Type>& vPoint) const; // [tested]

  /// Returns the minimum distance between the box and the sphere. Zero or negative if both overlap.
  Type GetDistanceTo(const xiiBoundingSphereTemplate<Type>& sphere) const; // [tested]

  /// Returns the minimum distance between the two boxes. Zero if the boxes overlap.
  Type GetDistanceTo(const xiiBoundingBoxTemplate& rhs) const; // [tested]

  /// Returns whether the given ray intersects the box. Optionally returns the intersection distance and position.
  /// Note that vRayDir is not required to be normalized.
  bool GetRayIntersection(const xiiVec3Template<Type>& vStartPos, const xiiVec3Template<Type>& vRayDir, Type* out_pIntersectionDistance = nullptr, xiiVec3Template<Type>* out_pIntersection = nullptr) const; // [tested]

  /// Checks whether the line segment intersects the box. Optionally returns the intersection point and the fraction along the line segment
  /// where the intersection occurred.
  bool GetLineSegmentIntersection(const xiiVec3Template<Type>& vStartPos, const xiiVec3Template<Type>& vEndPos, Type* out_pLineFraction = nullptr, xiiVec3Template<Type>* out_pIntersection = nullptr) const; // [tested]

  /// Returns a bounding sphere that encloses this box.
  const xiiBoundingSphereTemplate<Type> GetBoundingSphere() const; // [tested]

public:
  xiiVec3Template<Type> m_vMin;
  xiiVec3Template<Type> m_vMax;
};

/// Checks whether this box and the other are identical.
template <typename Type>
bool operator==(const xiiBoundingBoxTemplate<Type>& lhs, const xiiBoundingBoxTemplate<Type>& rhs); // [tested]

#include <Foundation/Math/Implementation/BoundingBox_inl.h>
