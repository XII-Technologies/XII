#pragma once

#include <Foundation/SimdMath/SimdBBoxd.h>

class XII_FOUNDATION_DLL xiiSimdBBoxSphered
{
public:
  XII_DECLARE_POD_TYPE();

  /// \brief Default constructor does not initialize anything.
  xiiSimdBBoxSphered(); // [tested]

  /// \brief Constructs the bounds from the given box. The sphere radius is calculated from the box extends.
  xiiSimdBBoxSphered(const xiiSimdBBoxd& box); // [tested]

  /// \brief Constructs the bounds from the given sphere. The box extends are calculated from the sphere radius.
  xiiSimdBBoxSphered(const xiiSimdBSphered& sphere); // [tested]

  /// \brief Creates an object with all zero values. These are valid bounds around the origin with no volume.
  [[nodiscard]] static xiiSimdBBoxSphered MakeZero();

  /// \brief Creates an 'invalid' object, ie one with negative extents/radius. Invalid objects can be made valid through ExpandToInclude().
  [[nodiscard]] static xiiSimdBBoxSphered MakeInvalid(); // [tested]

  /// \brief Creates an object from the given center point and extents.
  [[nodiscard]] static xiiSimdBBoxSphered MakeFromCenterExtents(const xiiSimdVec4d& vCenter, const xiiSimdVec4d& vBoxHalfExtents, const xiiSimdDouble& fSphereRadius);

  /// \brief Creates an object that contains all the provided points.
  [[nodiscard]] static xiiSimdBBoxSphered MakeFromPoints(const xiiSimdVec4d* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride = sizeof(xiiSimdVec4d));

  /// \brief Creates an object from another bounding box.
  [[nodiscard]] static xiiSimdBBoxSphered MakeFromBox(const xiiSimdBBoxd& box);

  /// \brief Creates an object from another bounding sphere.
  [[nodiscard]] static xiiSimdBBoxSphered MakeFromSphere(const xiiSimdBSphered& sphere);

  /// \brief Creates an object from another bounding box and a sphere.
  [[nodiscard]] static xiiSimdBBoxSphered MakeFromBoxAndSphere(const xiiSimdBBoxd& box, const xiiSimdBSphered& sphere);

public:
  /// \brief Checks whether the bounds is in an invalid state.
  bool IsValid() const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

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

  [[nodiscard]] bool operator==(const xiiSimdBBoxSphered& rhs) const; // [tested]
  [[nodiscard]] bool operator!=(const xiiSimdBBoxSphered& rhs) const; // [tested]

public:
  xiiSimdVec4d m_CenterAndRadius;
  xiiSimdVec4d m_BoxHalfExtents;
};

#include <Foundation/SimdMath/Implementation/SimdBBoxSphered_inl.h>
