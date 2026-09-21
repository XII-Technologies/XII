/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Vec4.h>

/// A combination of a bounding box and a bounding sphere with the same center.
///
/// This class uses less memory than storying a bounding box and sphere separate.

template <typename Type>
class xiiBoundingBoxSphereTemplate
{
public:
  // Means this object can be copied using memcpy instead of copy construction.
  XII_DECLARE_POD_TYPE();

  using ComponentType = Type;

public:
  /// Default constructor does not initialize anything.
  xiiBoundingBoxSphereTemplate(); // [tested]

  xiiBoundingBoxSphereTemplate(const xiiBoundingBoxSphereTemplate& rhs);

  void operator=(const xiiBoundingBoxSphereTemplate& rhs);

  /// Constructs the bounds from the given box. The sphere radius is calculated from the box extends.
  xiiBoundingBoxSphereTemplate(const xiiBoundingBoxTemplate<Type>& box); // [tested]

  /// Constructs the bounds from the given sphere. The box extends are calculated from the sphere radius.
  xiiBoundingBoxSphereTemplate(const xiiBoundingSphereTemplate<Type>& sphere); // [tested]

  /// Creates an object with all zero values. These are valid bounds around the origin with no volume.
  [[nodiscard]] static xiiBoundingBoxSphereTemplate<Type> MakeZero();

  /// Creates an 'invalid' object, ie one with negative extents/radius. Invalid objects can be made valid through ExpandToInclude().
  [[nodiscard]] static xiiBoundingBoxSphereTemplate<Type> MakeInvalid();

  /// Creates an object from the given center point and extents.
  [[nodiscard]] static xiiBoundingBoxSphereTemplate<Type> MakeFromCenterExtents(const xiiVec3Template<Type>& vCenter, const xiiVec3Template<Type>& vBoxHalfExtents, Type fSphereRadius);

  /// Creates an object that contains all the provided points.
  [[nodiscard]] static xiiBoundingBoxSphereTemplate<Type> MakeFromPoints(const xiiVec3Template<Type>* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride = sizeof(xiiVec3Template<Type>));

  /// Creates an object from another bounding box.
  [[nodiscard]] static xiiBoundingBoxSphereTemplate<Type> MakeFromBox(const xiiBoundingBoxTemplate<Type>& box);

  /// Creates an object from another bounding sphere.
  [[nodiscard]] static xiiBoundingBoxSphereTemplate<Type> MakeFromSphere(const xiiBoundingSphereTemplate<Type>& sphere);

  /// Creates an object from another bounding box and a sphere.
  [[nodiscard]] static xiiBoundingBoxSphereTemplate<Type> MakeFromBoxAndSphere(const xiiBoundingBoxTemplate<Type>& box, const xiiBoundingSphereTemplate<Type>& sphere);

#if XII_ENABLED(XII_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    XII_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please check that "
                                "all code-paths properly initialize this object.");
  }
#endif

  /// Checks whether the bounds is in an invalid state.
  bool IsValid() const; // [tested]

  /// Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// Returns the bounding box.
  const xiiBoundingBoxTemplate<Type> GetBox() const; // [tested]

  /// Returns the bounding sphere.
  const xiiBoundingSphereTemplate<Type> GetSphere() const; // [tested]

  /// Expands the bounds such that the given bounds are inside it.
  void ExpandToInclude(const xiiBoundingBoxSphereTemplate& rhs); // [tested]

  /// Transforms the bounds in its local space.
  void Transform(const xiiMat4Template<Type>& mTransform); // [tested]

public:
  xiiVec3Template<Type> m_vCenter;
  Type                  m_fSphereRadius;
  xiiVec3Template<Type> m_vBoxHalfExtents;
};

/// Checks whether this bounds and the other are identical.
template <typename Type>
bool operator==(const xiiBoundingBoxSphereTemplate<Type>& lhs, const xiiBoundingBoxSphereTemplate<Type>& rhs); // [tested]

#include <Foundation/Math/Implementation/BoundingBoxSphere_inl.h>
