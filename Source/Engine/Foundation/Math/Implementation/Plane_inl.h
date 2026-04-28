/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Math/Mat4.h>

template <typename Type>
XII_FORCE_INLINE xiiPlaneTemplate<Type>::xiiPlaneTemplate()
{
#if XII_ENABLED(XII_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const Type TypeNaN = xiiMath::NaN<Type>();
  m_vNormal.Set(TypeNaN);
  m_fNegDistance = TypeNaN;
#endif
}

template <typename Type>
xiiPlaneTemplate<Type> xiiPlaneTemplate<Type>::MakeInvalid()
{
  xiiPlaneTemplate<Type> res;
  res.m_vNormal.Set(0);
  res.m_fNegDistance = 0;
  return res;
}

template <typename Type>
xiiPlaneTemplate<Type> xiiPlaneTemplate<Type>::MakeFromNormalAndPoint(const xiiVec3Template<Type>& vNormal, const xiiVec3Template<Type>& vPointOnPlane)
{
  XII_ASSERT_DEV(vNormal.IsNormalized(), "Normal must be normalized.");

  xiiPlaneTemplate<Type> res;
  res.m_vNormal      = vNormal;
  res.m_fNegDistance = -vNormal.Dot(vPointOnPlane);
  return res;
}

template <typename Type>
xiiPlaneTemplate<Type> xiiPlaneTemplate<Type>::MakeFromPoints(const xiiVec3Template<Type>& v1, const xiiVec3Template<Type>& v2, const xiiVec3Template<Type>& v3)
{
  xiiPlaneTemplate<Type> res;
  XII_VERIFY(res.m_vNormal.CalculateNormal(v1, v2, v3).Succeeded(), "The 3 provided points do not form a plane");

  res.m_fNegDistance = -res.m_vNormal.Dot(v1);
  return res;
}

template <typename Type>
xiiVec4Template<Type> xiiPlaneTemplate<Type>::GetAsVec4() const
{
  return xiiVec4(m_vNormal.x, m_vNormal.y, m_vNormal.z, m_fNegDistance);
}

template <typename Type>
xiiResult xiiPlaneTemplate<Type>::SetFromPoints(const xiiVec3Template<Type>& v1, const xiiVec3Template<Type>& v2, const xiiVec3Template<Type>& v3)
{
  if (m_vNormal.CalculateNormal(v1, v2, v3) == XII_FAILURE)
    return XII_FAILURE;

  m_fNegDistance = -m_vNormal.Dot(v1);
  return XII_SUCCESS;
}

template <typename Type>
xiiResult xiiPlaneTemplate<Type>::SetFromPoints(const xiiVec3Template<Type>* const pVertices)
{
  if (m_vNormal.CalculateNormal(pVertices[0], pVertices[1], pVertices[2]) == XII_FAILURE)
    return XII_FAILURE;

  m_fNegDistance = -m_vNormal.Dot(pVertices[0]);
  return XII_SUCCESS;
}

template <typename Type>
xiiResult xiiPlaneTemplate<Type>::SetFromDirections(const xiiVec3Template<Type>& vTangent1, const xiiVec3Template<Type>& vTangent2, const xiiVec3Template<Type>& vPointOnPlane)
{
  xiiVec3Template<Type> vNormal = vTangent1.CrossRH(vTangent2);
  xiiResult             res     = vNormal.NormalizeIfNotZero();

  m_vNormal      = vNormal;
  m_fNegDistance = -vNormal.Dot(vPointOnPlane);
  return res;
}

