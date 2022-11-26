#pragma once

#include <Foundation/Math/Mat4.h>

template <typename Type>
XII_ALWAYS_INLINE xiiBoundingBoxTemplate<Type>::xiiBoundingBoxTemplate()
{
}

template <typename Type>
XII_FORCE_INLINE xiiBoundingBoxTemplate<Type>::xiiBoundingBoxTemplate(const xiiVec3Template<Type>& vMin, const xiiVec3Template<Type>& vMax)
{
  SetElements(vMin, vMax);
}

template <typename Type>
XII_FORCE_INLINE void xiiBoundingBoxTemplate<Type>::SetElements(const xiiVec3Template<Type>& vMin, const xiiVec3Template<Type>& vMax)
{
  m_vMin = vMin;
  m_vMax = vMax;

  XII_ASSERT_DEBUG(IsValid(), "The given values did not create a valid bounding box ({0} | {1} | {2} - {3} | {4} | {5})", xiiArgF(vMin.x, 2),
                   xiiArgF(vMin.y, 2), xiiArgF(vMin.z, 2), xiiArgF(vMax.x, 2), xiiArgF(vMax.y, 2), xiiArgF(vMax.z, 2));
}

template <typename Type>
void xiiBoundingBoxTemplate<Type>::SetFromPoints(
  const xiiVec3Template<Type>* pPoints,
  xiiUInt32                    uiNumPoints,
  xiiUInt32                    uiStride /* = sizeof(xiiVec3Template<Type>) */)
{
  SetInvalid();
  ExpandToInclude(pPoints, uiNumPoints, uiStride);
}

template <typename Type>
void xiiBoundingBoxTemplate<Type>::GetCorners(xiiVec3Template<Type>* out_pCorners) const
{
  XII_NAN_ASSERT(this);
  XII_ASSERT_DEBUG(out_pCorners != nullptr, "Out Parameter must not be nullptr.");

  out_pCorners[0].Set(m_vMin.x, m_vMin.y, m_vMin.z);
  out_pCorners[1].Set(m_vMin.x, m_vMin.y, m_vMax.z);
  out_pCorners[2].Set(m_vMin.x, m_vMax.y, m_vMin.z);
  out_pCorners[3].Set(m_vMin.x, m_vMax.y, m_vMax.z);
  out_pCorners[4].Set(m_vMax.x, m_vMin.y, m_vMin.z);
  out_pCorners[5].Set(m_vMax.x, m_vMin.y, m_vMax.z);
  out_pCorners[6].Set(m_vMax.x, m_vMax.y, m_vMin.z);
  out_pCorners[7].Set(m_vMax.x, m_vMax.y, m_vMax.z);
}

template <typename Type>
XII_FORCE_INLINE const xiiVec3Template<Type> xiiBoundingBoxTemplate<Type>::GetCenter() const
{
  return m_vMin + GetHalfExtents();
}

template <typename Type>
XII_ALWAYS_INLINE const xiiVec3Template<Type> xiiBoundingBoxTemplate<Type>::GetExtents() const
{
  return m_vMax - m_vMin;
}

template <typename Type>
const xiiVec3Template<Type> xiiBoundingBoxTemplate<Type>::GetHalfExtents() const
{
  return (m_vMax - m_vMin) / (Type)2;
}

template <typename Type>
void xiiBoundingBoxTemplate<Type>::SetCenterAndHalfExtents(const xiiVec3Template<Type>& vCenter, const xiiVec3Template<Type>& vHalfExtents)
{
  m_vMin = vCenter - vHalfExtents;
  m_vMax = vCenter + vHalfExtents;
}

template <typename Type>
void xiiBoundingBoxTemplate<Type>::SetInvalid()
{
  m_vMin.Set(xiiMath::MaxValue<Type>());
  m_vMax.Set(-xiiMath::MaxValue<Type>());
}

