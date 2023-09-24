#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Vec2.h>

/// \brief A simple rectangle class templated on the type for x, y and width, height.
///
template <typename Type>
class xiiRectTemplate
{
public:
  // Means this object can be copied using memcpy instead of copy construction.
  XII_DECLARE_POD_TYPE();

  // *** Data ***
public:
  Type x;
  Type y;

  Type width;
  Type height;

  // *** Constructors ***
public:
  /// \brief Default constructor does not initialize the data.
  xiiRectTemplate();

  /// \brief Constructor to set all values.
  xiiRectTemplate(Type x, Type y, Type width, Type height);

  /// \brief Initializes x and y with zero, width and height with the given values.
  xiiRectTemplate(Type width, Type height);

  /// The smaller value along x.
  Type Left() const { return x; }

  /// The larger value along x.
  Type Right() const { return x + width; }

  /// The smaller value along y.
  Type Top() const { return y; }

  /// The larger value along y.
  Type Bottom() const { return y + height; }

  /// The smaller value along x. Same as Left().
  Type GetX1() const { return x; }

  /// The larger value along x. Same as Right().
  Type GetX2() const { return x + width; }

  /// The smaller value along y. Same as Top().
  Type GetY1() const { return y; }

  /// The larger value along y. Same as Bottom().
  Type GetY2() const { return y + height; }

   /// \brief Returns the center point of the rectangle.
  xiiVec2Template<Type> GetCenter() const { return xiiVec2Template<Type>(x + width / 2, y + height / 2); }

  /// \brief Returns the width and height as a vec2.
  xiiVec2Template<Type> GetExtents() const { return xiiVec2Template<Type>(width, height); }

  /// \brief Returns the half width and half height as a vec2.
  xiiVec2Template<Type> GetHalfExtents() const { return xiiVec2Template<Type>(width / 2, height / 2); }

  /// \brief Increases the size of the rect in all directions.
  void Grow(Type xy);

  // *** Common Functions ***
public:
  bool operator==(const xiiRectTemplate<Type>& rhs) const;

  bool operator!=(const xiiRectTemplate<Type>& rhs) const;

  /// \brief Sets the rect to invalid values.
  ///
  /// IsValid() will return false afterwards.
  /// It is possible to make an invalid rect valid using ExpandToInclude().
  void SetInvalid();

  /// \brief Checks whether the position and size contain valid values.
  bool IsValid() const;

  /// \brief Returns true if the area of the rectangle is non zero
  bool HasNonZeroArea() const;

  /// \brief Returns true if the rectangle contains the provided point
  bool Contains(const xiiVec2Template<Type>& vPoint) const;

  /// \brief Returns true if the rectangle overlaps the provided rectangle.
  /// Also returns true if the rectangles are contained within each other completely(no intersecting edges).
  bool Overlaps(const xiiRectTemplate<Type>& other) const;

  /// \brief Extends this rectangle so that the provided rectangle is completely contained within it.
  void ExpandToInclude(const xiiRectTemplate<Type>& other);

  /// \brief Extends this rectangle so that the provided point is contained within it.
  void ExpandToInclude(const xiiVec2Template<Type>& other);

  /// \brief Clips this rect so that it is fully inside the provided rectangle.
  void Clip(const xiiRectTemplate<Type>& clipRect);

  /// \brief The given point is clamped to the area of the rect, i.e. it will be either inside the rect or on its edge and it will have the closest
  /// possible distance to the original point.
  const xiiVec2Template<Type> GetClampedPoint(const xiiVec2Template<Type>& vPoint) const;

  void SetIntersection(const xiiRectTemplate<Type>& r0, const xiiRectTemplate<Type>& r1);

  void SetUnion(const xiiRectTemplate<Type>& r0, const xiiRectTemplate<Type>& r1);

  /// \brief Moves the rectangle
  void Translate(Type tX, Type tY);

  /// \brief Scales width and height, and moves the position as well.
  void Scale(Type sX, Type sY);
};

#include <Foundation/Math/Implementation/Rect_inl.h>

using xiiRectU64 = xiiRectTemplate<xiiUInt64>;
using xiiRectU32 = xiiRectTemplate<xiiUInt32>;
using xiiRectU16 = xiiRectTemplate<xiiUInt16>;

using xiiRectI64 = xiiRectTemplate<xiiInt64>;
using xiiRectI32 = xiiRectTemplate<xiiInt32>;
using xiiRectI16 = xiiRectTemplate<xiiInt16>;

using xiiRectFloat  = xiiRectTemplate<float>;
using xiiRectDouble = xiiRectTemplate<double>;
using xiiRectReal   = xiiRectTemplate<xiiReal>;