template <typename Type>
void xiiPlaneTemplate<Type>::Transform(const xiiMat3Template<Type>& m)
{
  xiiVec3Template<Type> vPointOnPlane = m_vNormal * -m_fNegDistance;

  // Transform the normal
  xiiVec3Template<Type> vTransformedNormal = m.TransformDirection(m_vNormal);

  // Normalize the normal vector
  const bool normalizeSucceeded = vTransformedNormal.NormalizeIfNotZero().Succeeded();
  XII_ASSERT_DEBUG(normalizeSucceeded, "");
  XII_IGNORE_UNUSED(normalizeSucceeded);

  // If the plane's distance is already infinite, there won't be any meaningful change
  // to it as a result of the transformation.
  if (!xiiMath::IsFinite(m_fNegDistance))
  {
    m_vNormal = vTransformedNormal;
  }
  else
  {
    *this = xiiPlane::MakeFromNormalAndPoint(vTransformedNormal, m * vPointOnPlane);
  }
}

template <typename Type>
void xiiPlaneTemplate<Type>::Transform(const xiiMat4Template<Type>& m)
{
  xiiVec3Template<Type> vPointOnPlane = m_vNormal * -m_fNegDistance;

  // Transform the normal
  xiiVec3Template<Type> vTransformedNormal = m.TransformDirection(m_vNormal);

  // Normalize the normal vector
  const bool normalizeSucceeded = vTransformedNormal.NormalizeIfNotZero().Succeeded();
  XII_ASSERT_DEBUG(normalizeSucceeded, "");
  XII_IGNORE_UNUSED(normalizeSucceeded);

  // If the plane's distance is already infinite, there won't be any meaningful change
  // to it as a result of the transformation.
  if (!xiiMath::IsFinite(m_fNegDistance))
  {
    m_vNormal = vTransformedNormal;
  }
  else
  {
    *this = xiiPlane::MakeFromNormalAndPoint(vTransformedNormal, m * vPointOnPlane);
  }
}

template <typename Type>
XII_FORCE_INLINE void xiiPlaneTemplate<Type>::Flip()
{
  m_fNegDistance = -m_fNegDistance;
  m_vNormal      = -m_vNormal;
}

template <typename Type>
XII_FORCE_INLINE Type xiiPlaneTemplate<Type>::GetDistanceTo(const xiiVec3Template<Type>& vPoint) const
{
  return (m_vNormal.Dot(vPoint) + m_fNegDistance);
}

template <typename Type>
XII_FORCE_INLINE xiiPositionOnPlane::Enum xiiPlaneTemplate<Type>::GetPointPosition(const xiiVec3Template<Type>& vPoint) const
{
  return (m_vNormal.Dot(vPoint) < -m_fNegDistance ? xiiPositionOnPlane::Back : xiiPositionOnPlane::Front);
}

template <typename Type>
xiiPositionOnPlane::Enum xiiPlaneTemplate<Type>::GetPointPosition(const xiiVec3Template<Type>& vPoint, Type fPlaneHalfWidth) const
{
  const Type f = m_vNormal.Dot(vPoint);

  if (f + fPlaneHalfWidth < -m_fNegDistance)
    return xiiPositionOnPlane::Back;

  if (f - fPlaneHalfWidth > -m_fNegDistance)
    return xiiPositionOnPlane::Front;

  return xiiPositionOnPlane::OnPlane;
}

template <typename Type>
XII_FORCE_INLINE const xiiVec3Template<Type> xiiPlaneTemplate<Type>::ProjectOntoPlane(const xiiVec3Template<Type>& vPoint) const
{
  return vPoint - m_vNormal * (m_vNormal.Dot(vPoint) + m_fNegDistance);
}

template <typename Type>
XII_FORCE_INLINE const xiiVec3Template<Type> xiiPlaneTemplate<Type>::Mirror(const xiiVec3Template<Type>& vPoint) const
{
  return vPoint - (Type)2 * GetDistanceTo(vPoint) * m_vNormal;
}

template <typename Type>
const xiiVec3Template<Type> xiiPlaneTemplate<Type>::GetCoplanarDirection(const xiiVec3Template<Type>& vDirection) const
{
  xiiVec3Template<Type> res = vDirection;
  res.MakeOrthogonalTo(m_vNormal);
  return res;
}

