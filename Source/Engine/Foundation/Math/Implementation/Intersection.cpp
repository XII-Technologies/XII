#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Intersection.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Math/Plane.h>

bool xiiIntersectionUtils::RayPolygonIntersection(const xiiVec3& vRayStartPos, const xiiVec3& vRayDir, const xiiVec3* pPolygonVertices, xiiUInt32 uiNumVertices, float* out_fIntersectionTime, xiiVec3* out_vIntersectionPoint, xiiUInt32 uiVertexStride)
{
  XII_ASSERT_DEBUG(uiNumVertices >= 3, "A polygon must have at least three vertices.");
  XII_ASSERT_DEBUG(uiVertexStride >= sizeof(xiiVec3), "The vertex stride is invalid.");

  xiiPlane p(*pPolygonVertices, *xiiMemoryUtils::AddByteOffset(pPolygonVertices, uiVertexStride),
             *xiiMemoryUtils::AddByteOffset(pPolygonVertices, uiVertexStride * 2));

  XII_ASSERT_DEBUG(p.IsValid(), "The given polygon's plane is invalid (computed from the first three vertices only).");

  xiiVec3 vIntersection;

  if (!p.GetRayIntersection(vRayStartPos, vRayDir, out_fIntersectionTime, &vIntersection))
    return false;

  if (out_vIntersectionPoint)
    *out_vIntersectionPoint = vIntersection;

  // start with the last point as the 'wrap around' position
  xiiVec3 vPrevPoint = *xiiMemoryUtils::AddByteOffset(pPolygonVertices, xiiMath::SafeMultiply32(uiVertexStride, (uiNumVertices - 1)));

  // for each polygon edge
  for (xiiUInt32 i = 0; i < uiNumVertices; ++i)
  {
    const xiiVec3 vThisPoint = *xiiMemoryUtils::AddByteOffset(pPolygonVertices, xiiMath::SafeMultiply32(uiVertexStride, i));

    const xiiPlane EdgePlane(vThisPoint, vPrevPoint, vPrevPoint + p.m_vNormal);

    // if the intersection point is outside of any of the edge planes, it is not inside the (convex) polygon
    if (EdgePlane.GetPointPosition(vIntersection) == xiiPositionOnPlane::Back)
      return false;

    vPrevPoint = vThisPoint;
  }

  // inside all edge planes -> inside the polygon -> there is a proper intersection
  return true;
}

bool xiiIntersectionUtils::RayPolygonIntersection(const xiiVec3d& vRayStartPos, const xiiVec3d& vRayDir, const xiiVec3d* pPolygonVertices, xiiUInt32 uiNumVertices, double* out_fIntersectionTime, xiiVec3d* out_vIntersectionPoint, xiiUInt32 uiVertexStride)
{
  XII_ASSERT_DEBUG(uiNumVertices >= 3, "A polygon must have at least three vertices.");
  XII_ASSERT_DEBUG(uiVertexStride >= sizeof(xiiVec3d), "The vertex stride is invalid.");

  xiiPlaned p(*pPolygonVertices, *xiiMemoryUtils::AddByteOffset(pPolygonVertices, uiVertexStride),
              *xiiMemoryUtils::AddByteOffset(pPolygonVertices, uiVertexStride * 2));

  XII_ASSERT_DEBUG(p.IsValid(), "The given polygon's plane is invalid (computed from the first three vertices only).");

  xiiVec3d vIntersection;

  if (!p.GetRayIntersection(vRayStartPos, vRayDir, out_fIntersectionTime, &vIntersection))
    return false;

  if (out_vIntersectionPoint)
    *out_vIntersectionPoint = vIntersection;

  // start with the last point as the 'wrap around' position
  xiiVec3d vPrevPoint = *xiiMemoryUtils::AddByteOffset(pPolygonVertices, xiiMath::SafeMultiply32(uiVertexStride, (uiNumVertices - 1)));

  // for each polygon edge
  for (xiiUInt32 i = 0; i < uiNumVertices; ++i)
  {
    const xiiVec3d vThisPoint = *xiiMemoryUtils::AddByteOffset(pPolygonVertices, xiiMath::SafeMultiply32(uiVertexStride, i));

    const xiiPlaned EdgePlane(vThisPoint, vPrevPoint, vPrevPoint + p.m_vNormal);

    // if the intersection point is outside of any of the edge planes, it is not inside the (convex) polygon
    if (EdgePlane.GetPointPosition(vIntersection) == xiiPositionOnPlane::Back)
      return false;

    vPrevPoint = vThisPoint;
  }

  // inside all edge planes -> inside the polygon -> there is a proper intersection
  return true;
}

