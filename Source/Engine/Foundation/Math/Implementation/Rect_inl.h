#pragma once

template <typename Type>
XII_ALWAYS_INLINE xiiRectTemplate<Type>::xiiRectTemplate() = default;

template <typename Type>
XII_ALWAYS_INLINE xiiRectTemplate<Type>::xiiRectTemplate(Type x, Type y, Type width, Type height) :
  x(x), y(y), width(width), height(height)
{
}

template <typename Type>
XII_ALWAYS_INLINE xiiRectTemplate<Type>::xiiRectTemplate(Type width, Type height) :
  x(0), y(0), width(width), height(height)
{
}

template <typename Type>
XII_ALWAYS_INLINE xiiRectTemplate<Type>::xiiRectTemplate(const xiiVec2Template<Type>& vTopLeftPosition, const xiiVec2Template<Type>& vSize)
{
  x      = vTopLeftPosition.x;
  y      = vTopLeftPosition.y;
  width  = vSize.x;
  height = vSize.y;
}

template <typename Type>
xiiRectTemplate<Type> xiiRectTemplate<Type>::MakeInvalid()
{
  /// \test This is new

  xiiRectTemplate<Type> res;

  const Type fLargeValue = xiiMath::MaxValue<Type>() / 2;
  res.x                  = fLargeValue;
  res.y                  = fLargeValue;
  res.width              = -fLargeValue;
  res.height             = -fLargeValue;

  return res;
}

template <typename Type>
xiiRectTemplate<Type> xiiRectTemplate<Type>::MakeIntersection(const xiiRectTemplate<Type>& r0, const xiiRectTemplate<Type>& r1)
{
  /// \test This is new

  xiiRectTemplate<Type> res;

  Type x1 = xiiMath::Max(r0.GetX1(), r1.GetX1());
  Type y1 = xiiMath::Max(r0.GetY1(), r1.GetY1());
  Type x2 = xiiMath::Min(r0.GetX2(), r1.GetX2());
  Type y2 = xiiMath::Min(r0.GetY2(), r1.GetY2());

  res.x      = x1;
  res.y      = y1;
  res.width  = x2 - x1;
  res.height = y2 - y1;

  return res;
}

template <typename Type>
xiiRectTemplate<Type> xiiRectTemplate<Type>::MakeUnion(const xiiRectTemplate<Type>& r0, const xiiRectTemplate<Type>& r1)
{
  /// \test This is new

  xiiRectTemplate<Type> res;

  Type x1 = xiiMath::Min(r0.GetX1(), r1.GetX1());
  Type y1 = xiiMath::Min(r0.GetY1(), r1.GetY1());
  Type x2 = xiiMath::Max(r0.GetX2(), r1.GetX2());
  Type y2 = xiiMath::Max(r0.GetY2(), r1.GetY2());

  res.x      = x1;
  res.y      = y1;
  res.width  = x2 - x1;
  res.height = y2 - y1;

  return res;
}

template <typename Type>
XII_ALWAYS_INLINE bool xiiRectTemplate<Type>::operator==(const xiiRectTemplate<Type>& rhs) const
{
  return x == rhs.x && y == rhs.y && width == rhs.width && height == rhs.height;
}

template <typename Type>
XII_ALWAYS_INLINE bool xiiRectTemplate<Type>::HasNonZeroArea() const
{
  return (width > 0) && (height > 0);
}

template <typename Type>
XII_ALWAYS_INLINE bool xiiRectTemplate<Type>::Contains(const xiiVec2Template<Type>& vPoint) const
{
  if (vPoint.x >= x && vPoint.x <= Right())
  {
    if (vPoint.y >= y && vPoint.y <= Bottom())
      return true;
  }

  return false;
}

template <typename Type>
XII_ALWAYS_INLINE bool xiiRectTemplate<Type>::Contains(const xiiRectTemplate<Type>& r) const
{
  return r.x >= x && r.y >= y && r.Right() <= Right() && r.Bottom() <= Bottom();
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
void xiiRectTemplate<Type>::ExpandToInclude(const xiiVec2Template<Type>& other)
{
  Type thisRight  = Right();
  Type thisBottom = Bottom();

  if (other.x < x)
    x = other.x;

  if (other.y < y)
    y = other.y;

  if (other.x > thisRight)
    width = other.x - x;
  else
    width = thisRight - x;

  if (other.y > thisBottom)
    height = other.y - y;
  else
    height = thisBottom - y;
}

template <typename Type>
void xiiRectTemplate<Type>::Grow(Type xy)
{
  x -= xy;
  y -= xy;
  width += xy * 2;
  height += xy * 2;
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
void xiiRectTemplate<Type>::SetCenter(Type tX, Type tY)
{
  /// \test This is new

  x = tX - width / 2;
  y = tY - height / 2;
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