template <typename Type>
bool xiiPlaneTemplate<Type>::IsIdentical(const xiiPlaneTemplate& rhs) const
{
  return m_vNormal.IsIdentical(rhs.m_vNormal) && m_fNegDistance == rhs.m_fNegDistance;
}

template <typename Type>
bool xiiPlaneTemplate<Type>::IsEqual(const xiiPlaneTemplate& rhs, Type fEpsilon) const
{
  return m_vNormal.IsEqual(rhs.m_vNormal, fEpsilon) && xiiMath::IsEqual(m_fNegDistance, rhs.m_fNegDistance, fEpsilon);
}

template <typename Type>
XII_ALWAYS_INLINE bool operator==(const xiiPlaneTemplate<Type>& lhs, const xiiPlaneTemplate<Type>& rhs)
{
  return lhs.IsIdentical(rhs);
}

template <typename Type>
bool xiiPlaneTemplate<Type>::FlipIfNecessary(const xiiVec3Template<Type>& vPoint, bool bPlaneShouldFacePoint)
{
  if ((GetPointPosition(vPoint) == xiiPositionOnPlane::Front) != bPlaneShouldFacePoint)
  {
    Flip();
    return true;
  }

  return false;
}

template <typename Type>
bool xiiPlaneTemplate<Type>::IsValid() const
{
  return !IsNaN() && m_vNormal.IsNormalized(xiiMath::DefaultEpsilon<Type>());
}

template <typename Type>
bool xiiPlaneTemplate<Type>::IsNaN() const
{
  return xiiMath::IsNaN(m_fNegDistance) || m_vNormal.IsNaN();
}

template <typename Type>
bool xiiPlaneTemplate<Type>::IsFinite() const
{
  return m_vNormal.IsValid() && xiiMath::IsFinite(m_fNegDistance);
}

/*! The given vertices can be partially equal or lie on the same line. The algorithm will try to find 3 vertices, that
  form a plane, and deduce the normal from them. This algorithm is much slower, than all the other methods, so only
  use it, when you know, that your data can contain such configurations. */
template <typename Type>
xiiResult xiiPlaneTemplate<Type>::SetFromPoints(const xiiVec3Template<Type>* const pVertices, xiiUInt32 uiMaxVertices)
{
  xiiInt32 iPoints[3];

  if (FindSupportPoints(pVertices, uiMaxVertices, iPoints[0], iPoints[1], iPoints[2]) == XII_FAILURE)
  {
    SetFromPoints(pVertices).IgnoreResult();
    return XII_FAILURE;
  }

  SetFromPoints(pVertices[iPoints[0]], pVertices[iPoints[1]], pVertices[iPoints[2]]).IgnoreResult();
  return XII_SUCCESS;
}

template <typename Type>
xiiResult xiiPlaneTemplate<Type>::FindSupportPoints(const xiiVec3Template<Type>* const pVertices, xiiInt32 iMaxVertices, xiiInt32& out_i1, xiiInt32& out_i2, xiiInt32& out_i3)
{
  const xiiVec3Template<Type> v1 = pVertices[0];

  bool bFoundSecond = false;

  xiiInt32 i = 1;
  while (i < iMaxVertices)
  {
    if (pVertices[i].IsEqual(v1, 0.001f) == false)
    {
      bFoundSecond = true;
      break;
    }

    ++i;
  }

  if (!bFoundSecond)
    return XII_FAILURE;

  const xiiVec3Template<Type> v2 = pVertices[i];

  const xiiVec3Template<Type> vDir1 = (v1 - v2).GetNormalized();

  out_i1 = 0;
  out_i2 = i;

  ++i;

  while (i < iMaxVertices)
  {
    // check for inequality, then for non-collinearity
    if ((pVertices[i].IsEqual(v2, 0.001f) == false) && (xiiMath::Abs((pVertices[i] - v2).GetNormalized().Dot(vDir1)) < (Type)0.999))
    {
      out_i3 = i;
      return XII_SUCCESS;
    }

    ++i;
  }

  return XII_FAILURE;
}

