#pragma once

#include <Foundation/SimdMath/SimdBBoxd.h>

class XII_FOUNDATION_DLL xiiSimdBBoxSphered
{
public:
  XII_DECLARE_POD_TYPE();

  /// \brief Default constructor does not initialize anything.
  xiiSimdBBoxSphered(); // [tested]

  /// \brief Constructs the bounds from the center position, the box half extends and the sphere radius.
  xiiSimdBBoxSphered(const xiiSimdVec4d& vCenter, const xiiSimdVec4d& vBoxHalfExtents, const xiiSimdDouble& fSphereRadius); // [tested]

  /// \brief Constructs the bounds from the given box and sphere.
  xiiSimdBBoxSphered(const xiiSimdBBoxd& box, const xiiSimdBSphered& sphere); // [tested]

  /// \brief Constructs the bounds from the given box. The sphere radius is calculated from the box extends.
  xiiSimdBBoxSphered(const xiiSimdBBoxd& box); // [tested]

  /// \brief Constructs the bounds from the given sphere. The box extends are calculated from the sphere radius.
  xiiSimdBBoxSphered(const xiiSimdBSphered& sphere); // [tested]

public:
  /// \brief Resets the bounds to an invalid state.
  void SetInvalid(); // [tested]

  /// \brief Checks whether the bounds is in an invalid state.
  bool IsValid() const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Calculates the bounds from given set of points.
  void SetFromPoints(const xiiSimdVec4d* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride = sizeof(xiiSimdVec4d)); // [tested]

  /// \brief Returns the bounding box.
  xiiSimdBBoxd GetBox() const; // [tested]

  /// \brief Returns the bounding sphere.
  xiiSimdBSphered GetSphere() const; // [tested]

  /// \brief Expands the bounds such that the given bounds are inside it.
  void ExpandToInclude(const xiiSimdBBoxSphered& rhs); // [tested]

  /// \brief Transforms the bounds in its local space.
  void Transform(const xiiSimdTransformd& t); // [tested]

  /// \brief Transforms the bounds in its local space.
  void Transform(const xiiSimdMat4d& mMat); // [tested]


  bool operator==(const xiiSimdBBoxSphered& rhs) const; // [tested]
  bool operator!=(const xiiSimdBBoxSphered& rhs) const; // [tested]

public:
  xiiSimdVec4d m_CenterAndRadius;
  xiiSimdVec4d m_BoxHalfExtents;
};

#include <Foundation/SimdMath/Implementation/SimdBBoxSphered_inl.h>
