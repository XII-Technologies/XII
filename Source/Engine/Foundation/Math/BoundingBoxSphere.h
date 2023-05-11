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

  /// \brief Constructs the bounds from the center position, the box half extends and the sphere radius.
  xiiBoundingBoxSphereTemplate(const xiiVec3Template<Type>& vCenter, const xiiVec3Template<Type>& vBoxHalfExtents, Type fSphereRadius); // [tested]

  /// \brief Constructs the bounds from the given box and sphere.
  xiiBoundingBoxSphereTemplate(const xiiBoundingBoxTemplate<Type>& box, const xiiBoundingSphereTemplate<Type>& sphere); // [tested]

  /// \brief Constructs the bounds from the given box. The sphere radius is calculated from the box extends.
  xiiBoundingBoxSphereTemplate(const xiiBoundingBoxTemplate<Type>& box); // [tested]

  /// \brief Constructs the bounds from the given sphere. The box extends are calculated from the sphere radius.
  xiiBoundingBoxSphereTemplate(const xiiBoundingSphereTemplate<Type>& sphere); // [tested]

#if XII_ENABLED(XII_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    XII_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please check that "
                                "all code-paths properly initialize this object.");
  }
#endif

  /// \brief Resets the bounds to an invalid state.
  void SetInvalid(); // [tested]

  /// \brief Checks whether the bounds is in an invalid state.
  bool IsValid() const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Calculates the bounds from given set of points.
  void SetFromPoints(const xiiVec3Template<Type>* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride = sizeof(xiiVec3Template<Type>)); // [tested]

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

/// \brief Checks whether this bounds and the other are not identical.
template <typename Type>
bool operator!=(const xiiBoundingBoxSphereTemplate<Type>& lhs, const xiiBoundingBoxSphereTemplate<Type>& rhs); // [tested]


#include <Foundation/Math/Implementation/BoundingBoxSphere_inl.h>