template <typename Type>
xiiPositionOnPlane::Enum xiiPlaneTemplate<Type>::GetObjectPosition(const xiiVec3Template<Type>* const pPoints, xiiUInt32 uiVertices) const
{
  bool bFront = false;
  bool bBack  = false;

  for (xiiUInt32 i = 0; i < uiVertices; ++i)
  {
    switch (GetPointPosition(pPoints[i]))
    {
      case xiiPositionOnPlane::Front:
        if (bBack)
          return (xiiPositionOnPlane::Spanning);
        bFront = true;
        break;
      case xiiPositionOnPlane::Back:
        if (bFront)
          return (xiiPositionOnPlane::Spanning);
        bBack = true;
        break;

      default:
        break;
    }
  }

  return (bFront ? xiiPositionOnPlane::Front : xiiPositionOnPlane::Back);
}

template <typename Type>
xiiPositionOnPlane::Enum xiiPlaneTemplate<Type>::GetObjectPosition(const xiiVec3Template<Type>* const pPoints, xiiUInt32 uiVertices, Type fPlaneHalfWidth) const
{
  bool bFront = false;
  bool bBack  = false;

  for (xiiUInt32 i = 0; i < uiVertices; ++i)
  {
    switch (GetPointPosition(pPoints[i], fPlaneHalfWidth))
    {
      case xiiPositionOnPlane::Front:
        if (bBack)
          return (xiiPositionOnPlane::Spanning);
        bFront = true;
        break;
      case xiiPositionOnPlane::Back:
        if (bFront)
          return (xiiPositionOnPlane::Spanning);
        bBack = true;
        break;

      default:
        break;
    }
  }

  if (bFront)
    return (xiiPositionOnPlane::Front);
  if (bBack)
    return (xiiPositionOnPlane::Back);

  return (xiiPositionOnPlane::OnPlane);
}

template <typename Type>
bool xiiPlaneTemplate<Type>::GetRayIntersection(const xiiVec3Template<Type>& vRayStartPos, const xiiVec3Template<Type>& vRayDir, Type* out_pIntersectionDistance, xiiVec3Template<Type>* out_pIntersection) const
{
  XII_ASSERT_DEBUG(vRayStartPos.IsValid(), "Ray start position must be valid.");
  XII_ASSERT_DEBUG(vRayDir.IsValid(), "Ray direction must be valid.");

  const Type fPlaneSide = GetDistanceTo(vRayStartPos);
  const Type fCosAlpha  = m_vNormal.Dot(vRayDir);

  if (fCosAlpha == 0) // ray is orthogonal to plane
    return false;

  if (xiiMath::Sign(fPlaneSide) == xiiMath::Sign(fCosAlpha)) // ray points away from the plane
    return false;

  const Type fTime = -fPlaneSide / fCosAlpha;

  if (out_pIntersectionDistance)
    *out_pIntersectionDistance = fTime;

  if (out_pIntersection)
    *out_pIntersection = vRayStartPos + fTime * vRayDir;

  return true;
}

template <typename Type>
bool xiiPlaneTemplate<Type>::GetRayIntersectionBiDirectional(const xiiVec3Template<Type>& vRayStartPos, const xiiVec3Template<Type>& vRayDir, Type* out_pIntersectionDistance, xiiVec3Template<Type>* out_pIntersection) const
{
  XII_ASSERT_DEBUG(vRayStartPos.IsValid(), "Ray start position must be valid.");
  XII_ASSERT_DEBUG(vRayDir.IsValid(), "Ray direction must be valid.");

  const Type fPlaneSide = GetDistanceTo(vRayStartPos);
  const Type fCosAlpha  = m_vNormal.Dot(vRayDir);

  if (fCosAlpha == 0) // ray is orthogonal to plane
    return false;

  const Type fTime = -fPlaneSide / fCosAlpha;

  if (out_pIntersectionDistance)
    *out_pIntersectionDistance = fTime;

  if (out_pIntersection)
    *out_pIntersection = vRayStartPos + fTime * vRayDir;

  return true;
}