xiiVec3 xiiIntersectionUtils::ClosestPoint_PointLineSegment(
  const xiiVec3& vStartPoint,
  const xiiVec3& vLineSegmentPos0,
  const xiiVec3& vLineSegmentPos1,
  float*         out_fFractionAlongSegment)
{
  const xiiVec3 vLineDir      = vLineSegmentPos1 - vLineSegmentPos0;
  const xiiVec3 vToStartPoint = vStartPoint - vLineSegmentPos0;

  const float fProjected = vToStartPoint.Dot(vLineDir);

  float fPosAlongSegment;

  // clamp t to [0; 1] range, and only do the division etc. when necessary
  if (fProjected <= 0.0f)
  {
    fPosAlongSegment = 0.0f;
  }
  else
  {
    const float fSquaredDirLen = vLineDir.GetLengthSquared();

    if (fProjected >= fSquaredDirLen)
    {
      fPosAlongSegment = 1.0f;
    }
    else
    {
      fPosAlongSegment = fProjected / fSquaredDirLen;
    }
  }

  if (out_fFractionAlongSegment)
    *out_fFractionAlongSegment = fPosAlongSegment;

  return vLineSegmentPos0 + fPosAlongSegment * vLineDir;
}

xiiVec3d xiiIntersectionUtils::ClosestPoint_PointLineSegment(
  const xiiVec3d& vStartPoint,
  const xiiVec3d& vLineSegmentPos0,
  const xiiVec3d& vLineSegmentPos1,
  double*         out_fFractionAlongSegment)
{
  const xiiVec3d vLineDir      = vLineSegmentPos1 - vLineSegmentPos0;
  const xiiVec3d vToStartPoint = vStartPoint - vLineSegmentPos0;

  const double fProjected = vToStartPoint.Dot(vLineDir);

  double fPosAlongSegment;

  // clamp t to [0; 1] range, and only do the division etc. when necessary
  if (fProjected <= 0.0)
  {
    fPosAlongSegment = 0.0;
  }
  else
  {
    const double fSquaredDirLen = vLineDir.GetLengthSquared();

    if (fProjected >= fSquaredDirLen)
    {
      fPosAlongSegment = 1.0;
    }
    else
    {
      fPosAlongSegment = fProjected / fSquaredDirLen;
    }
  }

  if (out_fFractionAlongSegment)
    *out_fFractionAlongSegment = fPosAlongSegment;

  return vLineSegmentPos0 + fPosAlongSegment * vLineDir;
}

