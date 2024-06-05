#pragma once

#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Vec4.h>

/// \brief A combination of a bounding box and a bounding sphere with the same center.
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
  /// \brief Default constructor does not initialize anything.
  xiiBoundingBoxSphereTemplate(); // [tested]

  xiiBoundingBoxSphereTemplate(const xiiBoundingBoxSphereTemplate& rhs);

  void operator=(const xiiBoundingBoxSphereTemplate& rhs);

  /// \brief Constructs the bounds from the given box. The sphere radius is calculated from the box extends.
  xiiBoundingBoxSphereTemplate(const xiiBoundingBoxTemplate<Type>& box); // [tested]

  /// \brief Constructs the bounds from the given sphere. The box extends are calculated from the sphere radius.
  xiiBoundingBoxSphereTemplate(const xiiBoundingSphereTemplate<Type>& sphere); // [tested]

  /// \brief Creates an object with all zero values. These are valid bounds around the origin with no volume.
  [[nodiscard]] static xiiBoundingBoxSphereTemplate<Type> MakeZero();

  /// \brief Creates an 'invalid' object, ie one with negative extents/radius. Invalid objects can be made valid through ExpandToInclude().
  [[nodiscard]] static xiiBoundingBoxSphereTemplate<Type> MakeInvalid();

  /// \brief Creates an object from the given center point and extents.
  [[nodiscard]] static xiiBoundingBoxSphereTemplate<Type> MakeFromCenterExtents(const xiiVec3Template<Type>& vCenter, const xiiVec3Template<Type>& vBoxHalfExtents, Type fSphereRadius);

  /// \brief Creates an object that contains all the provided points.
  [[nodiscard]] static xiiBoundingBoxSphereTemplate<Type> MakeFromPoints(const xiiVec3Template<Type>* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride = sizeof(xiiVec3Template<Type>));

  /// \brief Creates an object from another bounding box.
  [[nodiscard]] static xiiBoundingBoxSphereTemplate<Type> MakeFromBox(const xiiBoundingBoxTemplate<Type>& box);

  /// \brief Creates an object from another bounding sphere.
  [[nodiscard]] static xiiBoundingBoxSphereTemplate<Type> MakeFromSphere(const xiiBoundingSphereTemplate<Type>& sphere);

  /// \brief Creates an object from another bounding box and a sphere.
  [[nodiscard]] static xiiBoundingBoxSphereTemplate<Type> MakeFromBoxAndSphere(const xiiBoundingBoxTemplate<Type>& box, const xiiBoundingSphereTemplate<Type>& sphere);

#if XII_ENABLED(XII_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    XII_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please check that "
                                "all code-paths properly initialize this object.");
  }
#endif

  /// \brief Checks whether the bounds is in an invalid state.
  bool IsValid() const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Returns the bounding box.
  const xiiBoundingBoxTemplate<Type> GetBox() const; // [tested]

  /// \brief Returns the bounding sphere.
  const xiiBoundingSphereTemplate<Type> GetSphere() const; // [tested]

  /// \brief Expands the bounds such that the given bounds are inside it.
  void ExpandToInclude(const xiiBoundingBoxSphereTemplate& rhs); // [tested]

  /// \brief Transforms the bounds in its local space.
  void Transform(const xiiMat4Template<Type>& mTransform); // [tested]

public:
  xiiVec3Template<Type> m_vCenter;
  Type                  m_fSphereRadius;
  xiiVec3Template<Type> m_vBoxHalfExtends;
};

/// \brief Checks whether this bounds and the other are identical.
template <typename Type>
bool operator==(const xiiBoundingBoxSphereTemplate<Type>& lhs, const xiiBoundingBoxSphereTemplate<Type>& rhs); // [tested]

#include <Foundation/Math/Implementation/BoundingBoxSphere_inl.h>