template <typename Type>
bool xiiPlaneTemplate<Type>::GetLineSegmentIntersection(const xiiVec3Template<Type>& vLineStartPos, const xiiVec3Template<Type>& vLineEndPos, Type* out_pHitFraction, xiiVec3Template<Type>* out_pIntersection) const
{
  Type fTime = 0;

  if (!GetRayIntersection(vLineStartPos, vLineEndPos - vLineStartPos, &fTime, out_pIntersection))
    return false;

  if (out_pHitFraction)
    *out_pHitFraction = fTime;

  return (fTime <= 1);
}

template <typename Type>
Type xiiPlaneTemplate<Type>::GetMinimumDistanceTo(const xiiVec3Template<Type>* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride /* = sizeof (xiiVec3Template<Type>) */) const
{
  XII_ASSERT_DEBUG(pPoints != nullptr, "Array may not be nullptr.");
  XII_ASSERT_DEBUG(uiStride >= sizeof(xiiVec3Template<Type>), "Stride must be at least sizeof(xiiVec3Template) to not have overlapping data.");
  XII_ASSERT_DEBUG(uiNumPoints >= 1, "Array must contain at least one point.");

  Type fMinDist = xiiMath::MaxValue<Type>();

  const xiiVec3Template<Type>* pCurPoint = pPoints;

  for (xiiUInt32 i = 0; i < uiNumPoints; ++i)
  {
    fMinDist = xiiMath::Min(m_vNormal.Dot(*pCurPoint), fMinDist);

    pCurPoint = xiiMemoryUtils::AddByteOffset(pCurPoint, uiStride);
  }

  return fMinDist + m_fNegDistance;
}

template <typename Type>
void xiiPlaneTemplate<Type>::GetMinMaxDistanceTo(Type& out_fMin, Type& out_fMax, const xiiVec3Template<Type>* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride /* = sizeof (xiiVec3Template<Type>) */) const
{
  XII_ASSERT_DEBUG(pPoints != nullptr, "Array may not be nullptr.");
  XII_ASSERT_DEBUG(uiStride >= sizeof(xiiVec3Template<Type>), "Stride must be at least sizeof(xiiVec3Template) to not have overlapping data.");
  XII_ASSERT_DEBUG(uiNumPoints >= 1, "Array must contain at least one point.");

  out_fMin = xiiMath::MaxValue<Type>();
  out_fMax = -xiiMath::MaxValue<Type>();

  const xiiVec3Template<Type>* pCurPoint = pPoints;

  for (xiiUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const Type f = m_vNormal.Dot(*pCurPoint);

    out_fMin = xiiMath::Min(f, out_fMin);
    out_fMax = xiiMath::Max(f, out_fMax);

    pCurPoint = xiiMemoryUtils::AddByteOffset(pCurPoint, uiStride);
  }

  out_fMin += m_fNegDistance;
  out_fMax += m_fNegDistance;
}

template <typename Type>
xiiResult xiiPlaneTemplate<Type>::GetPlanesIntersectionPoint(const xiiPlaneTemplate& p0, const xiiPlaneTemplate& p1, const xiiPlaneTemplate& p2, xiiVec3Template<Type>& out_vResult)
{
  const xiiVec3Template<Type> n1(p0.m_vNormal);
  const xiiVec3Template<Type> n2(p1.m_vNormal);
  const xiiVec3Template<Type> n3(p2.m_vNormal);

  const Type det = n1.Dot(n2.CrossRH(n3));

  if (xiiMath::IsZero<Type>(det, xiiMath::LargeEpsilon<Type>()))
    return XII_FAILURE;

  out_vResult = (-p0.m_fNegDistance * n2.CrossRH(n3) + -p1.m_fNegDistance * n3.CrossRH(n1) + -p2.m_fNegDistance * n1.CrossRH(n2)) / det;

  return XII_SUCCESS;
}

#include <Foundation/Math/Implementation/AllClasses_inl.h>
