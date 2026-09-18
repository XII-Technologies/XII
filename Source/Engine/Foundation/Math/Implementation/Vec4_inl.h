/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>

// *** Vec2 and Vec3 Code ***
// Cannot put this into the Vec3_inl.h file, that would result in circular dependencies

template <typename Type>
XII_FORCE_INLINE const xiiVec3Template<Type> xiiVec2Template<Type>::GetAsVec3(Type z) const
{
  XII_NAN_ASSERT(this);

  return xiiVec3Template<Type>(x, y, z);
}

template <typename Type>
XII_FORCE_INLINE const xiiVec4Template<Type> xiiVec2Template<Type>::GetAsVec4(Type z, Type w) const
{
  XII_NAN_ASSERT(this);

  return xiiVec4Template<Type>(x, y, z, w);
}

template <typename Type>
XII_FORCE_INLINE const xiiVec2Template<Type> xiiVec3Template<Type>::GetAsVec2() const
{
  // Do not assert here, as the 3rd and 4th component may be NaN when this is fine, e.g. during interop with the SIMD classes
  // XII_NAN_ASSERT(this);

  return xiiVec2Template<Type>(x, y);
}

template <typename Type>
XII_FORCE_INLINE const xiiVec4Template<Type> xiiVec3Template<Type>::GetAsVec4(Type w) const
{
  XII_NAN_ASSERT(this);

  return xiiVec4Template<Type>(x, y, z, w);
}

template <typename Type>
XII_FORCE_INLINE const xiiVec4Template<Type> xiiVec3Template<Type>::GetAsPositionVec4() const
{
  // Do not assert here, as the 4th component may be NaN when this is fine, e.g. during interop with the SIMD classes
  // XII_NAN_ASSERT(this);

  return xiiVec4Template<Type>(x, y, z, 1);
}

template <typename Type>
XII_FORCE_INLINE const xiiVec4Template<Type> xiiVec3Template<Type>::GetAsDirectionVec4() const
{
  // Do not assert here, as the 4th component may be NaN when this is fine, e.g. during interop with the SIMD classes
  // XII_NAN_ASSERT(this);

  return xiiVec4Template<Type>(x, y, z, 0);
}

// *****************

template <typename Type>
XII_ALWAYS_INLINE xiiVec4Template<Type>::xiiVec4Template()
{
#if XII_ENABLED(XII_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const Type TypeNaN = xiiMath::NaN<Type>();
  x                  = TypeNaN;
  y                  = TypeNaN;
  z                  = 0;
  w                  = 0;
#endif
}

template <typename Type>
XII_ALWAYS_INLINE xiiVec4Template<Type>::xiiVec4Template(Type x, Type y, Type z, Type w) :
  x(x), y(y), z(z), w(w)
{
}

template <typename Type>
XII_ALWAYS_INLINE xiiVec4Template<Type>::xiiVec4Template(xiiVec3Template<Type> vXyz, Type w) :
  x(vXyz.x), y(vXyz.y), z(vXyz.z), w(w)
{
}

template <typename Type>
XII_ALWAYS_INLINE xiiVec4Template<Type>::xiiVec4Template(Type v) :
  x(v), y(v), z(v), w(v)
{
}

template <typename Type>
XII_FORCE_INLINE const xiiVec2Template<Type> xiiVec4Template<Type>::GetAsVec2() const
{
  // Do not assert here, as the 4th component may be NaN when this is fine, e.g. during interop with the SIMD classes.
  // XII_NAN_ASSERT(this);

  return xiiVec2Template<Type>(x, y);
}

template <typename Type>
XII_FORCE_INLINE const xiiVec3Template<Type> xiiVec4Template<Type>::GetAsVec3() const
{
  // Do not assert here, as the 4th component may be NaN when this is fine, e.g. during interop with the SIMD classes.
  // XII_NAN_ASSERT(this);

  return xiiVec3Template<Type>(x, y, z);
}

template <typename Type>
XII_ALWAYS_INLINE void xiiVec4Template<Type>::Set(Type xyzw)
{
  x = xyzw;
  y = xyzw;
  z = xyzw;
  w = xyzw;
}

template <typename Type>
XII_ALWAYS_INLINE void xiiVec4Template<Type>::Set(Type inX, Type inY, Type inZ, Type inW)
{
  x = inX;
  y = inY;
  z = inZ;
  w = inW;
}

