/// Copyright (c) Theophilus Eriata. All Rights Reserved.

template <typename Type>
XII_FORCE_INLINE xiiVec3Template<Type>::xiiVec3Template()
{
#if XII_ENABLED(XII_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const Type TypeNaN = xiiMath::NaN<Type>();
  x                  = TypeNaN;
  y                  = TypeNaN;
  z                  = TypeNaN;
#endif
}

template <typename Type>
XII_ALWAYS_INLINE xiiVec3Template<Type>::xiiVec3Template(Type x, Type y, Type z) :
  x(x), y(y), z(z)
{
}

template <typename Type>
XII_ALWAYS_INLINE xiiVec3Template<Type>::xiiVec3Template(Type v) :
  x(v), y(v), z(v)
{
}

template <typename Type>
XII_ALWAYS_INLINE void xiiVec3Template<Type>::Set(Type xyz)
{
  x = xyz;
  y = xyz;
  z = xyz;
}

template <typename Type>
XII_ALWAYS_INLINE void xiiVec3Template<Type>::Set(Type inX, Type inY, Type inZ)
{
  x = inX;
  y = inY;
  z = inZ;
}

template <typename Type>
XII_ALWAYS_INLINE void xiiVec3Template<Type>::SetZero()
{
  x = y = z = 0;
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE XII_ALWAYS_INLINE Type xiiVec3Template<Type>::GetLength() const
{
  return (xiiMath::Sqrt(GetLengthSquared()));
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE xiiResult xiiVec3Template<Type>::SetLength(Type fNewLength, Type fEpsilon /* = xiiMath::DefaultEpsilon<Type>() */)
{
  if (NormalizeIfNotZero(xiiVec3Template<Type>::MakeZero(), fEpsilon) == XII_FAILURE)
    return XII_FAILURE;

  *this *= fNewLength;
  return XII_SUCCESS;
}

template <typename Type>
XII_FORCE_INLINE Type xiiVec3Template<Type>::GetLengthSquared() const
{
  XII_NAN_ASSERT(this);

  return (x * x + y * y + z * z);
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE XII_FORCE_INLINE Type xiiVec3Template<Type>::GetLengthAndNormalize()
{
  const Type fLength = GetLength();
  *this /= fLength;
  return fLength;
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE XII_FORCE_INLINE const xiiVec3Template<Type> xiiVec3Template<Type>::GetNormalized() const
{
  const Type fLen = GetLength();

  const Type fLengthInv = xiiMath::Invert(fLen);
  return xiiVec3Template<Type>(x * fLengthInv, y * fLengthInv, z * fLengthInv);
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE XII_ALWAYS_INLINE void xiiVec3Template<Type>::Normalize()
{
  *this /= GetLength();
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE xiiResult xiiVec3Template<Type>::NormalizeIfNotZero(const xiiVec3Template<Type>& vFallback, Type fEpsilon)
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
XII_IMPLEMENT_IF_FLOAT_TYPE XII_FORCE_INLINE bool xiiVec3Template<Type>::IsNormalized(Type fEpsilon /* = xiiMath::HugeEpsilon<Type>() */) const
{
  const Type t = GetLengthSquared();
  return xiiMath::IsEqual(t, (Type)1, fEpsilon);
}

template <typename Type>
XII_FORCE_INLINE bool xiiVec3Template<Type>::IsZero() const
{
  XII_NAN_ASSERT(this);

  return ((x == 0.0f) && (y == 0.0f) && (z == 0.0f));
}

template <typename Type>
bool xiiVec3Template<Type>::IsZero(Type fEpsilon) const
{
  XII_NAN_ASSERT(this);

  return (xiiMath::IsZero(x, fEpsilon) && xiiMath::IsZero(y, fEpsilon) && xiiMath::IsZero(z, fEpsilon));
}

template <typename Type>
bool xiiVec3Template<Type>::IsNaN() const
{
  if (xiiMath::IsNaN(x))
    return true;
  if (xiiMath::IsNaN(y))
    return true;
  if (xiiMath::IsNaN(z))
    return true;

  return false;
}

template <typename Type>
bool xiiVec3Template<Type>::IsValid() const
{
  if (!xiiMath::IsFinite(x))
    return false;
  if (!xiiMath::IsFinite(y))
    return false;
  if (!xiiMath::IsFinite(z))
    return false;

  return true;
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE XII_ALWAYS_INLINE Type xiiVec3Template<Type>::GetDistanceTo(const xiiVec3Template<Type>& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return (*this - rhs).GetLength();
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE XII_ALWAYS_INLINE Type xiiVec3Template<Type>::GetSquaredDistanceTo(const xiiVec3Template<Type>& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return (*this - rhs).GetLengthSquared();
}

template <typename Type>
XII_FORCE_INLINE const xiiVec3Template<Type> xiiVec3Template<Type>::operator-() const
{
  XII_NAN_ASSERT(this);

  return xiiVec3Template<Type>(-x, -y, -z);
}

template <typename Type>
XII_FORCE_INLINE void xiiVec3Template<Type>::operator+=(const xiiVec3Template<Type>& rhs)
{
  x += rhs.x;
  y += rhs.y;
  z += rhs.z;

  XII_NAN_ASSERT(this);
}

template <typename Type>
XII_FORCE_INLINE void xiiVec3Template<Type>::operator-=(const xiiVec3Template<Type>& rhs)
{
  x -= rhs.x;
  y -= rhs.y;
  z -= rhs.z;

  XII_NAN_ASSERT(this);
}

template <typename Type>
XII_FORCE_INLINE void xiiVec3Template<Type>::operator*=(const xiiVec3Template& rhs)
{
  /// \test this is new

  x *= rhs.x;
  y *= rhs.y;
  z *= rhs.z;

  XII_NAN_ASSERT(this);
}

template <typename Type>
XII_FORCE_INLINE void xiiVec3Template<Type>::operator/=(const xiiVec3Template& rhs)
{
  /// \test this is new

  x /= rhs.x;
  y /= rhs.y;
  z /= rhs.z;

  XII_NAN_ASSERT(this);
}

template <typename Type>
XII_FORCE_INLINE void xiiVec3Template<Type>::operator*=(Type f)
{
  x *= f;
  y *= f;
  z *= f;

  XII_NAN_ASSERT(this);
}

template <typename Type>
XII_FORCE_INLINE void xiiVec3Template<Type>::operator/=(Type f)
{
  if constexpr (std::is_floating_point_v<Type>)
  {
    const Type fInverse = xiiMath::Invert(f);

    x *= fInverse;
    y *= fInverse;
    z *= fInverse;
  }
  else
  {
    x /= f;
    y /= f;
    z /= f;
  }

  // if this assert fires, you might have tried to normalize a zero-length vector
  XII_NAN_ASSERT(this);
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE xiiResult xiiVec3Template<Type>::CalculateNormal(const xiiVec3Template<Type>& v1, const xiiVec3Template<Type>& v2, const xiiVec3Template<Type>& v3)
{
  *this = (v3 - v2).CrossRH(v1 - v2);
  return NormalizeIfNotZero();
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE void xiiVec3Template<Type>::MakeOrthogonalTo(const xiiVec3Template<Type>& vNormal)
{
  XII_ASSERT_DEBUG(vNormal.IsNormalized(), "The vector to make this vector orthogonal to, must be normalized. It's length is {0}", xiiArgF(vNormal.GetLength(), 3));

  xiiVec3Template<Type> vOrtho = vNormal.CrossRH(*this);
  *this                        = vOrtho.CrossRH(vNormal);
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE const xiiVec3Template<Type> xiiVec3Template<Type>::GetOrthogonalVector() const
{
  XII_ASSERT_DEBUG(!IsZero(xiiMath::SmallEpsilon<Type>()), "The vector must not be zero to be able to compute an orthogonal vector.");

  Type fDot = xiiMath::Abs(this->Dot(xiiVec3Template<Type>(0, 1, 0)));
  if (fDot < 0.999f)
    return this->CrossRH(xiiVec3Template<Type>(0, 1, 0));

  return this->CrossRH(xiiVec3Template<Type>(1, 0, 0));
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE const xiiVec3Template<Type> xiiVec3Template<Type>::GetReflectedVector(const xiiVec3Template<Type>& vNormal) const
{
  XII_ASSERT_DEBUG(vNormal.IsNormalized(), "vNormal must be normalized.");

  return ((*this) - ((Type)2 * this->Dot(vNormal) * vNormal));
}

template <typename Type>
XII_FORCE_INLINE Type xiiVec3Template<Type>::Dot(const xiiVec3Template<Type>& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return ((x * rhs.x) + (y * rhs.y) + (z * rhs.z));
}

template <typename Type>
const xiiVec3Template<Type> xiiVec3Template<Type>::CrossRH(const xiiVec3Template<Type>& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return xiiVec3Template<Type>(y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x);
}

template <typename Type>
xiiAngle xiiVec3Template<Type>::GetAngleBetween(const xiiVec3Template<Type>& rhs) const
{
  XII_ASSERT_DEBUG(this->IsNormalized(), "This vector must be normalized.");
  XII_ASSERT_DEBUG(rhs.IsNormalized(), "The other vector must be normalized.");

  return xiiMath::ACos(static_cast<float>(xiiMath::Clamp(this->Dot(rhs), (Type)-1, (Type)1)));
}

template <typename Type>
XII_FORCE_INLINE const xiiVec3Template<Type> xiiVec3Template<Type>::CompMin(const xiiVec3Template<Type>& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return xiiVec3Template<Type>(xiiMath::Min(x, rhs.x), xiiMath::Min(y, rhs.y), xiiMath::Min(z, rhs.z));
}

template <typename Type>
XII_FORCE_INLINE const xiiVec3Template<Type> xiiVec3Template<Type>::CompMax(const xiiVec3Template<Type>& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return xiiVec3Template<Type>(xiiMath::Max(x, rhs.x), xiiMath::Max(y, rhs.y), xiiMath::Max(z, rhs.z));
}

template <typename Type>
XII_FORCE_INLINE const xiiVec3Template<Type> xiiVec3Template<Type>::CompClamp(const xiiVec3Template& vLow, const xiiVec3Template& vHigh) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&vLow);
  XII_NAN_ASSERT(&vHigh);

  return xiiVec3Template<Type>(xiiMath::Clamp(x, vLow.x, vHigh.x), xiiMath::Clamp(y, vLow.y, vHigh.y), xiiMath::Clamp(z, vLow.z, vHigh.z));
}

template <typename Type>
XII_FORCE_INLINE const xiiVec3Template<Type> xiiVec3Template<Type>::CompMul(const xiiVec3Template<Type>& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return xiiVec3Template<Type>(x * rhs.x, y * rhs.y, z * rhs.z);
}

template <typename Type>
XII_FORCE_INLINE const xiiVec3Template<Type> xiiVec3Template<Type>::CompDiv(const xiiVec3Template<Type>& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return xiiVec3Template<Type>(x / rhs.x, y / rhs.y, z / rhs.z);
}

template <typename Type>
inline const xiiVec3Template<Type> xiiVec3Template<Type>::Abs() const
{
  XII_NAN_ASSERT(this);

  return xiiVec3Template<Type>(xiiMath::Abs(x), xiiMath::Abs(y), xiiMath::Abs(z));
}

template <typename Type>
XII_FORCE_INLINE const xiiVec3Template<Type> operator+(const xiiVec3Template<Type>& v1, const xiiVec3Template<Type>& v2)
{
  XII_NAN_ASSERT(&v1);
  XII_NAN_ASSERT(&v2);

  return xiiVec3Template<Type>(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

template <typename Type>
XII_FORCE_INLINE const xiiVec3Template<Type> operator-(const xiiVec3Template<Type>& v1, const xiiVec3Template<Type>& v2)
{
  XII_NAN_ASSERT(&v1);
  XII_NAN_ASSERT(&v2);

  return xiiVec3Template<Type>(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

template <typename Type>
XII_FORCE_INLINE const xiiVec3Template<Type> operator*(Type f, const xiiVec3Template<Type>& v)
{
  XII_NAN_ASSERT(&v);

  return xiiVec3Template<Type>(v.x * f, v.y * f, v.z * f);
}

template <typename Type>
XII_FORCE_INLINE const xiiVec3Template<Type> operator*(const xiiVec3Template<Type>& v, Type f)
{
  XII_NAN_ASSERT(&v);

  return xiiVec3Template<Type>(v.x * f, v.y * f, v.z * f);
}

template <typename Type>
XII_FORCE_INLINE const xiiVec3Template<Type> operator/(const xiiVec3Template<Type>& v, Type f)
{
  XII_NAN_ASSERT(&v);

  if constexpr (std::is_floating_point_v<Type>)
  {
    // Multiplication is much faster than division.
    const Type fInverse = xiiMath::Invert(f);
    return xiiVec3Template<Type>(v.x * fInverse, v.y * fInverse, v.z * fInverse);
  }
  else
  {
    return xiiVec3Template<Type>(v.x / f, v.y / f, v.z / f);
  }
}

template <typename Type>
bool xiiVec3Template<Type>::IsIdentical(const xiiVec3Template<Type>& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return ((x == rhs.x) && (y == rhs.y) && (z == rhs.z));
}

template <typename Type>
bool xiiVec3Template<Type>::IsEqual(const xiiVec3Template<Type>& rhs, Type fEpsilon) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return (xiiMath::IsEqual(x, rhs.x, fEpsilon) && xiiMath::IsEqual(y, rhs.y, fEpsilon) && xiiMath::IsEqual(z, rhs.z, fEpsilon));
}

template <typename Type>
XII_ALWAYS_INLINE bool operator==(const xiiVec3Template<Type>& v1, const xiiVec3Template<Type>& v2)
{
  return v1.IsIdentical(v2);
}

template <typename Type>
XII_FORCE_INLINE bool operator<(const xiiVec3Template<Type>& v1, const xiiVec3Template<Type>& v2)
{
  XII_NAN_ASSERT(&v1);
  XII_NAN_ASSERT(&v2);

  if (v1.x < v2.x)
    return true;
  if (v1.x > v2.x)
    return false;
  if (v1.y < v2.y)
    return true;
  if (v1.y > v2.y)
    return false;

  return (v1.z < v2.z);
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE const xiiVec3Template<Type> xiiVec3Template<Type>::GetRefractedVector(const xiiVec3Template<Type>& vNormal, Type fRefIndex1, Type fRefIndex2) const
{
  XII_ASSERT_DEBUG(vNormal.IsNormalized(), "vNormal must be normalized.");

  const Type n     = fRefIndex1 / fRefIndex2;
  const Type cosI  = this->Dot(vNormal);
  const Type sinT2 = n * n * (1.0f - (cosI * cosI));

  // invalid refraction
  if (sinT2 > 1.0f)
    return (*this);

  return ((n * (*this)) - (n + xiiMath::Sqrt(1.0f - sinT2)) * vNormal);
}
