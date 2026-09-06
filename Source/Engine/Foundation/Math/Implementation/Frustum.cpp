/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Frustum.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Foundation/Utilities/GraphicsUtils.h>

xiiFrustum::xiiFrustum()  = default;
xiiFrustum::~xiiFrustum() = default;

xiiFrustum xiiFrustum::MakeInvalid()
{
  xiiFrustum frustum;
  for (xiiUInt32 i = 0; i < PLANE_COUNT; ++i)
  {
    frustum.m_Planes[i] = xiiPlane::MakeInvalid();
  }
  return frustum;
}

const xiiPlane& xiiFrustum::GetPlane(xiiUInt8 uiPlane) const
{
  XII_ASSERT_DEBUG(uiPlane < PLANE_COUNT, "Invalid plane index.");

  return m_Planes[uiPlane];
}

xiiPlane& xiiFrustum::AccessPlane(xiiUInt8 uiPlane)
{
  XII_ASSERT_DEBUG(uiPlane < PLANE_COUNT, "Invalid plane index.");

  return m_Planes[uiPlane];
}

bool xiiFrustum::IsValid() const
{
  // For frustums with infinite farplanes we test a finite frustum slice for validity, as the
  // computations below don't work when 4 of the corner points are at infinity.
  if (xiiMath::Abs(m_Planes[FarPlane].m_fNegDistance) == xiiMath::Infinity<float>())
  {
    xiiFrustum finiteSlice                        = *this;
    finiteSlice.m_Planes[FarPlane].m_fNegDistance = -2.f * xiiMath::Abs(m_Planes[NearPlane].m_fNegDistance);
    return finiteSlice.IsValid();
  }

  for (xiiUInt32 i = 0; i < PLANE_COUNT; ++i)
  {
    if (!m_Planes[i].IsValid() || (i != FarPlane && !xiiMath::IsFinite(m_Planes[i].m_fNegDistance)))
      return false;
  }

  xiiVec3 corners[8];
  if (ComputeCornerPoints(corners).Failed())
    return false;

  xiiVec3 center = xiiVec3::MakeZero();
  for (xiiUInt32 i = 0; i < 8; ++i)
  {
    center += corners[i];
  }
  center /= 8.0f;

  if (GetObjectPosition(&center, 1) != xiiVolumePosition::Inside)
    return false;

  return true;
}

xiiFrustum xiiFrustum::MakeFromPlanes(const xiiPlane* pPlanes)
{
  xiiFrustum      frustum;
  const xiiResult res = TryMakeFromPlanes(frustum, pPlanes);
  XII_ASSERT_DEV(res.Succeeded() && frustum.IsValid(), "Frustum is not valid after construction.");
  XII_IGNORE_UNUSED(res);
  return frustum;
}

