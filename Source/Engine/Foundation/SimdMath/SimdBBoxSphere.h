/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/SimdMath/SimdBBox.h>

class XII_FOUNDATION_DLL xiiSimdBBoxSphere
{
public:
  XII_DECLARE_POD_TYPE();

  /// Default constructor does not initialize anything.
  xiiSimdBBoxSphere(); // [tested]

  /// Constructs the bounds from the given box. The sphere radius is calculated from the box extends.
  xiiSimdBBoxSphere(const xiiSimdBBox& box); // [tested]

  /// Constructs the bounds from the given sphere. The box extends are calculated from the sphere radius.
  xiiSimdBBoxSphere(const xiiSimdBSphere& sphere); // [tested]

  /// Creates an object with all zero values. These are valid bounds around the origin with no volume.
  [[nodiscard]] static xiiSimdBBoxSphere MakeZero();

  /// Creates an 'invalid' object, ie one with negative extents/radius. Invalid objects can be made valid through ExpandToInclude().
  [[nodiscard]] static xiiSimdBBoxSphere MakeInvalid(); // [tested]

  /// Creates an object from the given center point and extents.
  [[nodiscard]] static xiiSimdBBoxSphere MakeFromCenterExtents(const xiiSimdVec4f& vCenter, const xiiSimdVec4f& vBoxHalfExtents, const xiiSimdFloat& fSphereRadius);

  /// Creates an object that contains all the provided points.
  [[nodiscard]] static xiiSimdBBoxSphere MakeFromPoints(const xiiSimdVec4f* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride = sizeof(xiiSimdVec4f));

  /// Creates an object from another bounding box.
  [[nodiscard]] static xiiSimdBBoxSphere MakeFromBox(const xiiSimdBBox& box);

  /// Creates an object from another bounding sphere.
  [[nodiscard]] static xiiSimdBBoxSphere MakeFromSphere(const xiiSimdBSphere& sphere);

  /// Creates an object from another bounding box and a sphere.
  [[nodiscard]] static xiiSimdBBoxSphere MakeFromBoxAndSphere(const xiiSimdBBox& box, const xiiSimdBSphere& sphere);

public:
  /// Checks whether the bounds is in an invalid state.
  bool IsValid() const; // [tested]

  /// Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// Returns the bounding box.
  xiiSimdBBox GetBox() const; // [tested]

  /// Returns the bounding sphere.
  xiiSimdBSphere GetSphere() const; // [tested]

  /// Expands the bounds such that the given bounds are inside it.
  void ExpandToInclude(const xiiSimdBBoxSphere& rhs); // [tested]

  /// Transforms the bounds in its local space.
  void Transform(const xiiSimdTransform& t); // [tested]

  /// Transforms the bounds in its local space.
  void Transform(const xiiSimdMat4f& mMat); // [tested]

  [[nodiscard]] bool operator==(const xiiSimdBBoxSphere& rhs) const; // [tested]
  [[nodiscard]] bool operator!=(const xiiSimdBBoxSphere& rhs) const; // [tested]

public:
  xiiSimdVec4f m_CenterAndRadius;
  xiiSimdVec4f m_BoxHalfExtents;
};

#include <Foundation/SimdMath/Implementation/SimdBBoxSphere_inl.h>
