/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/SimdMath/SimdBBoxd.h>

class XII_FOUNDATION_DLL xiiSimdBBoxSphered
{
public:
  XII_DECLARE_POD_TYPE();

  /// Default constructor does not initialize anything.
  xiiSimdBBoxSphered(); // [tested]

  /// Constructs the bounds from the given box. The sphere radius is calculated from the box extends.
  xiiSimdBBoxSphered(const xiiSimdBBoxd& box); // [tested]

  /// Constructs the bounds from the given sphere. The box extends are calculated from the sphere radius.
  xiiSimdBBoxSphered(const xiiSimdBSphered& sphere); // [tested]

  /// Creates an object with all zero values. These are valid bounds around the origin with no volume.
  [[nodiscard]] static xiiSimdBBoxSphered MakeZero();

  /// Creates an 'invalid' object, ie one with negative extents/radius. Invalid objects can be made valid through ExpandToInclude().
  [[nodiscard]] static xiiSimdBBoxSphered MakeInvalid(); // [tested]

  /// Creates an object from the given center point and extents.
  [[nodiscard]] static xiiSimdBBoxSphered MakeFromCenterExtents(const xiiSimdVec4d& vCenter, const xiiSimdVec4d& vBoxHalfExtents, const xiiSimdDouble& fSphereRadius);

  /// Creates an object that contains all the provided points.
  [[nodiscard]] static xiiSimdBBoxSphered MakeFromPoints(const xiiSimdVec4d* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride = sizeof(xiiSimdVec4d));

  /// Creates an object from another bounding box.
  [[nodiscard]] static xiiSimdBBoxSphered MakeFromBox(const xiiSimdBBoxd& box);

  /// Creates an object from another bounding sphere.
  [[nodiscard]] static xiiSimdBBoxSphered MakeFromSphere(const xiiSimdBSphered& sphere);

  /// Creates an object from another bounding box and a sphere.
  [[nodiscard]] static xiiSimdBBoxSphered MakeFromBoxAndSphere(const xiiSimdBBoxd& box, const xiiSimdBSphered& sphere);

public:
  /// Checks whether the bounds is in an invalid state.
  bool IsValid() const; // [tested]

  /// Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// Returns the bounding box.
  xiiSimdBBoxd GetBox() const; // [tested]

  /// Returns the bounding sphere.
  xiiSimdBSphered GetSphere() const; // [tested]

  /// Expands the bounds such that the given bounds are inside it.
  void ExpandToInclude(const xiiSimdBBoxSphered& rhs); // [tested]

  /// Transforms the bounds in its local space.
  void Transform(const xiiSimdTransformd& t); // [tested]

  /// Transforms the bounds in its local space.
  void Transform(const xiiSimdMat4d& mMat); // [tested]

  [[nodiscard]] bool operator==(const xiiSimdBBoxSphered& rhs) const; // [tested]
  [[nodiscard]] bool operator!=(const xiiSimdBBoxSphered& rhs) const; // [tested]

public:
  xiiSimdVec4d m_CenterAndRadius;
  xiiSimdVec4d m_BoxHalfExtents;
};

#include <Foundation/SimdMath/Implementation/SimdBBoxSphered_inl.h>
