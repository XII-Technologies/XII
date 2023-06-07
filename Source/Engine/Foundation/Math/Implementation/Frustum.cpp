#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Frustum.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Foundation/Utilities/GraphicsUtils.h>

xiiFrustum::xiiFrustum()  = default;
xiiFrustum::~xiiFrustum() = default;

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
  for (xiiUInt32 i = 0; i < PLANE_COUNT; ++i)
  {
    if (!m_Planes[i].IsValid())
      return false;
  }

  return true;
}

void xiiFrustum::SetFrustum(const xiiPlane* pPlanes)
{
  for (xiiUInt32 i = 0; i < PLANE_COUNT; ++i)
    m_Planes[i] = pPlanes[i];
}

void xiiFrustum::TransformFrustum(const xiiMat4& mTransform)
{
  for (xiiUInt32 i = 0; i < PLANE_COUNT; ++i)
    m_Planes[i].Transform(mTransform);
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

void xiiFrustum::ComputeCornerPoints(xiiVec3 out_pPoints[FrustumCorner::CORNER_COUNT]) const
{
  // clang-format off
  xiiPlane::GetPlanesIntersectionPoint(m_Planes[NearPlane], m_Planes[TopPlane], m_Planes[LeftPlane], out_pPoints[FrustumCorner::NearTopLeft]).IgnoreResult();
  xiiPlane::GetPlanesIntersectionPoint(m_Planes[NearPlane], m_Planes[TopPlane], m_Planes[RightPlane], out_pPoints[FrustumCorner::NearTopRight]).IgnoreResult();
  xiiPlane::GetPlanesIntersectionPoint(m_Planes[NearPlane], m_Planes[BottomPlane], m_Planes[LeftPlane], out_pPoints[FrustumCorner::NearBottomLeft]).IgnoreResult();
  xiiPlane::GetPlanesIntersectionPoint(m_Planes[NearPlane], m_Planes[BottomPlane], m_Planes[RightPlane], out_pPoints[FrustumCorner::NearBottomRight]).IgnoreResult();

  xiiPlane::GetPlanesIntersectionPoint(m_Planes[FarPlane], m_Planes[TopPlane], m_Planes[LeftPlane], out_pPoints[FrustumCorner::FarTopLeft]).IgnoreResult();
  xiiPlane::GetPlanesIntersectionPoint(m_Planes[FarPlane], m_Planes[TopPlane], m_Planes[RightPlane], out_pPoints[FrustumCorner::FarTopRight]).IgnoreResult();
  xiiPlane::GetPlanesIntersectionPoint(m_Planes[FarPlane], m_Planes[BottomPlane], m_Planes[LeftPlane], out_pPoints[FrustumCorner::FarBottomLeft]).IgnoreResult();
  xiiPlane::GetPlanesIntersectionPoint(m_Planes[FarPlane], m_Planes[BottomPlane], m_Planes[RightPlane], out_pPoints[FrustumCorner::FarBottomRight]).IgnoreResult();
  // clang-format on
}

void xiiFrustum::SetFrustum(const xiiMat4& mModelViewProjection0, xiiClipSpaceDepthRange::Enum depthRange, xiiHandedness::Enum handedness)
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
  for (int p = 0; p < 6; ++p)
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

  static_assert(offsetof(xiiPlane, m_vNormal) == offsetof(xiiVec4, x) && offsetof(xiiPlane, m_fNegDistance) == offsetof(xiiVec4, w));
  xiiMemoryUtils::Copy(m_Planes, (xiiPlane*)planes, 6);
}

void xiiFrustum::SetFrustum(const xiiVec3& vPosition, const xiiVec3& vForwards, const xiiVec3& vUp, xiiAngle fovX, xiiAngle fovY, float fNearPlane, float fFarPlane)
{
  XII_ASSERT_DEBUG(xiiMath::Abs(vForwards.GetNormalized().Dot(vUp.GetNormalized())) < 0.999f, "Up dir must be different from forward direction");

  const xiiVec3 vForwardsNorm = vForwards.GetNormalized();
  const xiiVec3 vRightNorm    = vForwards.CrossRH(vUp).GetNormalized();
  const xiiVec3 vUpNorm       = vRightNorm.CrossRH(vForwards).GetNormalized();

  // Near Plane
  m_Planes[NearPlane].SetFromNormalAndPoint(-vForwardsNorm, vPosition + fNearPlane * vForwardsNorm);

  // Far Plane
  m_Planes[FarPlane].SetFromNormalAndPoint(vForwardsNorm, vPosition + fFarPlane * vForwardsNorm);

  // Making sure the near/far plane is always closest/farthest.
  if (fNearPlane > fFarPlane)
  {
    xiiMath::Swap(m_Planes[NearPlane], m_Planes[FarPlane]);
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

    m_Planes[LeftPlane].SetFromNormalAndPoint(vPlaneNormal, vPosition);
  }

  // Right Plane
  {
    xiiVec3 vPlaneNormal = mLocalFrame * xiiVec3(fCosFovX, 0, fSinFovX);
    vPlaneNormal.Normalize();

    m_Planes[RightPlane].SetFromNormalAndPoint(vPlaneNormal, vPosition);
  }

  // Bottom Plane
  {
    xiiVec3 vPlaneNormal = mLocalFrame * xiiVec3(0, -fCosFovY, fSinFovY);
    vPlaneNormal.Normalize();

    m_Planes[BottomPlane].SetFromNormalAndPoint(vPlaneNormal, vPosition);
  }

  // Top Plane
  {
    xiiVec3 vPlaneNormal = mLocalFrame * xiiVec3(0, fCosFovY, fSinFovY);
    vPlaneNormal.Normalize();

    m_Planes[TopPlane].SetFromNormalAndPoint(vPlaneNormal, vPosition);
  }
}

XII_STATICLINK_FILE(Foundation, Foundation_Math_Implementation_Frustum);