bool xiiIntersectionUtils::Ray2DLine2D(const xiiVec2& vRayStartPos, const xiiVec2& vRayDir, const xiiVec2& vLineSegmentPos0, const xiiVec2& vLineSegmentPos1, float* out_fIntersectionTime, xiiVec2* out_vIntersectionPoint)
{
  const xiiVec2 vLineDir = vLineSegmentPos1 - vLineSegmentPos0;

  // 2D Plane
  const xiiVec2 vPlaneNormal  = vLineDir.GetOrthogonalVector();
  const float   fPlaneNegDist = -vPlaneNormal.Dot(vLineSegmentPos0);

  xiiVec2 vIntersection;
  float   fIntersectionTime;

  // 2D Plane ray intersection test
  {
    const float fPlaneSide = vPlaneNormal.Dot(vRayStartPos) + fPlaneNegDist;
    const float fCosAlpha  = vPlaneNormal.Dot(vRayDir);

    if (fCosAlpha == 0) // ray is orthogonal to plane
      return false;

    if (xiiMath::Sign(fPlaneSide) == xiiMath::Sign(fCosAlpha)) // ray points away from the plane
      return false;

    fIntersectionTime = -fPlaneSide / fCosAlpha;

    vIntersection = vRayStartPos + fIntersectionTime * vRayDir;
  }

  const xiiVec2 vToIntersection = vIntersection - vLineSegmentPos0;

  const float fProjected = vLineDir.Dot(vToIntersection);

  if (fProjected < 0.0f)
    return false;

  if (fProjected > vLineDir.GetLengthSquared())
    return false;

  if (out_fIntersectionTime)
    *out_fIntersectionTime = fIntersectionTime;

  if (out_vIntersectionPoint)
    *out_vIntersectionPoint = vIntersection;

  return true;
}

bool xiiIntersectionUtils::Ray2DLine2D(const xiiVec2d& vRayStartPos, const xiiVec2d& vRayDir, const xiiVec2d& vLineSegmentPos0, const xiiVec2d& vLineSegmentPos1, double* out_fIntersectionTime, xiiVec2d* out_vIntersectionPoint)
{
  const xiiVec2d vLineDir = vLineSegmentPos1 - vLineSegmentPos0;

  // 2D Plane
  const xiiVec2d vPlaneNormal  = vLineDir.GetOrthogonalVector();
  const double   fPlaneNegDist = -vPlaneNormal.Dot(vLineSegmentPos0);

  xiiVec2d vIntersection;
  double   fIntersectionTime;

  // 2D Plane ray intersection test
  {
    const double fPlaneSide = vPlaneNormal.Dot(vRayStartPos) + fPlaneNegDist;
    const double fCosAlpha  = vPlaneNormal.Dot(vRayDir);

    if (fCosAlpha == 0) // ray is orthogonal to plane
      return false;

    if (xiiMath::Sign(fPlaneSide) == xiiMath::Sign(fCosAlpha)) // ray points away from the plane
      return false;

    fIntersectionTime = -fPlaneSide / fCosAlpha;

    vIntersection = vRayStartPos + fIntersectionTime * vRayDir;
  }

  const xiiVec2d vToIntersection = vIntersection - vLineSegmentPos0;

  const double fProjected = vLineDir.Dot(vToIntersection);

  if (fProjected < 0.0)
    return false;

  if (fProjected > vLineDir.GetLengthSquared())
    return false;

  if (out_fIntersectionTime)
    *out_fIntersectionTime = fIntersectionTime;

  if (out_vIntersectionPoint)
    *out_vIntersectionPoint = vIntersection;

  return true;
}

bool xiiIntersectionUtils::IsPointOnLine(const xiiVec3& vLineStart, const xiiVec3& vLineEnd, const xiiVec3& vPoint, float fMaxDist /*= 0.01f*/)
{
  const xiiVec3 vClosest        = ClosestPoint_PointLineSegment(vPoint, vLineStart, vLineEnd);
  const float   fClosestDistSqr = (vClosest - vPoint).GetLengthSquared();

  return (fClosestDistSqr <= fMaxDist * fMaxDist);
}

bool xiiIntersectionUtils::IsPointOnLine(const xiiVec3d& vLineStart, const xiiVec3d& vLineEnd, const xiiVec3d& vPoint, double fMaxDist /*= 0.001f*/)
{
  const xiiVec3d vClosest        = ClosestPoint_PointLineSegment(vPoint, vLineStart, vLineEnd);
  const double   fClosestDistSqr = (vClosest - vPoint).GetLengthSquared();

  return (fClosestDistSqr <= fMaxDist * fMaxDist);
}

XII_STATICLINK_FILE(Foundation, Foundation_Math_Implementation_Intersection);
