#pragma once

#include <Foundation/Math/Mat4.h>

template <typename Type>
XII_FORCE_INLINE xiiBoundingSphereTemplate<Type>::xiiBoundingSphereTemplate()
{
#if XII_ENABLED(XII_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  // m_vCenter is already initialized to NaN by its own constructor.
  const Type TypeNaN = xiiMath::NaN<Type>();
  m_fRadius          = TypeNaN;
#endif
}

template <typename Type>
XII_FORCE_INLINE xiiBoundingSphereTemplate<Type> xiiBoundingSphereTemplate<Type>::MakeZero()
{
  xiiBoundingSphereTemplate<Type> res;
  res.m_vCenter.SetZero();
  res.m_fRadius = 0.0f;
  return res;
}

template <typename Type>
XII_FORCE_INLINE xiiBoundingSphereTemplate<Type> xiiBoundingSphereTemplate<Type>::MakeInvalid(const xiiVec3Template<Type>& vCenter)
{
  xiiBoundingSphereTemplate<Type> res;
  res.m_vCenter = vCenter;
  res.m_fRadius = -xiiMath::SmallEpsilon<Type>(); // has to be very small for ExpandToInclude to work
  return res;
}

template <typename Type>
XII_FORCE_INLINE xiiBoundingSphereTemplate<Type> xiiBoundingSphereTemplate<Type>::MakeFromCenterAndRadius(const xiiVec3Template<Type>& vCenter, Type fRadius)
{
  xiiBoundingSphereTemplate<Type> res;
  res.m_vCenter = vCenter;
  res.m_fRadius = fRadius;
  XII_ASSERT_DEBUG(res.IsValid(), "The sphere was created with invalid values.");
  return res;
}

