#pragma once

#include <Foundation/SimdMath/SimdBBox.h>

class XII_FOUNDATION_DLL xiiSimdBBoxSphere
{
public:
  XII_DECLARE_POD_TYPE();

  /// \brief Default constructor does not initialize anything.
  xiiSimdBBoxSphere(); // [tested]

  /// \brief Constructs the bounds from the center position, the box half extends and the sphere radius.
  xiiSimdBBoxSphere(const xiiSimdVec4f& vCenter, const xiiSimdVec4f& vBoxHalfExtents, const xiiSimdFloat& fSphereRadius); // [tested]

  /// \brief Constructs the bounds from the given box and sphere.
  xiiSimdBBoxSphere(const xiiSimdBBox& box, const xiiSimdBSphere& sphere); // [tested]

  /// \brief Constructs the bounds from the given box. The sphere radius is calculated from the box extends.
  xiiSimdBBoxSphere(const xiiSimdBBox& box); // [tested]

  /// \brief Constructs the bounds from the given sphere. The box extends are calculated from the sphere radius.
  xiiSimdBBoxSphere(const xiiSimdBSphere& sphere); // [tested]

public:
  /// \brief Resets the bounds to an invalid state.
  void SetInvalid(); // [tested]

  /// \brief Checks whether the bounds is in an invalid state.
  bool IsValid() const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Calculates the bounds from given set of points.
  void SetFromPoints(const xiiSimdVec4f* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride = sizeof(xiiSimdVec4f)); // [tested]

  /// \brief Returns the bounding box.
  xiiSimdBBox GetBox() const; // [tested]

  /// \brief Returns the bounding sphere.
  xiiSimdBSphere GetSphere() const; // [tested]

  /// \brief Expands the bounds such that the given bounds are inside it.
  void ExpandToInclude(const xiiSimdBBoxSphere& rhs); // [tested]

  /// \brief Transforms the bounds in its local space.
  void Transform(const xiiSimdTransform& t); // [tested]

  /// \brief Transforms the bounds in its local space.
  void Transform(const xiiSimdMat4f& mat); // [tested]


  bool operator==(const xiiSimdBBoxSphere& rhs) const; // [tested]
  bool operator!=(const xiiSimdBBoxSphere& rhs) const; // [tested]

public:
  xiiSimdVec4f m_CenterAndRadius;
  xiiSimdVec4f m_BoxHalfExtents;
};

#include <Foundation/SimdMath/Implementation/SimdBBoxSphere_inl.h>