template <typename Type>
bool xiiBoundingBoxTemplate<Type>::IsValid() const
{
  return (m_vMin.IsValid() && m_vMax.IsValid() && m_vMin.x <= m_vMax.x && m_vMin.y <= m_vMax.y && m_vMin.z <= m_vMax.z);
}

template <typename Type>
bool xiiBoundingBoxTemplate<Type>::IsNaN() const
{
  return m_vMin.IsNaN() || m_vMax.IsNaN();
}

template <typename Type>
XII_FORCE_INLINE void xiiBoundingBoxTemplate<Type>::ExpandToInclude(const xiiVec3Template<Type>& vPoint)
{
  m_vMin = m_vMin.CompMin(vPoint);
  m_vMax = m_vMax.CompMax(vPoint);
}

template <typename Type>
XII_FORCE_INLINE void xiiBoundingBoxTemplate<Type>::ExpandToInclude(const xiiBoundingBoxTemplate<Type>& rhs)
{
  XII_ASSERT_DEBUG(rhs.IsValid(), "rhs must be a valid AABB.");
  ExpandToInclude(rhs.m_vMin);
  ExpandToInclude(rhs.m_vMax);
}

template <typename Type>
void xiiBoundingBoxTemplate<Type>::ExpandToInclude(const xiiVec3Template<Type>* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride)
{
  XII_ASSERT_DEBUG(pPoints != nullptr, "Array may not be nullptr.");
  XII_ASSERT_DEBUG(uiStride >= sizeof(xiiVec3Template<Type>), "Data may not overlap.");

  const xiiVec3Template<Type>* pCur = &pPoints[0];

  for (xiiUInt32 i = 0; i < uiNumPoints; ++i)
  {
    ExpandToInclude(*pCur);

    pCur = xiiMemoryUtils::AddByteOffset(pCur, uiStride);
  }
}

template <typename Type>
void xiiBoundingBoxTemplate<Type>::ExpandToCube()
{
  xiiVec3Template<Type>       vHalfExtents = GetHalfExtents();
  const xiiVec3Template<Type> vCenter      = m_vMin + vHalfExtents;

  const Type f = xiiMath::Max(vHalfExtents.x, vHalfExtents.y, vHalfExtents.z);

  m_vMin = vCenter - xiiVec3Template<Type>(f);
  m_vMax = vCenter + xiiVec3Template<Type>(f);
}

template <typename Type>
XII_FORCE_INLINE void xiiBoundingBoxTemplate<Type>::Grow(const xiiVec3Template<Type>& vDiff)
{
  XII_ASSERT_DEBUG(IsValid(), "Cannot grow a box that is invalid.");

  m_vMax += vDiff;
  m_vMin -= vDiff;

  XII_ASSERT_DEBUG(IsValid(), "The grown box has become invalid.");
}

template <typename Type>
XII_FORCE_INLINE bool xiiBoundingBoxTemplate<Type>::Contains(const xiiVec3Template<Type>& vPoint) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&vPoint);

  return (xiiMath::IsInRange(vPoint.x, m_vMin.x, m_vMax.x) && xiiMath::IsInRange(vPoint.y, m_vMin.y, m_vMax.y) &&
          xiiMath::IsInRange(vPoint.z, m_vMin.z, m_vMax.z));
}

template <typename Type>
XII_FORCE_INLINE bool xiiBoundingBoxTemplate<Type>::Contains(const xiiBoundingBoxTemplate<Type>& rhs) const
{
  return Contains(rhs.m_vMin) && Contains(rhs.m_vMax);
}

