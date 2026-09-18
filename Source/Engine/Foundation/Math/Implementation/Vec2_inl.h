/// Copyright (c) Theophilus Eriata. All Rights Reserved.

template <typename Type>
XII_ALWAYS_INLINE xiiVec2Template<Type>::xiiVec2Template()
{
#if XII_ENABLED(XII_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const Type TypeNaN = xiiMath::NaN<Type>();
  x                  = TypeNaN;
  y                  = TypeNaN;
#endif
}

template <typename Type>
XII_ALWAYS_INLINE xiiVec2Template<Type>::xiiVec2Template(Type x, Type y) :
  x(x), y(y)
{
}

template <typename Type>
XII_ALWAYS_INLINE xiiVec2Template<Type>::xiiVec2Template(Type v) :
  x(v), y(v)
{
}

template <typename Type>
XII_ALWAYS_INLINE void xiiVec2Template<Type>::Set(Type xy)
{
  x = xy;
  y = xy;
}

template <typename Type>
XII_ALWAYS_INLINE void xiiVec2Template<Type>::Set(Type inX, Type inY)
{
  x = inX;
  y = inY;
}

template <typename Type>
XII_ALWAYS_INLINE void xiiVec2Template<Type>::SetZero()
{
  x = y = 0;
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE XII_ALWAYS_INLINE Type xiiVec2Template<Type>::GetLength() const
{
  return (xiiMath::Sqrt(GetLengthSquared()));
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE xiiResult xiiVec2Template<Type>::SetLength(Type fNewLength, Type fEpsilon /* = xiiMath::DefaultEpsilon<Type>() */)
{
  if (NormalizeIfNotZero(xiiVec2Template<Type>::MakeZero(), fEpsilon) == XII_FAILURE)
    return XII_FAILURE;

  *this *= fNewLength;
  return XII_SUCCESS;
}

template <typename Type>
XII_ALWAYS_INLINE Type xiiVec2Template<Type>::GetLengthSquared() const
{
  return (x * x + y * y);
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE XII_FORCE_INLINE Type xiiVec2Template<Type>::GetLengthAndNormalize()
{
  const Type fLength = GetLength();
  *this /= fLength;
  return fLength;
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE XII_FORCE_INLINE const xiiVec2Template<Type> xiiVec2Template<Type>::GetNormalized() const
{
  const Type fLen = GetLength();

  const Type fLengthInv = xiiMath::Invert(fLen);
  return xiiVec2Template<Type>(x * fLengthInv, y * fLengthInv);
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE XII_ALWAYS_INLINE void xiiVec2Template<Type>::Normalize()
{
  *this /= GetLength();
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE inline xiiResult xiiVec2Template<Type>::NormalizeIfNotZero(const xiiVec2Template<Type>& vFallback, Type fEpsilon)
{
  XII_NAN_ASSERT(&vFallback);

  const Type fLength = GetLength();

  if (!xiiMath::IsFinite(fLength) || xiiMath::IsZero(fLength, fEpsilon))
  {
    *this = vFallback;
    return XII_FAILURE;
  }

  *this /= fLength;
  return XII_SUCCESS;
}

/*! \note Normalization, especially with SSE is not very precise. So this function checks whether the (squared)
  length is between a lower and upper limit.
*/
template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE inline bool xiiVec2Template<Type>::IsNormalized(Type fEpsilon /* = xiiMath::HugeEpsilon<Type>() */) const
{
  const Type t = GetLengthSquared();
  return xiiMath::IsEqual(t, (Type)(1), fEpsilon);
}

template <typename Type>
inline bool xiiVec2Template<Type>::IsZero() const
{
  return (x == 0 && y == 0);
}

template <typename Type>
inline bool xiiVec2Template<Type>::IsZero(Type fEpsilon) const
{
  XII_NAN_ASSERT(this);

  return (xiiMath::IsZero(x, fEpsilon) && xiiMath::IsZero(y, fEpsilon));
}

template <typename Type>
inline bool xiiVec2Template<Type>::IsNaN() const
{
  if (xiiMath::IsNaN(x))
    return true;
  if (xiiMath::IsNaN(y))
    return true;

  return false;
}

template <typename Type>
inline bool xiiVec2Template<Type>::IsValid() const
{
  if (!xiiMath::IsFinite(x))
    return false;
  if (!xiiMath::IsFinite(y))
    return false;

  return true;
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE XII_ALWAYS_INLINE Type xiiVec2Template<Type>::GetDistanceTo(const xiiVec2Template<Type>& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return (*this - rhs).GetLength();
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE XII_ALWAYS_INLINE Type xiiVec2Template<Type>::GetSquaredDistanceTo(const xiiVec2Template<Type>& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return (*this - rhs).GetLengthSquared();
}

template <typename Type>
XII_FORCE_INLINE const xiiVec2Template<Type> xiiVec2Template<Type>::operator-() const
{
  XII_NAN_ASSERT(this);

  return xiiVec2Template<Type>(-x, -y);
}

template <typename Type>
XII_FORCE_INLINE void xiiVec2Template<Type>::operator+=(const xiiVec2Template<Type>& rhs)
{
  x += rhs.x;
  y += rhs.y;

  XII_NAN_ASSERT(this);
}

template <typename Type>
XII_FORCE_INLINE void xiiVec2Template<Type>::operator-=(const xiiVec2Template<Type>& rhs)
{
  x -= rhs.x;
  y -= rhs.y;

  XII_NAN_ASSERT(this);
}

template <typename Type>
XII_FORCE_INLINE void xiiVec2Template<Type>::operator*=(Type f)
{
  x *= f;
  y *= f;

  XII_NAN_ASSERT(this);
}

template <typename Type>
XII_FORCE_INLINE void xiiVec2Template<Type>::operator/=(Type f)
{
  if constexpr (std::is_floating_point_v<Type>)
  {
    const Type fInverse = xiiMath::Invert(f);

    x *= fInverse;
    y *= fInverse;
  }
  else
  {
    x /= f;
    y /= f;
  }

  XII_NAN_ASSERT(this);
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE inline void xiiVec2Template<Type>::MakeOrthogonalTo(const xiiVec2Template<Type>& vNormal)
{
  XII_ASSERT_DEBUG(vNormal.IsNormalized(), "The normal must be normalized.");

  const Type fDot = this->Dot(vNormal);
  *this -= fDot * vNormal;
}

template <typename Type>
XII_FORCE_INLINE const xiiVec2Template<Type> xiiVec2Template<Type>::GetOrthogonalVector() const
{
  XII_NAN_ASSERT(this);
  XII_ASSERT_DEBUG(!IsZero(xiiMath::SmallEpsilon<Type>()), "The vector must not be zero to be able to compute an orthogonal vector.");

  return xiiVec2Template<Type>(-y, x);
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE inline const xiiVec2Template<Type> xiiVec2Template<Type>::GetReflectedVector(const xiiVec2Template<Type>& vNormal) const
{
  XII_ASSERT_DEBUG(vNormal.IsNormalized(), "vNormal must be normalized.");

  return ((*this) - (2 * this->Dot(vNormal) * vNormal));
}

template <typename Type>
XII_FORCE_INLINE Type xiiVec2Template<Type>::Dot(const xiiVec2Template<Type>& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return ((x * rhs.x) + (y * rhs.y));
}

template <typename Type>
inline xiiAngle xiiVec2Template<Type>::GetAngleBetween(const xiiVec2Template<Type>& rhs) const
{
  XII_ASSERT_DEBUG(this->IsNormalized(), "This vector must be normalized.");
  XII_ASSERT_DEBUG(rhs.IsNormalized(), "The other vector must be normalized.");

  return xiiMath::ACos(static_cast<float>(xiiMath::Clamp<Type>(this->Dot(rhs), (Type)-1, (Type)1)));
}

template <typename Type>
XII_FORCE_INLINE const xiiVec2Template<Type> xiiVec2Template<Type>::CompMin(const xiiVec2Template<Type>& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return xiiVec2Template<Type>(xiiMath::Min(x, rhs.x), xiiMath::Min(y, rhs.y));
}

template <typename Type>
XII_FORCE_INLINE const xiiVec2Template<Type> xiiVec2Template<Type>::CompMax(const xiiVec2Template<Type>& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return xiiVec2Template<Type>(xiiMath::Max(x, rhs.x), xiiMath::Max(y, rhs.y));
}

template <typename Type>
XII_FORCE_INLINE const xiiVec2Template<Type> xiiVec2Template<Type>::CompClamp(const xiiVec2Template<Type>& vLow, const xiiVec2Template<Type>& vHigh) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&vLow);
  XII_NAN_ASSERT(&vHigh);

  return xiiVec2Template<Type>(xiiMath::Clamp(x, vLow.x, vHigh.x), xiiMath::Clamp(y, vLow.y, vHigh.y));
}

template <typename Type>
XII_FORCE_INLINE const xiiVec2Template<Type> xiiVec2Template<Type>::CompMul(const xiiVec2Template<Type>& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return xiiVec2Template<Type>(x * rhs.x, y * rhs.y);
}

template <typename Type>
XII_FORCE_INLINE const xiiVec2Template<Type> xiiVec2Template<Type>::CompDiv(const xiiVec2Template<Type>& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return xiiVec2Template<Type>(x / rhs.x, y / rhs.y);
}

template <typename Type>
inline const xiiVec2Template<Type> xiiVec2Template<Type>::Abs() const
{
  XII_NAN_ASSERT(this);

  return xiiVec2Template<Type>(xiiMath::Abs(x), xiiMath::Abs(y));
}

template <typename Type>
XII_FORCE_INLINE const xiiVec2Template<Type> operator+(const xiiVec2Template<Type>& v1, const xiiVec2Template<Type>& v2)
{
  XII_NAN_ASSERT(&v1);
  XII_NAN_ASSERT(&v2);

  return xiiVec2Template<Type>(v1.x + v2.x, v1.y + v2.y);
}

template <typename Type>
XII_FORCE_INLINE const xiiVec2Template<Type> operator-(const xiiVec2Template<Type>& v1, const xiiVec2Template<Type>& v2)
{
  XII_NAN_ASSERT(&v1);
  XII_NAN_ASSERT(&v2);

  return xiiVec2Template<Type>(v1.x - v2.x, v1.y - v2.y);
}

template <typename Type>
XII_FORCE_INLINE const xiiVec2Template<Type> operator*(Type f, const xiiVec2Template<Type>& v)
{
  XII_NAN_ASSERT(&v);

  return xiiVec2Template<Type>(v.x * f, v.y * f);
}

template <typename Type>
XII_FORCE_INLINE const xiiVec2Template<Type> operator*(const xiiVec2Template<Type>& v, Type f)
{
  XII_NAN_ASSERT(&v);

  return xiiVec2Template<Type>(v.x * f, v.y * f);
}

template <typename Type>
XII_FORCE_INLINE const xiiVec2Template<Type> operator/(const xiiVec2Template<Type>& v, Type f)
{
  XII_NAN_ASSERT(&v);

  if constexpr (std::is_floating_point_v<Type>)
  {
    // Multiplication is much faster than division.
    const Type fInverse = xiiMath::Invert(f);

    return xiiVec2Template<Type>(v.x * fInverse, v.y * fInverse);
  }
  else
  {
    return xiiVec2Template<Type>(v.x / f, v.y / f);
  }
}

template <typename Type>
inline bool xiiVec2Template<Type>::IsIdentical(const xiiVec2Template<Type>& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return ((x == rhs.x) && (y == rhs.y));
}

template <typename Type>
inline bool xiiVec2Template<Type>::IsEqual(const xiiVec2Template<Type>& rhs, Type fEpsilon) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return (xiiMath::IsEqual(x, rhs.x, fEpsilon) && xiiMath::IsEqual(y, rhs.y, fEpsilon));
}

template <typename Type>
XII_FORCE_INLINE bool operator==(const xiiVec2Template<Type>& v1, const xiiVec2Template<Type>& v2)
{
  return v1.IsIdentical(v2);
}

template <typename Type>
XII_FORCE_INLINE bool operator<(const xiiVec2Template<Type>& v1, const xiiVec2Template<Type>& v2)
{
  XII_NAN_ASSERT(&v1);
  XII_NAN_ASSERT(&v2);

  if (v1.x < v2.x)
    return true;
  if (v1.x > v2.x)
    return false;

  return (v1.y < v2.y);
}
