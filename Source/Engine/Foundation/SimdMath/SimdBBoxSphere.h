#pragma once

#include <Foundation/SimdMath/SimdBBox.h>

class XII_FOUNDATION_DLL xiiSimdBBoxSphere
{
public:
  XII_DECLARE_POD_TYPE();

  /// \brief Default constructor does not initialize anything.
  xiiSimdBBoxSphere(); // [tested]

  /// \brief Constructs the bounds from the given box. The sphere radius is calculated from the box extends.
  xiiSimdBBoxSphere(const xiiSimdBBox& box); // [tested]

  /// \brief Constructs the bounds from the given sphere. The box extends are calculated from the sphere radius.
  xiiSimdBBoxSphere(const xiiSimdBSphere& sphere); // [tested]

  /// \brief Creates an object with all zero values. These are valid bounds around the origin with no volume.
  [[nodiscard]] static xiiSimdBBoxSphere MakeZero();

  /// \brief Creates an 'invalid' object, ie one with negative extents/radius. Invalid objects can be made valid through ExpandToInclude().
  [[nodiscard]] static xiiSimdBBoxSphere MakeInvalid(); // [tested]

  /// \brief Creates an object from the given center point and extents.
  [[nodiscard]] static xiiSimdBBoxSphere MakeFromCenterExtents(const xiiSimdVec4f& vCenter, const xiiSimdVec4f& vBoxHalfExtents, const xiiSimdFloat& fSphereRadius);

  /// \brief Creates an object that contains all the provided points.
  [[nodiscard]] static xiiSimdBBoxSphere MakeFromPoints(const xiiSimdVec4f* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride = sizeof(xiiSimdVec4f));

  /// \brief Creates an object from another bounding box.
  [[nodiscard]] static xiiSimdBBoxSphere MakeFromBox(const xiiSimdBBox& box);

  /// \brief Creates an object from another bounding sphere.
  [[nodiscard]] static xiiSimdBBoxSphere MakeFromSphere(const xiiSimdBSphere& sphere);

  /// \brief Creates an object from another bounding box and a sphere.
  [[nodiscard]] static xiiSimdBBoxSphere MakeFromBoxAndSphere(const xiiSimdBBox& box, const xiiSimdBSphere& sphere);

public:
  /// \brief Checks whether the bounds is in an invalid state.
  bool IsValid() const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Returns the bounding box.
  xiiSimdBBox GetBox() const; // [tested]

  /// \brief Returns the bounding sphere.
  xiiSimdBSphere GetSphere() const; // [tested]

  /// \brief Expands the bounds such that the given bounds are inside it.
  void ExpandToInclude(const xiiSimdBBoxSphere& rhs); // [tested]

  /// \brief Transforms the bounds in its local space.
  void Transform(const xiiSimdTransform& t); // [tested]

  /// \brief Transforms the bounds in its local space.
  void Transform(const xiiSimdMat4f& mMat); // [tested]

  [[nodiscard]] bool operator==(const xiiSimdBBoxSphere& rhs) const; // [tested]
  [[nodiscard]] bool operator!=(const xiiSimdBBoxSphere& rhs) const; // [tested]

public:
  xiiSimdVec4f m_CenterAndRadius;
  xiiSimdVec4f m_BoxHalfExtents;
};

#include <Foundation/SimdMath/Implementation/SimdBBoxSphere_inl.h>