xiiResult xiiFrustum::TryMakeFromPlanes(xiiFrustum& out_frustum, const xiiPlane* pPlanes)
{
  xiiFrustum f;

  for (xiiUInt32 i = 0; i < PLANE_COUNT; ++i)
    f.m_Planes[i] = pPlanes[i];

  if (f.IsValid())
  {
    out_frustum = std::move(f);
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

void xiiFrustum::TransformFrustum(const xiiMat4& mTransform)
{
  for (xiiUInt32 i = 0; i < PLANE_COUNT; ++i)
  {
    m_Planes[i].Transform(mTransform);
  }
}

xiiFrustum xiiFrustum::GetTransformedFrustum(const xiiMat4& mTransform) const
{
  xiiFrustum result = *this;
  result.TransformFrustum(mTransform);
  return result;
}

xiiVolumePosition::Enum xiiFrustum::GetObjectPosition(const xiiVec3* pVertices, xiiUInt32 uiNumVertices) const
{
  /// \test Not yet tested

  bool bOnSomePlane = false;

  for (xiiUInt32 i = 0; i < PLANE_COUNT; ++i)
  {
    const xiiPositionOnPlane::Enum pos = m_Planes[i].GetObjectPosition(pVertices, uiNumVertices);

    if (pos == xiiPositionOnPlane::Back)
      continue;

    if (pos == xiiPositionOnPlane::Front)
      return xiiVolumePosition::Outside;

    bOnSomePlane = true;
  }

  if (bOnSomePlane)
    return xiiVolumePosition::Intersecting;

  return xiiVolumePosition::Inside;
}

static xiiPositionOnPlane::Enum GetPlaneObjectPosition(const xiiPlane& p, const xiiVec3* const pPoints, xiiUInt32 uiVertices, const xiiMat4& mTransform)
{
  bool bFront = false;
  bool bBack  = false;

  for (xiiUInt32 i = 0; i < uiVertices; ++i)
  {
    switch (p.GetPointPosition(mTransform * pPoints[i]))
    {
      case xiiPositionOnPlane::Front:
      {
        if (bBack)
          return xiiPositionOnPlane::Spanning;

        bFront = true;
      }
      break;

      case xiiPositionOnPlane::Back:
      {
        if (bFront)
          return (xiiPositionOnPlane::Spanning);

        bBack = true;
      }
      break;

      default:
        break;
    }
  }

  return (bFront ? xiiPositionOnPlane::Front : xiiPositionOnPlane::Back);
}


xiiVolumePosition::Enum xiiFrustum::GetObjectPosition(const xiiVec3* pVertices, xiiUInt32 uiNumVertices, const xiiMat4& mObjectTransform) const
{
  /// \test Not yet tested

  bool bOnSomePlane = false;

  for (xiiUInt32 i = 0; i < PLANE_COUNT; ++i)
  {
    const xiiPositionOnPlane::Enum pos = GetPlaneObjectPosition(m_Planes[i], pVertices, uiNumVertices, mObjectTransform);

    if (pos == xiiPositionOnPlane::Back)
      continue;

    if (pos == xiiPositionOnPlane::Front)
      return xiiVolumePosition::Outside;

    bOnSomePlane = true;
  }

  if (bOnSomePlane)
    return xiiVolumePosition::Intersecting;

  return xiiVolumePosition::Inside;
}

xiiVolumePosition::Enum xiiFrustum::GetObjectPosition(const xiiBoundingSphere& sphere) const
{
  /// \test Not yet tested

  bool bOnSomePlane = false;

  for (xiiUInt32 i = 0; i < PLANE_COUNT; ++i)
  {
    const xiiPositionOnPlane::Enum pos = m_Planes[i].GetObjectPosition(sphere);

    if (pos == xiiPositionOnPlane::Back)
      continue;

    if (pos == xiiPositionOnPlane::Front)
      return xiiVolumePosition::Outside;

    bOnSomePlane = true;
  }

  if (bOnSomePlane)
    return xiiVolumePosition::Intersecting;

  return xiiVolumePosition::Inside;
}

xiiVolumePosition::Enum xiiFrustum::GetObjectPosition(const xiiBoundingBox& box) const
{
  /// \test Not yet tested

  bool bOnSomePlane = false;

  for (xiiUInt32 i = 0; i < PLANE_COUNT; ++i)
  {
    const xiiPositionOnPlane::Enum pos = m_Planes[i].GetObjectPosition(box);

    if (pos == xiiPositionOnPlane::Back)
      continue;

    if (pos == xiiPositionOnPlane::Front)
      return xiiVolumePosition::Outside;

    bOnSomePlane = true;
  }

  if (bOnSomePlane)
    return xiiVolumePosition::Intersecting;

  return xiiVolumePosition::Inside;
}

void xiiFrustum::InvertFrustum()
{
  for (xiiUInt32 i = 0; i < PLANE_COUNT; ++i)
    m_Planes[i].Flip();
}

xiiResult xiiFrustum::ComputeCornerPoints(xiiVec3 out_pPoints[FrustumCorner::CORNER_COUNT]) const
{
  XII_SUCCEED_OR_RETURN(xiiPlane::GetPlanesIntersectionPoint(m_Planes[NearPlane], m_Planes[TopPlane], m_Planes[LeftPlane], out_pPoints[FrustumCorner::NearTopLeft]));
  XII_SUCCEED_OR_RETURN(xiiPlane::GetPlanesIntersectionPoint(m_Planes[NearPlane], m_Planes[TopPlane], m_Planes[RightPlane], out_pPoints[FrustumCorner::NearTopRight]));
  XII_SUCCEED_OR_RETURN(xiiPlane::GetPlanesIntersectionPoint(m_Planes[NearPlane], m_Planes[BottomPlane], m_Planes[LeftPlane], out_pPoints[FrustumCorner::NearBottomLeft]));
  XII_SUCCEED_OR_RETURN(xiiPlane::GetPlanesIntersectionPoint(m_Planes[NearPlane], m_Planes[BottomPlane], m_Planes[RightPlane], out_pPoints[FrustumCorner::NearBottomRight]));

  XII_SUCCEED_OR_RETURN(xiiPlane::GetPlanesIntersectionPoint(m_Planes[FarPlane], m_Planes[TopPlane], m_Planes[LeftPlane], out_pPoints[FrustumCorner::FarTopLeft]));
  XII_SUCCEED_OR_RETURN(xiiPlane::GetPlanesIntersectionPoint(m_Planes[FarPlane], m_Planes[TopPlane], m_Planes[RightPlane], out_pPoints[FrustumCorner::FarTopRight]));
  XII_SUCCEED_OR_RETURN(xiiPlane::GetPlanesIntersectionPoint(m_Planes[FarPlane], m_Planes[BottomPlane], m_Planes[LeftPlane], out_pPoints[FrustumCorner::FarBottomLeft]));
  XII_SUCCEED_OR_RETURN(xiiPlane::GetPlanesIntersectionPoint(m_Planes[FarPlane], m_Planes[BottomPlane], m_Planes[RightPlane], out_pPoints[FrustumCorner::FarBottomRight]));

  return XII_SUCCESS;
}

xiiFrustum xiiFrustum::MakeFromMVP(const xiiMat4& mModelViewProjection0, xiiClipSpaceDepthRange::Enum depthRange, xiiHandedness::Enum handedness)
{
  xiiFrustum      frustum;
  const xiiResult res = TryMakeFromMVP(frustum, mModelViewProjection0, depthRange, handedness);
  XII_ASSERT_DEV(res.Succeeded() && frustum.IsValid(), "Frustum is not valid after construction.");
  XII_IGNORE_UNUSED(res);
  return frustum;
}

xiiResult xiiFrustum::TryMakeFromMVP(xiiFrustum& out_frustum, const xiiMat4& mModelViewProjection0, xiiClipSpaceDepthRange::Enum depthRange, xiiHandedness::Enum handedness)
{
  xiiMat4 ModelViewProjection = mModelViewProjection0;
  xiiGraphicsUtils::ConvertProjectionMatrixDepthRange(ModelViewProjection, depthRange, xiiClipSpaceDepthRange::MinusOneToOne);

  xiiVec4 planes[6];

  if (handedness == xiiHandedness::LeftHanded)
  {
    ModelViewProjection.SetRow(0, -ModelViewProjection.GetRow(0));
  }

  planes[LeftPlane]   = -ModelViewProjection.GetRow(3) - ModelViewProjection.GetRow(0);
  planes[RightPlane]  = -ModelViewProjection.GetRow(3) + ModelViewProjection.GetRow(0);
  planes[BottomPlane] = -ModelViewProjection.GetRow(3) - ModelViewProjection.GetRow(1);
  planes[TopPlane]    = -ModelViewProjection.GetRow(3) + ModelViewProjection.GetRow(1);
  planes[NearPlane]   = -ModelViewProjection.GetRow(3) - ModelViewProjection.GetRow(2);
  planes[FarPlane]    = -ModelViewProjection.GetRow(3) + ModelViewProjection.GetRow(2);

  // Normalize planes
  for (xiiInt32 p = 0; p < 6; ++p)
  {
    const float len = planes[p].GetAsVec3().GetLength();
    // doing the division here manually since we want to accept the case where length is 0 (infinite plane)
    const float invLen = 1.f / len;
    planes[p].x *= xiiMath::IsFinite(invLen) ? invLen : 0.f;
    planes[p].y *= xiiMath::IsFinite(invLen) ? invLen : 0.f;
    planes[p].z *= xiiMath::IsFinite(invLen) ? invLen : 0.f;
    planes[p].w *= invLen;
  }

  // The last matrix row is giving the camera's plane, which means its normal is
  // also the camera's viewing direction.
  const xiiVec3 cameraViewDirection = ModelViewProjection.GetRow(3).GetAsVec3();

  // Making sure the near/far plane is always closest/farthest. The way we derive the
  // planes always yields the closer plane pointing towards the camera and the farther
  // plane pointing away from the camera, so flip when that relationship inverts.
  if (planes[FarPlane].GetAsVec3().Dot(cameraViewDirection) < 0)
  {
    XII_ASSERT_DEBUG(planes[NearPlane].GetAsVec3().Dot(cameraViewDirection) >= 0, "");
    xiiMath::Swap(planes[NearPlane], planes[FarPlane]);
  }

  // In case we have an infinity far plane projection, the normal is invalid.
  // We'll just take the mirrored normal from the near plane.
  XII_ASSERT_DEBUG(planes[NearPlane].IsValid(), "Near plane is expected to be non-nan and finite at this point!");
  if (xiiMath::Abs(planes[FarPlane].w) == xiiMath::Infinity<float>())
  {
    planes[FarPlane] = (-planes[NearPlane].GetAsVec3()).GetAsVec4(planes[FarPlane].w);
  }

  static_assert(sizeof(xiiFrustum) == sizeof(planes));
  if (reinterpret_cast<xiiFrustum*>(planes)->IsValid())
  {
    static_assert(offsetof(xiiPlane, m_vNormal) == offsetof(xiiVec4, x) && offsetof(xiiPlane, m_fNegDistance) == offsetof(xiiVec4, w));
    xiiMemoryUtils::Copy(out_frustum.m_Planes, (xiiPlane*)planes, 6);
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiFrustum xiiFrustum::MakeFromFOV(const xiiVec3& vPosition, const xiiVec3& vForwards, const xiiVec3& vUp, xiiAngle fovX, xiiAngle fovY, float fNearPlane, float fFarPlane)
{
  xiiFrustum      frustum;
  const xiiResult res = TryMakeFromFOV(frustum, vPosition, vForwards, vUp, fovX, fovY, fNearPlane, fFarPlane);
  XII_ASSERT_DEV(res.Succeeded() && frustum.IsValid(), "Frustum is not valid after construction.");
  XII_IGNORE_UNUSED(res);
  return frustum;
}

xiiResult xiiFrustum::TryMakeFromFOV(xiiFrustum& out_frustum, const xiiVec3& vPosition, const xiiVec3& vForwards, const xiiVec3& vUp, xiiAngle fovX, xiiAngle fovY, float fNearPlane, float fFarPlane)
{
  XII_ASSERT_DEBUG(xiiMath::Abs(vForwards.GetNormalized().Dot(vUp.GetNormalized())) < 0.999f, "Up dir must be different from forward direction");

  const xiiVec3 vForwardsNorm = vForwards.GetNormalized();
  const xiiVec3 vRightNorm    = vForwards.CrossRH(vUp).GetNormalized();
  const xiiVec3 vUpNorm       = vRightNorm.CrossRH(vForwards).GetNormalized();

  xiiFrustum res;

  // Near Plane
  res.m_Planes[NearPlane] = xiiPlane::MakeFromNormalAndPoint(-vForwardsNorm, vPosition + fNearPlane * vForwardsNorm);

  // Far Plane
  res.m_Planes[FarPlane] = xiiPlane::MakeFromNormalAndPoint(vForwardsNorm, vPosition + fFarPlane * vForwardsNorm);

  // Making sure the near/far plane is always closest/farthest.
  if (fNearPlane > fFarPlane)
  {
    xiiMath::Swap(res.m_Planes[NearPlane], res.m_Planes[FarPlane]);
  }

  xiiMat3 mLocalFrame;
  mLocalFrame.SetColumn(0, vRightNorm);
  mLocalFrame.SetColumn(1, vUpNorm);
  mLocalFrame.SetColumn(2, -vForwardsNorm);

  const float fCosFovX = xiiMath::Cos(fovX * 0.5f);
  const float fSinFovX = xiiMath::Sin(fovX * 0.5f);

  const float fCosFovY = xiiMath::Cos(fovY * 0.5f);
  const float fSinFovY = xiiMath::Sin(fovY * 0.5f);

  // Left Plane
  {
    xiiVec3 vPlaneNormal = mLocalFrame * xiiVec3(-fCosFovX, 0, fSinFovX);
    vPlaneNormal.Normalize();

    res.m_Planes[LeftPlane] = xiiPlane::MakeFromNormalAndPoint(vPlaneNormal, vPosition);
  }

  // Right Plane
  {
    xiiVec3 vPlaneNormal = mLocalFrame * xiiVec3(fCosFovX, 0, fSinFovX);
    vPlaneNormal.Normalize();

    res.m_Planes[RightPlane] = xiiPlane::MakeFromNormalAndPoint(vPlaneNormal, vPosition);
  }

  // Bottom Plane
  {
    xiiVec3 vPlaneNormal = mLocalFrame * xiiVec3(0, -fCosFovY, fSinFovY);
    vPlaneNormal.Normalize();

    res.m_Planes[BottomPlane] = xiiPlane::MakeFromNormalAndPoint(vPlaneNormal, vPosition);
  }

  // Top Plane
  {
    xiiVec3 vPlaneNormal = mLocalFrame * xiiVec3(0, fCosFovY, fSinFovY);
    vPlaneNormal.Normalize();

    res.m_Planes[TopPlane] = xiiPlane::MakeFromNormalAndPoint(vPlaneNormal, vPosition);
  }

  if (res.IsValid())
  {
    out_frustum = std::move(res);
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiFrustum xiiFrustum::MakeFromCorners(const xiiVec3 pCorners[FrustumCorner::CORNER_COUNT])
{
  xiiFrustum      frustum;
  const xiiResult res = TryMakeFromCorners(frustum, pCorners);
  XII_ASSERT_DEV(res.Succeeded() && frustum.IsValid(), "Frustum is not valid after construction.");
  XII_IGNORE_UNUSED(res);
  return frustum;
}

xiiResult xiiFrustum::TryMakeFromCorners(xiiFrustum& out_frustum, const xiiVec3 pCorners[FrustumCorner::CORNER_COUNT])
{
  xiiFrustum res;

  res.m_Planes[PlaneType::LeftPlane]   = xiiPlane::MakeFromPoints(pCorners[FrustumCorner::FarTopLeft], pCorners[FrustumCorner::NearBottomLeft], pCorners[FrustumCorner::NearTopLeft]);
  res.m_Planes[PlaneType::RightPlane]  = xiiPlane::MakeFromPoints(pCorners[FrustumCorner::NearTopRight], pCorners[FrustumCorner::FarBottomRight], pCorners[FrustumCorner::FarTopRight]);
  res.m_Planes[PlaneType::BottomPlane] = xiiPlane::MakeFromPoints(pCorners[FrustumCorner::NearBottomLeft], pCorners[FrustumCorner::FarBottomRight], pCorners[FrustumCorner::NearBottomRight]);
  res.m_Planes[PlaneType::TopPlane]    = xiiPlane::MakeFromPoints(pCorners[FrustumCorner::FarTopLeft], pCorners[FrustumCorner::NearTopRight], pCorners[FrustumCorner::FarTopRight]);
  res.m_Planes[PlaneType::FarPlane]    = xiiPlane::MakeFromPoints(pCorners[FrustumCorner::FarTopLeft], pCorners[FrustumCorner::FarBottomRight], pCorners[FrustumCorner::FarBottomLeft]);
  res.m_Planes[PlaneType::NearPlane]   = xiiPlane::MakeFromPoints(pCorners[FrustumCorner::NearTopLeft], pCorners[FrustumCorner::NearBottomRight], pCorners[FrustumCorner::NearTopRight]);

  if (res.IsValid())
  {
    out_frustum = std::move(res);
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}