template <typename Type>
XII_FORCE_INLINE xiiBoundingSphereTemplate<Type> xiiBoundingSphereTemplate<Type>::MakeFromPoints(const xiiVec3Template<Type>* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride /*= sizeof(xiiVec3Template<Type>)*/)
{
  XII_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  XII_ASSERT_DEBUG(uiStride >= sizeof(xiiVec3Template<Type>), "The data must not overlap.");
  XII_ASSERT_DEBUG(uiNumPoints > 0, "The array must contain at least one point.");

  const xiiVec3Template<Type>* pCur = &pPoints[0];

  xiiVec3Template<Type> vCenter(0.0f);

  for (xiiUInt32 i = 0; i < uiNumPoints; ++i)
  {
    vCenter += *pCur;
    pCur = xiiMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  vCenter /= (Type)uiNumPoints;

  Type fMaxDistSQR = 0.0f;

  pCur = &pPoints[0];
  for (xiiUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const Type fDistSQR = (*pCur - vCenter).GetLengthSquared();
    fMaxDistSQR         = xiiMath::Max(fMaxDistSQR, fDistSQR);

    pCur = xiiMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  xiiBoundingSphereTemplate<Type> res;
  res.m_vCenter = vCenter;
  res.m_fRadius = xiiMath::Sqrt(fMaxDistSQR);

  XII_ASSERT_DEBUG(res.IsValid(), "The point cloud contained corrupted data.");

  return res;
}

template <typename Type>
bool xiiBoundingSphereTemplate<Type>::IsZero(Type fEpsilon /* = xiiMath::DefaultEpsilon<Type>() */) const
{
  return m_vCenter.IsZero(fEpsilon) && xiiMath::IsZero(m_fRadius, fEpsilon);
}

template <typename Type>
bool xiiBoundingSphereTemplate<Type>::IsValid() const
{
  return (m_vCenter.IsValid() && m_fRadius >= 0.0f);
}

template <typename Type>
bool xiiBoundingSphereTemplate<Type>::IsNaN() const
{
  return (m_vCenter.IsNaN() || xiiMath::IsNaN(m_fRadius));
}

template <typename Type>
void xiiBoundingSphereTemplate<Type>::ExpandToInclude(const xiiVec3Template<Type>& vPoint)
{
  const Type fDistSQR = (vPoint - m_vCenter).GetLengthSquared();

  if (xiiMath::Square(m_fRadius) < fDistSQR)
    m_fRadius = xiiMath::Sqrt(fDistSQR);
}

template <typename Type>
void xiiBoundingSphereTemplate<Type>::ExpandToInclude(const xiiBoundingSphereTemplate<Type>& rhs)
{
  const Type fReqRadius = (rhs.m_vCenter - m_vCenter).GetLength() + rhs.m_fRadius;

  m_fRadius = xiiMath::Max(m_fRadius, fReqRadius);
}

template <typename Type>
XII_FORCE_INLINE void xiiBoundingSphereTemplate<Type>::Grow(Type fDiff)
{
  XII_ASSERT_DEBUG(IsValid(), "Cannot grow a sphere that is invalid.");

  m_fRadius += fDiff;

  XII_ASSERT_DEBUG(IsValid(), "The grown sphere has become invalid.");
}

template <typename Type>
bool xiiBoundingSphereTemplate<Type>::IsIdentical(const xiiBoundingSphereTemplate<Type>& rhs) const
{
  return (m_vCenter.IsIdentical(rhs.m_vCenter) && m_fRadius == rhs.m_fRadius);
}

template <typename Type>
bool xiiBoundingSphereTemplate<Type>::IsEqual(const xiiBoundingSphereTemplate<Type>& rhs, Type fEpsilon) const
{
  return (m_vCenter.IsEqual(rhs.m_vCenter, fEpsilon) && xiiMath::IsEqual(m_fRadius, rhs.m_fRadius, fEpsilon));
}

template <typename Type>
XII_ALWAYS_INLINE bool operator==(const xiiBoundingSphereTemplate<Type>& lhs, const xiiBoundingSphereTemplate<Type>& rhs)
{
  return lhs.IsIdentical(rhs);
}

template <typename Type>
XII_ALWAYS_INLINE bool operator!=(const xiiBoundingSphereTemplate<Type>& lhs, const xiiBoundingSphereTemplate<Type>& rhs)
{
  return !lhs.IsIdentical(rhs);
}

template <typename Type>
XII_ALWAYS_INLINE void xiiBoundingSphereTemplate<Type>::Translate(const xiiVec3Template<Type>& vTranslation)
{
  m_vCenter += vTranslation;
}

template <typename Type>
XII_FORCE_INLINE void xiiBoundingSphereTemplate<Type>::ScaleFromCenter(Type fScale)
{
  XII_ASSERT_DEBUG(fScale >= 0.0f, "Cannot invert the sphere.");

  m_fRadius *= fScale;

  XII_NAN_ASSERT(this);
}

template <typename Type>
void xiiBoundingSphereTemplate<Type>::ScaleFromOrigin(const xiiVec3Template<Type>& vScale)
{
  XII_ASSERT_DEBUG(vScale.x >= 0.0f, "Cannot invert the sphere.");
  XII_ASSERT_DEBUG(vScale.y >= 0.0f, "Cannot invert the sphere.");
  XII_ASSERT_DEBUG(vScale.z >= 0.0f, "Cannot invert the sphere.");

  m_vCenter = m_vCenter.CompMul(vScale);

  // scale the radius by the maximum scaling factor (the sphere cannot become an ellipsoid,
  // so to be a 'bounding' sphere, it should be as large as possible
  m_fRadius *= xiiMath::Max(vScale.x, vScale.y, vScale.z);
}

template <typename Type>
void xiiBoundingSphereTemplate<Type>::TransformFromOrigin(const xiiMat4Template<Type>& mTransform)
{
  m_vCenter = mTransform.TransformPosition(m_vCenter);

  const xiiVec3Template<Type> Scale = mTransform.GetScalingFactors();
  m_fRadius *= xiiMath::Max(Scale.x, Scale.y, Scale.z);
}

template <typename Type>
void xiiBoundingSphereTemplate<Type>::TransformFromCenter(const xiiMat4Template<Type>& mTransform)
{
  m_vCenter += mTransform.GetTranslationVector();

  const xiiVec3Template<Type> Scale = mTransform.GetScalingFactors();
  m_fRadius *= xiiMath::Max(Scale.x, Scale.y, Scale.z);
}

template <typename Type>
Type xiiBoundingSphereTemplate<Type>::GetDistanceTo(const xiiVec3Template<Type>& vPoint) const
{
  return (vPoint - m_vCenter).GetLength() - m_fRadius;
}

template <typename Type>
Type xiiBoundingSphereTemplate<Type>::GetDistanceTo(const xiiBoundingSphereTemplate<Type>& rhs) const
{
  return (rhs.m_vCenter - m_vCenter).GetLength() - m_fRadius - rhs.m_fRadius;
}

template <typename Type>
bool xiiBoundingSphereTemplate<Type>::Contains(const xiiVec3Template<Type>& vPoint) const
{
  return (vPoint - m_vCenter).GetLengthSquared() <= xiiMath::Square(m_fRadius);
}

template <typename Type>
bool xiiBoundingSphereTemplate<Type>::Contains(const xiiBoundingSphereTemplate<Type>& rhs) const
{
  return (rhs.m_vCenter - m_vCenter).GetLength() + rhs.m_fRadius <= m_fRadius;
}

template <typename Type>
bool xiiBoundingSphereTemplate<Type>::Overlaps(const xiiBoundingSphereTemplate<Type>& rhs) const
{
  return (rhs.m_vCenter - m_vCenter).GetLengthSquared() < xiiMath::Square(rhs.m_fRadius + m_fRadius);
}

template <typename Type>
const xiiVec3Template<Type> xiiBoundingSphereTemplate<Type>::GetClampedPoint(const xiiVec3Template<Type>& vPoint)
{
  const xiiVec3Template<Type> vDir     = vPoint - m_vCenter;
  const Type                  fDistSQR = vDir.GetLengthSquared();

  // return the point, if it is already inside the sphere
  if (fDistSQR <= xiiMath::Square(m_fRadius))
    return vPoint;

  // otherwise return a point on the surface of the sphere

  const Type fLength = xiiMath::Sqrt(fDistSQR);

  return m_vCenter + m_fRadius * (vDir / fLength);
}

template <typename Type>
bool xiiBoundingSphereTemplate<Type>::Contains(const xiiVec3Template<Type>* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride /* = sizeof(xiiVec3Template) */) const
{
  XII_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  XII_ASSERT_DEBUG(uiNumPoints > 0, "The array must contain at least one point.");
  XII_ASSERT_DEBUG(uiStride >= sizeof(xiiVec3Template<Type>), "The data must not overlap.");

  const Type fRadiusSQR = xiiMath::Square(m_fRadius);

  const xiiVec3Template<Type>* pCur = &pPoints[0];

  for (xiiUInt32 i = 0; i < uiNumPoints; ++i)
  {
    if ((*pCur - m_vCenter).GetLengthSquared() > fRadiusSQR)
      return false;

    pCur = xiiMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  return true;
}

template <typename Type>
bool xiiBoundingSphereTemplate<Type>::Overlaps(const xiiVec3Template<Type>* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride /* = sizeof(xiiVec3Template) */) const
{
  XII_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  XII_ASSERT_DEBUG(uiNumPoints > 0, "The array must contain at least one point.");
  XII_ASSERT_DEBUG(uiStride >= sizeof(xiiVec3Template<Type>), "The data must not overlap.");

  const Type fRadiusSQR = xiiMath::Square(m_fRadius);

  const xiiVec3Template<Type>* pCur = &pPoints[0];

  for (xiiUInt32 i = 0; i < uiNumPoints; ++i)
  {
    if ((*pCur - m_vCenter).GetLengthSquared() <= fRadiusSQR)
      return true;

    pCur = xiiMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  return false;
}

template <typename Type>
void xiiBoundingSphereTemplate<Type>::ExpandToInclude(const xiiVec3Template<Type>* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride /* = sizeof(xiiVec3Template) */)
{
  XII_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  XII_ASSERT_DEBUG(uiStride >= sizeof(xiiVec3Template<Type>), "The data must not overlap.");

  const xiiVec3Template<Type>* pCur = &pPoints[0];

  Type fMaxDistSQR = 0.0f;

  for (xiiUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const Type fDistSQR = (*pCur - m_vCenter).GetLengthSquared();
    fMaxDistSQR         = xiiMath::Max(fMaxDistSQR, fDistSQR);

    pCur = xiiMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  if (xiiMath::Square(m_fRadius) < fMaxDistSQR)
    m_fRadius = xiiMath::Sqrt(fMaxDistSQR);
}

template <typename Type>
Type xiiBoundingSphereTemplate<Type>::GetDistanceTo(const xiiVec3Template<Type>* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride /* = sizeof(xiiVec3Template) */) const
{
  XII_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  XII_ASSERT_DEBUG(uiNumPoints > 0, "The array must contain at least one point.");
  XII_ASSERT_DEBUG(uiStride >= sizeof(xiiVec3Template<Type>), "The data must not overlap.");

  const xiiVec3Template<Type>* pCur = &pPoints[0];

  Type fMinDistSQR = xiiMath::MaxValue<Type>();

  for (xiiUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const Type fDistSQR = (*pCur - m_vCenter).GetLengthSquared();

    fMinDistSQR = xiiMath::Min(fMinDistSQR, fDistSQR);

    pCur = xiiMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  return xiiMath::Sqrt(fMinDistSQR);
}

template <typename Type>
bool xiiBoundingSphereTemplate<Type>::GetRayIntersection(const xiiVec3Template<Type>& vRayStartPos, const xiiVec3Template<Type>& vRayDirNormalized, Type* out_pIntersectionDistance /* = nullptr */, xiiVec3Template<Type>* out_pIntersection /* = nullptr */) const
{
  XII_ASSERT_DEBUG(vRayDirNormalized.IsNormalized(), "The ray direction must be normalized.");

  // Ugly Code taken from 'Real Time Rendering First Edition' Page 299

  const Type                  fRadiusSQR = xiiMath::Square(m_fRadius);
  const xiiVec3Template<Type> vRelPos    = m_vCenter - vRayStartPos;

  const Type d             = vRelPos.Dot(vRayDirNormalized);
  const Type fRelPosLenSQR = vRelPos.GetLengthSquared();

  if (d < 0.0f && fRelPosLenSQR > fRadiusSQR)
    return false;

  const Type m2 = fRelPosLenSQR - xiiMath::Square(d);

  if (m2 > fRadiusSQR)
    return false;

  const Type q = xiiMath::Sqrt(fRadiusSQR - m2);

  Type fIntersectionTime;

  if (fRelPosLenSQR > fRadiusSQR)
    fIntersectionTime = d - q;
  else
    fIntersectionTime = d + q;

  if (out_pIntersectionDistance)
    *out_pIntersectionDistance = fIntersectionTime;
  if (out_pIntersection)
    *out_pIntersection = vRayStartPos + vRayDirNormalized * fIntersectionTime;

  return true;
}

template <typename Type>
bool xiiBoundingSphereTemplate<Type>::GetLineSegmentIntersection(const xiiVec3Template<Type>& vLineStartPos, const xiiVec3Template<Type>& vLineEndPos, Type* out_pHitFraction /* = nullptr */, xiiVec3Template<Type>* out_pIntersection /* = nullptr */) const
{
  Type fIntersection = 0.0f;

  const xiiVec3Template<Type> vDir     = vLineEndPos - vLineStartPos;
  xiiVec3Template<Type>       vDirNorm = vDir;
  const Type                  fLen     = vDirNorm.GetLengthAndNormalize();

  if (!GetRayIntersection(vLineStartPos, vDirNorm, &fIntersection))
    return false;

  if (fIntersection > fLen)
    return false;

  if (out_pHitFraction)
    *out_pHitFraction = fIntersection / fLen;

  if (out_pIntersection)
    *out_pIntersection = vLineStartPos + vDirNorm * fIntersection;

  return true;
}

#include <Foundation/Math/Implementation/AllClasses_inl.h>