template <typename Type>
inline void xiiVec4Template<Type>::SetZero()
{
  x = y = z = w = 0;
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE XII_ALWAYS_INLINE Type xiiVec4Template<Type>::GetLength() const
{
  return (xiiMath::Sqrt(GetLengthSquared()));
}

template <typename Type>
XII_FORCE_INLINE Type xiiVec4Template<Type>::GetLengthSquared() const
{
  XII_NAN_ASSERT(this);

  return (x * x + y * y + z * z + w * w);
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE XII_FORCE_INLINE Type xiiVec4Template<Type>::GetLengthAndNormalize()
{
  const Type fLength = GetLength();
  *this /= fLength;
  return fLength;
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE XII_FORCE_INLINE const xiiVec4Template<Type> xiiVec4Template<Type>::GetNormalized() const
{
  const Type fLen = GetLength();

  const Type fLengthInv = xiiMath::Invert(fLen);
  return xiiVec4Template<Type>(x * fLengthInv, y * fLengthInv, z * fLengthInv, w * fLengthInv);
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE XII_ALWAYS_INLINE void xiiVec4Template<Type>::Normalize()
{
  *this /= GetLength();
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE inline xiiResult xiiVec4Template<Type>::NormalizeIfNotZero(const xiiVec4Template<Type>& vFallback, Type fEpsilon)
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
XII_IMPLEMENT_IF_FLOAT_TYPE inline bool xiiVec4Template<Type>::IsNormalized(Type fEpsilon /* = xiiMath::HugeEpsilon<Type>() */) const
{
  const Type t = GetLengthSquared();
  return xiiMath::IsEqual(t, (Type)1, fEpsilon);
}

template <typename Type>
inline bool xiiVec4Template<Type>::IsZero() const
{
  XII_NAN_ASSERT(this);

  return ((x == 0.0f) && (y == 0.0f) && (z == 0.0f) && (w == 0.0f));
}

template <typename Type>
inline bool xiiVec4Template<Type>::IsZero(Type fEpsilon) const
{
  XII_NAN_ASSERT(this);

  return (xiiMath::IsZero(x, fEpsilon) && xiiMath::IsZero(y, fEpsilon) && xiiMath::IsZero(z, fEpsilon) && xiiMath::IsZero(w, fEpsilon));
}

template <typename Type>
inline bool xiiVec4Template<Type>::IsNaN() const
{
  if (xiiMath::IsNaN(x))
    return true;
  if (xiiMath::IsNaN(y))
    return true;
  if (xiiMath::IsNaN(z))
    return true;
  if (xiiMath::IsNaN(w))
    return true;

  return false;
}

template <typename Type>
inline bool xiiVec4Template<Type>::IsValid() const
{
  if (!xiiMath::IsFinite(x))
    return false;
  if (!xiiMath::IsFinite(y))
    return false;
  if (!xiiMath::IsFinite(z))
    return false;
  if (!xiiMath::IsFinite(w))
    return false;

  return true;
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE XII_ALWAYS_INLINE Type xiiVec4Template<Type>::GetDistanceTo(const xiiVec4Template <Type>& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return (*this - rhs).GetLength();
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE XII_ALWAYS_INLINE Type xiiVec4Template<Type>::GetSquaredDistanceTo(const xiiVec4Template <Type>& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return (*this - rhs).GetLengthSquared();
}

template <typename Type>
XII_FORCE_INLINE const xiiVec4Template<Type> xiiVec4Template<Type>::operator-() const
{
  XII_NAN_ASSERT(this);

  return xiiVec4Template<Type>(-x, -y, -z, -w);
}

template <typename Type>
XII_FORCE_INLINE void xiiVec4Template<Type>::operator+=(const xiiVec4Template<Type>& vCc)
{
  x += vCc.x;
  y += vCc.y;
  z += vCc.z;
  w += vCc.w;

  XII_NAN_ASSERT(this);
}

template <typename Type>
XII_FORCE_INLINE void xiiVec4Template<Type>::operator-=(const xiiVec4Template<Type>& vCc)
{
  x -= vCc.x;
  y -= vCc.y;
  z -= vCc.z;
  w -= vCc.w;

  XII_NAN_ASSERT(this);
}

template <typename Type>
XII_FORCE_INLINE void xiiVec4Template<Type>::operator*=(Type f)
{
  x *= f;
  y *= f;
  z *= f;
  w *= f;

  XII_NAN_ASSERT(this);
}

template <typename Type>
XII_FORCE_INLINE void xiiVec4Template<Type>::operator/=(Type f)
{
  if constexpr (std::is_floating_point_v<Type>)
  {
    const Type fInverse = xiiMath::Invert(f);

    x *= fInverse;
    y *= fInverse;
    z *= fInverse;
    w *= fInverse;
  }
  else
  {
    x /= f;
    y /= f;
    z /= f;
    w /= f;
  }

  XII_NAN_ASSERT(this);
}

template <typename Type>
XII_FORCE_INLINE Type xiiVec4Template<Type>::Dot(const xiiVec4Template<Type>& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return ((x * rhs.x) + (y * rhs.y) + (z * rhs.z) + (w * rhs.w));
}

template <typename Type>
inline const xiiVec4Template<Type> xiiVec4Template<Type>::CompMin(const xiiVec4Template<Type>& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return xiiVec4Template<Type>(xiiMath::Min(x, rhs.x), xiiMath::Min(y, rhs.y), xiiMath::Min(z, rhs.z), xiiMath::Min(w, rhs.w));
}

template <typename Type>
inline const xiiVec4Template<Type> xiiVec4Template<Type>::CompMax(const xiiVec4Template<Type>& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return xiiVec4Template<Type>(xiiMath::Max(x, rhs.x), xiiMath::Max(y, rhs.y), xiiMath::Max(z, rhs.z), xiiMath::Max(w, rhs.w));
}

template <typename Type>
inline const xiiVec4Template<Type> xiiVec4Template<Type>::CompClamp(const xiiVec4Template& vLow, const xiiVec4Template& vHigh) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&vLow);
  XII_NAN_ASSERT(&vHigh);

  return xiiVec4Template<Type>(xiiMath::Clamp(x, vLow.x, vHigh.x), xiiMath::Clamp(y, vLow.y, vHigh.y), xiiMath::Clamp(z, vLow.z, vHigh.z), xiiMath::Clamp(w, vLow.w, vHigh.w));
}

template <typename Type>
inline const xiiVec4Template<Type> xiiVec4Template<Type>::CompMul(const xiiVec4Template<Type>& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return xiiVec4Template<Type>(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w);
}

XII_MSVC_ANALYSIS_WARNING_PUSH
XII_MSVC_ANALYSIS_WARNING_DISABLE(4723)
template <typename Type>
inline const xiiVec4Template<Type> xiiVec4Template<Type>::CompDiv(const xiiVec4Template<Type>& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return xiiVec4Template<Type>(x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w);
}
XII_MSVC_ANALYSIS_WARNING_POP

template <typename Type>
inline const xiiVec4Template<Type> xiiVec4Template<Type>::Abs() const
{
  XII_NAN_ASSERT(this);

  return xiiVec4Template<Type>(xiiMath::Abs(x), xiiMath::Abs(y), xiiMath::Abs(z), xiiMath::Abs(w));
}

template <typename Type>
XII_FORCE_INLINE const xiiVec4Template<Type> operator+(const xiiVec4Template<Type>& v1, const xiiVec4Template<Type>& v2)
{
  XII_NAN_ASSERT(&v1);
  XII_NAN_ASSERT(&v2);

  return xiiVec4Template<Type>(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w);
}

template <typename Type>
XII_FORCE_INLINE const xiiVec4Template<Type> operator-(const xiiVec4Template<Type>& v1, const xiiVec4Template<Type>& v2)
{
  XII_NAN_ASSERT(&v1);
  XII_NAN_ASSERT(&v2);

  return xiiVec4Template<Type>(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w);
}

template <typename Type>
XII_FORCE_INLINE const xiiVec4Template<Type> operator*(Type f, const xiiVec4Template<Type>& v)
{
  XII_NAN_ASSERT(&v);

  return xiiVec4Template<Type>(v.x * f, v.y * f, v.z * f, v.w * f);
}

template <typename Type>
XII_FORCE_INLINE const xiiVec4Template<Type> operator*(const xiiVec4Template<Type>& v, Type f)
{
  XII_NAN_ASSERT(&v);

  return xiiVec4Template<Type>(v.x * f, v.y * f, v.z * f, v.w * f);
}

template <typename Type>
XII_FORCE_INLINE const xiiVec4Template<Type> operator/(const xiiVec4Template<Type>& v, Type f)
{
  XII_NAN_ASSERT(&v);

  if constexpr (std::is_floating_point_v<Type>)
  {
    // Multiplication is much faster than division
    const Type fInverse = xiiMath::Invert(f);
    return xiiVec4Template<Type>(v.x * fInverse, v.y * fInverse, v.z * fInverse, v.w * fInverse);
  }
  else
  {
    return xiiVec4Template<Type>(v.x / f, v.y / f, v.z / f, v.w / f);
  }
}

template <typename Type>
inline bool xiiVec4Template<Type>::IsIdentical(const xiiVec4Template<Type>& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return ((x == rhs.x) && (y == rhs.y) && (z == rhs.z) && (w == rhs.w));
}

template <typename Type>
inline bool xiiVec4Template<Type>::IsEqual(const xiiVec4Template<Type>& rhs, Type fEpsilon) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return (xiiMath::IsEqual(x, rhs.x, fEpsilon) && xiiMath::IsEqual(y, rhs.y, fEpsilon) && xiiMath::IsEqual(z, rhs.z, fEpsilon) && xiiMath::IsEqual(w, rhs.w, fEpsilon));
}

template <typename Type>
XII_ALWAYS_INLINE bool operator==(const xiiVec4Template<Type>& v1, const xiiVec4Template<Type>& v2)
{
  return v1.IsIdentical(v2);
}

template <typename Type>
XII_FORCE_INLINE bool operator<(const xiiVec4Template<Type>& v1, const xiiVec4Template<Type>& v2)
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
  if (v1.z < v2.z)
    return true;
  if (v1.z > v2.z)
    return false;

  return (v1.w < v2.w);
}
