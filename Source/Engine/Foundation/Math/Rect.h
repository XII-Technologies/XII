/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Vec2.h>

/// A simple rectangle class templated on the type for x, y and width, height.
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
  /// Default constructor does not initialize the data.
  xiiRectTemplate();

  /// Constructor to set all values.
  xiiRectTemplate(Type x, Type y, Type width, Type height);

  /// Initializes x and y with zero, width and height with the given values.
  xiiRectTemplate(Type width, Type height);

  /// Initializes x and y from pos, width and height from vSize.
  xiiRectTemplate(const xiiVec2Template<Type>& vTopLeftPosition, const xiiVec2Template<Type>& vSize);

  /// Creates an 'invalid' rect.
  ///
  /// IsValid() will return false.
  /// It is possible to make an invalid rect valid using ExpandToInclude().
  [[nodiscard]] static xiiRectTemplate<Type> MakeInvalid();

  /// Creates a rect that is located at the origin and has zero size. This is a 'valid' rect.
  [[nodiscard]] static xiiRectTemplate<Type> MakeZero();

  /// Creates a rect that is the intersection of the two provided rects.
  ///
  /// If the two rects don't overlap, the result will be a valid rect, but have zero area.
  /// See IsValid() and HasNonZeroArea().
  [[nodiscard]] static xiiRectTemplate<Type> MakeIntersection(const xiiRectTemplate<Type>& r0, const xiiRectTemplate<Type>& r1);

  /// Creates a rect that is the union of the two provided rects.
  ///
  /// This is the same as constructing a bounding box around the two rects.
  [[nodiscard]] static xiiRectTemplate<Type> MakeUnion(const xiiRectTemplate<Type>& r0, const xiiRectTemplate<Type>& r1);

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

  /// Returns the minimum corner position. Same as GetTopLeft().
  xiiVec2Template<Type> GetMinCorner() const { return xiiVec2Template<Type>(x, y); }

  /// Returns the maximum corner position. Same as GetBottomRight().
  xiiVec2Template<Type> GetMaxCorner() const { return xiiVec2Template<Type>(x + width, y + height); }

  /// Returns the top left corner. Same as GetMinCorner().
  xiiVec2Template<Type> GetTopLeft() const { return xiiVec2Template<Type>(x, y); }

  /// Returns the top right corner.
  xiiVec2Template<Type> GetTopRight() const { return xiiVec2Template<Type>(x + width, y); }

  /// Returns the bottom left corner.
  xiiVec2Template<Type> GetBottomLeft() const { return xiiVec2Template<Type>(x, y + height); }

  /// Returns the bottom right corner. Same as GetMaxCorner().
  xiiVec2Template<Type> GetBottomRight() const { return xiiVec2Template<Type>(x + width, y + height); }

  /// Returns the center point of the rectangle.
  xiiVec2Template<Type> GetCenter() const { return xiiVec2Template<Type>(x + width / 2, y + height / 2); }

  /// Returns the width and height as a vec2.
  xiiVec2Template<Type> GetExtents() const { return xiiVec2Template<Type>(width, height); }

  /// Returns the half width and half height as a vec2.
  xiiVec2Template<Type> GetHalfExtents() const { return xiiVec2Template<Type>(width / 2, height / 2); }

  /// Increases the size of the rect in all directions.
  void Grow(Type xy);

  // *** Common Functions ***
public:
  [[nodiscard]] bool operator==(const xiiRectTemplate<Type>& rhs) const;

  /// Checks whether the position and size contain valid values.
  [[nodiscard]] bool IsValid() const;

  /// Returns true if the area of the rectangle is non zero
  [[nodiscard]] bool HasNonZeroArea() const;

  /// Returns true if the rectangle contains the provided point
  [[nodiscard]] bool Contains(const xiiVec2Template<Type>& vPoint) const;

  /// Returns true if the rectangle contains the provided rectangle completely (no intersecting edges).
  [[nodiscard]] bool Contains(const xiiRectTemplate<Type>& r) const;

  /// Returns true if the rectangle overlaps the provided rectangle.
  /// Also returns true if the rectangles are contained within each other completely (no intersecting edges).
  [[nodiscard]] bool Overlaps(const xiiRectTemplate<Type>& other) const;

  /// Extends this rectangle so that the provided rectangle is completely contained within it.
  void ExpandToInclude(const xiiRectTemplate<Type>& other);

  /// Extends this rectangle so that the provided point is contained within it.
  void ExpandToInclude(const xiiVec2Template<Type>& other);

  /// Clips this rect so that it is fully inside the provided rectangle.
  void Clip(const xiiRectTemplate<Type>& clipRect);

  /// The given point is clamped to the area of the rect, i.e. it will be either inside the rect or on its edge and it will have the closest
  /// possible distance to the original point.
  [[nodiscard]] const xiiVec2Template<Type> GetClampedPoint(const xiiVec2Template<Type>& vPoint) const;

  /// Clamps the given rect to the area of this rect and returns it.
  ///
  /// If the input rect is entirely outside this rect, the result will be reduced to a point or a line closest to the input rect.
  [[nodiscard]] const xiiRectTemplate<Type> GetClampedRect(const xiiRectTemplate<Type>& r) const
  {
    const xiiVec2Template<Type> vNewMin = GetClampedPoint(r.GetMinCorner());
    const xiiVec2Template<Type> vNewMax = GetClampedPoint(r.GetMaxCorner());
    return xiiRectTemplate<Type>(vNewMin, vNewMax - vNewMin);
  }

  /// Sets the center of the rectangle.
  void SetCenter(Type tX, Type tY);

  /// Moves the rectangle.
  void Translate(Type tX, Type tY);

  /// Scales width and height, and moves the position as well.
  void Scale(Type sX, Type sY);
};

#include <Foundation/Math/Implementation/Rect_inl.h>

using xiiRectU64 = xiiRectTemplate<xiiUInt64>;
using xiiRectU32 = xiiRectTemplate<xiiUInt32>;
using xiiRectU16 = xiiRectTemplate<xiiUInt16>;
using xiiRectU8  = xiiRectTemplate<xiiUInt8>;

using xiiRectI64 = xiiRectTemplate<xiiInt64>;
using xiiRectI32 = xiiRectTemplate<xiiInt32>;
using xiiRectI16 = xiiRectTemplate<xiiInt16>;
using xiiRectI8  = xiiRectTemplate<xiiInt8>;

using xiiRectFloat  = xiiRectTemplate<float>;
using xiiRectDouble = xiiRectTemplate<double>;
using xiiRectReal   = xiiRectTemplate<xiiReal>;
