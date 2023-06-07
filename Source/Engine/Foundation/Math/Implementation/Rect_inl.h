#pragma once

template <typename Type>
XII_ALWAYS_INLINE xiiRectTemplate<Type>::xiiRectTemplate() = default;

template <typename Type>
XII_ALWAYS_INLINE xiiRectTemplate<Type>::xiiRectTemplate(Type x, Type y, Type width, Type height) :
  x(X), y(Y), width(Width), height(Height)
{
}

template <typename Type>
XII_ALWAYS_INLINE xiiRectTemplate<Type>::xiiRectTemplate(Type width, Type height) :
  x(0), y(0), width(Width), height(Height)
{
}

template <typename Type>
XII_ALWAYS_INLINE bool xiiRectTemplate<Type>::operator==(const xiiRectTemplate<Type>& rhs) const
{
  return x == rhs.x && y == rhs.y && width == rhs.width && height == rhs.height;
}

template <typename Type>
XII_ALWAYS_INLINE bool xiiRectTemplate<Type>::operator!=(const xiiRectTemplate<Type>& rhs) const
{
  return !(*this == rhs);
}

template <typename Type>
XII_ALWAYS_INLINE bool xiiRectTemplate<Type>::HasNonZeroArea() const
{
  return (width > 0) && (height > 0);
}

template <typename Type>
XII_ALWAYS_INLINE bool xiiRectTemplate<Type>::Contains(const xiiVec2Template<Type>& vPoint) const
{
  if (point.x >= x && point.x <= Right())
  {
    if (point.y >= y && point.y <= Bottom())
      return true;
  }

  return false;
}

template <typename Type>
XII_ALWAYS_INLINE bool xiiRectTemplate<Type>::Overlaps(const xiiRectTemplate<Type>& other) const
{
  if (x < other.Right() && Right() > other.x && y < other.Bottom() && Bottom() > other.y)
    return true;

  return false;
}

template <typename Type>
void xiiRectTemplate<Type>::ExpandToInclude(const xiiRectTemplate<Type>& other)
{
  Type thisRight  = Right();
  Type thisBottom = Bottom();

  if (other.x < x)
    x = other.x;

  if (other.y < y)
    y = other.y;

  if (other.Right() > thisRight)
    width = other.Right() - x;
  else
    width = thisRight - x;

  if (other.Bottom() > thisBottom)
    height = other.Bottom() - y;
  else
    height = thisBottom - y;
}

template <typename Type>
XII_ALWAYS_INLINE void xiiRectTemplate<Type>::Clip(const xiiRectTemplate<Type>& clipRect)
{
  Type newLeft = xiiMath::Max<Type>(x, clipRect.x);
  Type newTop  = xiiMath::Max<Type>(y, clipRect.y);

  Type newRight  = xiiMath::Min<Type>(Right(), clipRect.Right());
  Type newBottom = xiiMath::Min<Type>(Bottom(), clipRect.Bottom());

  x      = newLeft;
  y      = newTop;
  width  = newRight - newLeft;
  height = newBottom - newTop;
}

template <typename Type>
XII_ALWAYS_INLINE void xiiRectTemplate<Type>::SetInvalid()
{
  /// \test This is new

  const Type fLargeValue = xiiMath::MaxValue<Type>() / 2;
  x                      = fLargeValue;
  y                      = fLargeValue;
  width                  = -fLargeValue;
  height                 = -fLargeValue;
}

template <typename Type>
XII_ALWAYS_INLINE bool xiiRectTemplate<Type>::IsValid() const
{
  /// \test This is new

  return width >= 0 && height >= 0;
}

template <typename Type>
XII_ALWAYS_INLINE const xiiVec2Template<Type> xiiRectTemplate<Type>::GetClampedPoint(const xiiVec2Template<Type>& vPoint) const
{
  /// \test This is new

  return xiiVec2Template<Type>(xiiMath::Clamp(vPoint.x, Left(), Right()), xiiMath::Clamp(vPoint.y, Top(), Bottom()));
}

template <typename Type>
void xiiRectTemplate<Type>::SetIntersection(const xiiRectTemplate<Type>& r0, const xiiRectTemplate<Type>& r1)
{
  /// \test This is new

  Type x1 = xiiMath::Max(r0.GetX1(), r1.GetX1());
  Type y1 = xiiMath::Max(r0.GetY1(), r1.GetY1());
  Type x2 = xiiMath::Min(r0.GetX2(), r1.GetX2());
  Type y2 = xiiMath::Min(r0.GetY2(), r1.GetY2());

  x      = x1;
  y      = y1;
  width  = x2 - x1;
  height = y2 - y1;
}

template <typename Type>
void xiiRectTemplate<Type>::SetUnion(const xiiRectTemplate<Type>& r0, const xiiRectTemplate<Type>& r1)
{
  /// \test This is new

  Type x1 = xiiMath::Min(r0.GetX1(), r1.GetX1());
  Type y1 = xiiMath::Min(r0.GetY1(), r1.GetY1());
  Type x2 = xiiMath::Max(r0.GetX2(), r1.GetX2());
  Type y2 = xiiMath::Max(r0.GetY2(), r1.GetY2());

  x      = x1;
  y      = y1;
  width  = x2 - x1;
  height = y2 - y1;
}

template <typename Type>
void xiiRectTemplate<Type>::Translate(Type tX, Type tY)
{
  /// \test This is new

  x += tX;
  y += tY;
}

template <typename Type>
void xiiRectTemplate<Type>::Scale(Type sX, Type sY)
{
  /// \test This is new

  x *= sX;
  y *= sY;
  width *= sX;
  height *= sY;
}