template <typename Type>
bool xiiBoundingBoxTemplate<Type>::Contains(
  const xiiVec3Template<Type>* pPoints,
  xiiUInt32                    uiNumPoints,
  xiiUInt32                    uiStride /* = sizeof(xiiVec3Template<Type>) */) const
{
  XII_ASSERT_DEBUG(pPoints != nullptr, "Array must not be NuLL.");
  XII_ASSERT_DEBUG(uiStride >= sizeof(xiiVec3Template<Type>), "Data must not overlap.");

  const xiiVec3Template<Type>* pCur = &pPoints[0];

  for (xiiUInt32 i = 0; i < uiNumPoints; ++i)
  {
    if (!Contains(*pCur))
      return false;

    pCur = xiiMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  return true;
}

template <typename Type>
bool xiiBoundingBoxTemplate<Type>::Overlaps(const xiiBoundingBoxTemplate<Type>& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  if (rhs.m_vMin.x >= m_vMax.x)
    return false;
  if (rhs.m_vMin.y >= m_vMax.y)
    return false;
  if (rhs.m_vMin.z >= m_vMax.z)
    return false;

  if (m_vMin.x >= rhs.m_vMax.x)
    return false;
  if (m_vMin.y >= rhs.m_vMax.y)
    return false;
  if (m_vMin.z >= rhs.m_vMax.z)
    return false;

  return true;
}

template <typename Type>
bool xiiBoundingBoxTemplate<Type>::Overlaps(
  const xiiVec3Template<Type>* pPoints,
  xiiUInt32                    uiNumPoints,
  xiiUInt32                    uiStride /* = sizeof(xiiVec3Template<Type>) */) const
{
  XII_ASSERT_DEBUG(pPoints != nullptr, "Array must not be NuLL.");
  XII_ASSERT_DEBUG(uiStride >= sizeof(xiiVec3Template<Type>), "Data must not overlap.");

  const xiiVec3Template<Type>* pCur = &pPoints[0];

  for (xiiUInt32 i = 0; i < uiNumPoints; ++i)
  {
    if (Contains(*pCur))
      return true;

    pCur = xiiMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  return false;
}

template <typename Type>
XII_ALWAYS_INLINE bool xiiBoundingBoxTemplate<Type>::IsIdentical(const xiiBoundingBoxTemplate<Type>& rhs) const
{
  return (m_vMin == rhs.m_vMin && m_vMax == rhs.m_vMax);
}

template <typename Type>
bool xiiBoundingBoxTemplate<Type>::IsEqual(const xiiBoundingBoxTemplate<Type>& rhs, Type fEpsilon) const
{
  return (m_vMin.IsEqual(rhs.m_vMin, fEpsilon) && m_vMax.IsEqual(rhs.m_vMax, fEpsilon));
}

template <typename Type>
XII_ALWAYS_INLINE bool operator==(const xiiBoundingBoxTemplate<Type>& lhs, const xiiBoundingBoxTemplate<Type>& rhs)
{
  return lhs.IsIdentical(rhs);
}

template <typename Type>
XII_ALWAYS_INLINE bool operator!=(const xiiBoundingBoxTemplate<Type>& lhs, const xiiBoundingBoxTemplate<Type>& rhs)
{
  return !lhs.IsIdentical(rhs);
}

template <typename Type>
XII_FORCE_INLINE void xiiBoundingBoxTemplate<Type>::Translate(const xiiVec3Template<Type>& vDiff)
{
  m_vMin += vDiff;
  m_vMax += vDiff;
}

template <typename Type>
void xiiBoundingBoxTemplate<Type>::ScaleFromCenter(const xiiVec3Template<Type>& vScale)
{
  const xiiVec3Template<Type> vCenter = GetCenter();
  const xiiVec3               vNewMin = vCenter + (m_vMin - vCenter).CompMul(vScale);
  const xiiVec3               vNewMax = vCenter + (m_vMax - vCenter).CompMul(vScale);

  // this is necessary for negative scalings to work as expected
  m_vMin = vNewMin.CompMin(vNewMax);
  m_vMax = vNewMin.CompMax(vNewMax);
}

template <typename Type>
XII_FORCE_INLINE void xiiBoundingBoxTemplate<Type>::ScaleFromOrigin(const xiiVec3Template<Type>& vScale)
{
  const xiiVec3 vNewMin = m_vMin.CompMul(vScale);
  const xiiVec3 vNewMax = m_vMax.CompMul(vScale);

  // this is necessary for negative scalings to work as expected
  m_vMin = vNewMin.CompMin(vNewMax);
  m_vMax = vNewMin.CompMax(vNewMax);
}

template <typename Type>
void xiiBoundingBoxTemplate<Type>::TransformFromCenter(const xiiMat4Template<Type>& mTransform)
{
  xiiVec3Template<Type> vCorners[8];
  GetCorners(vCorners);

  const xiiVec3Template<Type> vCenter = GetCenter();
  SetInvalid();

  for (xiiUInt32 i = 0; i < 8; ++i)
    ExpandToInclude(vCenter + mTransform.TransformPosition(vCorners[i] - vCenter));
}

template <typename Type>
void xiiBoundingBoxTemplate<Type>::TransformFromOrigin(const xiiMat4Template<Type>& mTransform)
{
  xiiVec3Template<Type> vCorners[8];
  GetCorners(vCorners);

  mTransform.TransformPosition(vCorners, 8);

  SetInvalid();
  ExpandToInclude(vCorners, 8);
}

template <typename Type>
XII_FORCE_INLINE const xiiVec3Template<Type> xiiBoundingBoxTemplate<Type>::GetClampedPoint(const xiiVec3Template<Type>& vPoint) const
{
  return vPoint.CompMin(m_vMax).CompMax(m_vMin);
}

template <typename Type>
Type xiiBoundingBoxTemplate<Type>::GetDistanceTo(const xiiVec3Template<Type>& vPoint) const
{
  const xiiVec3Template<Type> vClamped = GetClampedPoint(vPoint);

  return (vPoint - vClamped).GetLength();
}

template <typename Type>
Type xiiBoundingBoxTemplate<Type>::GetDistanceSquaredTo(const xiiVec3Template<Type>& vPoint) const
{
  const xiiVec3Template<Type> vClamped = GetClampedPoint(vPoint);

  return (vPoint - vClamped).GetLengthSquared();
}

template <typename Type>
Type xiiBoundingBoxTemplate<Type>::GetDistanceSquaredTo(const xiiBoundingBoxTemplate<Type>& rhs) const
{
  // This will return zero for overlapping boxes

  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  Type fDistSQR = 0.0f;

  {
    if (rhs.m_vMin.x > m_vMax.x)
    {
      fDistSQR += xiiMath::Square(rhs.m_vMin.x - m_vMax.x);
    }
    else if (rhs.m_vMax.x < m_vMin.x)
    {
      fDistSQR += xiiMath::Square(m_vMin.x - rhs.m_vMax.x);
    }
  }

  {
    if (rhs.m_vMin.y > m_vMax.y)
    {
      fDistSQR += xiiMath::Square(rhs.m_vMin.y - m_vMax.y);
    }
    else if (rhs.m_vMax.y < m_vMin.y)
    {
      fDistSQR += xiiMath::Square(m_vMin.y - rhs.m_vMax.y);
    }
  }

  {
    if (rhs.m_vMin.z > m_vMax.z)
    {
      fDistSQR += xiiMath::Square(rhs.m_vMin.z - m_vMax.z);
    }
    else if (rhs.m_vMax.z < m_vMin.z)
    {
      fDistSQR += xiiMath::Square(m_vMin.z - rhs.m_vMax.z);
    }
  }

  return fDistSQR;
}

template <typename Type>
Type xiiBoundingBoxTemplate<Type>::GetDistanceTo(const xiiBoundingBoxTemplate<Type>& rhs) const
{
  return xiiMath::Sqrt(GetDistanceSquaredTo(rhs));
}

template <typename Type>
bool xiiBoundingBoxTemplate<Type>::GetRayIntersection(
  const xiiVec3Template<Type>& vStartPos,
  const xiiVec3Template<Type>& vRayDir,
  Type*                        out_fIntersection,
  xiiVec3Template<Type>*       out_vIntersection) const
{
  // This code was taken from: http://people.csail.mit.edu/amy/papers/box-jgt.pdf
  // "An Efficient and Robust Ray-Box Intersection Algorithm"
  // Contrary to previous implementation, this one actually works with ray/box configurations
  // that produce division by zero and multiplication with infinity (which can produce NaNs).

  XII_ASSERT_DEBUG(xiiMath::SupportsInfinity<Type>(), "This type does not support infinite values, which is required for this algorithm.");
  XII_ASSERT_DEBUG(vStartPos.IsValid(), "Ray start position must be valid.");
  XII_ASSERT_DEBUG(vRayDir.IsValid(), "Ray direction must be valid.");

  XII_NAN_ASSERT(this);

  float tMin, tMax;

  // Compare along X and Z axis, find intersection point
  {
    float tMinY, tMaxY;

    const float fDivX = 1.0f / vRayDir.x;
    const float fDivY = 1.0f / vRayDir.y;

    if (vRayDir.x >= 0.0f)
    {
      tMin = (m_vMin.x - vStartPos.x) * fDivX;
      tMax = (m_vMax.x - vStartPos.x) * fDivX;
    }
    else
    {
      tMin = (m_vMax.x - vStartPos.x) * fDivX;
      tMax = (m_vMin.x - vStartPos.x) * fDivX;
    }

    if (vRayDir.y >= 0.0f)
    {
      tMinY = (m_vMin.y - vStartPos.y) * fDivY;
      tMaxY = (m_vMax.y - vStartPos.y) * fDivY;
    }
    else
    {
      tMinY = (m_vMax.y - vStartPos.y) * fDivY;
      tMaxY = (m_vMin.y - vStartPos.y) * fDivY;
    }

    if (tMin > tMaxY || tMinY > tMax)
      return false;

    if (tMinY > tMin)
      tMin = tMinY;
    if (tMaxY < tMax)
      tMax = tMaxY;
  }

  // Compare along Z axis and previous result, find intersection point
  {
    float tMinZ, tMaxZ;

    const float fDivZ = 1.0f / vRayDir.z;

    if (vRayDir.z >= 0.0f)
    {
      tMinZ = (m_vMin.z - vStartPos.z) * fDivZ;
      tMaxZ = (m_vMax.z - vStartPos.z) * fDivZ;
    }
    else
    {
      tMinZ = (m_vMax.z - vStartPos.z) * fDivZ;
      tMaxZ = (m_vMin.z - vStartPos.z) * fDivZ;
    }

    if (tMin > tMaxZ || tMinZ > tMax)
      return false;

    if (tMinZ > tMin)
      tMin = tMinZ;
    if (tMaxZ < tMax)
      tMax = tMaxZ;
  }

  // rays that start inside the box are considered as not hitting the box
  if (tMax <= 0.0f)
    return false;

  if (out_fIntersection)
    *out_fIntersection = tMin;

  if (out_vIntersection)
    *out_vIntersection = vStartPos + tMin * vRayDir;

  return true;
}

template <typename Type>
bool xiiBoundingBoxTemplate<Type>::GetLineSegmentIntersection(
  const xiiVec3Template<Type>& vStartPos,
  const xiiVec3Template<Type>& vEndPos,
  Type*                        out_fLineFraction,
  xiiVec3Template<Type>*       out_vIntersection) const
{
  const xiiVec3Template<Type> vRayDir = vEndPos - vStartPos;

  Type fIntersection = 0.0f;
  if (!GetRayIntersection(vStartPos, vRayDir, &fIntersection, out_vIntersection))
    return false;

  if (out_fLineFraction)
    *out_fLineFraction = fIntersection;

  return fIntersection <= 1.0f;
}



#include <Foundation/Math/Implementation/AllClasses_inl.h>
