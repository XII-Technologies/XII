/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GraphicsCore/Lighting/ShadowCascade.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiShadowCascadeSettings, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiShadowCascadeSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("CascadeCount", m_uiCascadeCount)->AddAttributes(new xiiClampValueAttribute(1U, 4U), new xiiDefaultValueAttribute(4U)),
    XII_MEMBER_PROPERTY("SplitLambda", m_fSplitLambda)->AddAttributes(new xiiClampValueAttribute(0.0f, 1.0f), new xiiDefaultValueAttribute(0.65f)),
    XII_MEMBER_PROPERTY("MaximumShadowDistance", m_fMaximumShadowDistance)->AddAttributes(new xiiClampValueAttribute(1.0f, xiiVariant()), new xiiSuffixAttribute(" m")),
    XII_MEMBER_PROPERTY("DepthPadding", m_fDepthPadding)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiSuffixAttribute(" m")),
    XII_MEMBER_PROPERTY("ShadowMapResolution", m_uiShadowMapResolution)->AddAttributes(new xiiClampValueAttribute(64U, 16384U), new xiiDefaultValueAttribute(4096U)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  static void AddFrustumPlaneCorners(const xiiVec3& vCenter, const xiiVec3& vRight, const xiiVec3& vUp, float fHalfWidth, float fHalfHeight, xiiVec3* pCorners)
  {
    pCorners[0] = vCenter - vRight * fHalfWidth - vUp * fHalfHeight;
    pCorners[1] = vCenter + vRight * fHalfWidth - vUp * fHalfHeight;
    pCorners[2] = vCenter - vRight * fHalfWidth + vUp * fHalfHeight;
    pCorners[3] = vCenter + vRight * fHalfWidth + vUp * fHalfHeight;
  }
} // namespace

xiiResult xiiShadowCascadeUtils::Build(const xiiCamera& camera, float fAspectRatio, const xiiVec3& vLightDirection,
  const xiiShadowCascadeSettings& settings, xiiStaticArray<xiiShadowCascadeDescription, 4>& out_cascades)
{
  out_cascades.Clear();

  const xiiUInt32 uiCascadeCount = xiiMath::Clamp(settings.m_uiCascadeCount, 1U, 4U);
  const float fNearPlane = xiiMath::Max(camera.GetNearPlane(), 0.001f);
  const float fFarPlane = xiiMath::Min(camera.GetFarPlane(), xiiMath::Max(settings.m_fMaximumShadowDistance, fNearPlane + 0.001f));
  if (!xiiMath::IsFinite(fFarPlane) || fFarPlane <= fNearPlane || fAspectRatio <= 0.0f)
    return XII_FAILURE;

  xiiVec3 vSunDirection = vLightDirection;
  if (vSunDirection.NormalizeIfNotZero(xiiVec3(0.0f, 0.0f, -1.0f)).Failed())
    return XII_FAILURE;

  const xiiVec3 vCameraPosition = camera.GetPosition();
  const xiiVec3 vCameraForward  = camera.GetDirForwards().GetNormalized();
  const xiiVec3 vCameraRight    = camera.GetDirRight().GetNormalized();
  const xiiVec3 vCameraUp       = camera.GetDirUp().GetNormalized();
  const float fSplitLambda      = xiiMath::Clamp(settings.m_fSplitLambda, 0.0f, 1.0f);
  const float fDepthPadding     = xiiMath::Max(settings.m_fDepthPadding, 0.0f);
  const float fResolution       = static_cast<float>(xiiMath::Max(settings.m_uiShadowMapResolution, 1U));

  float fPreviousSplit = fNearPlane;
  for (xiiUInt32 uiCascade = 0U; uiCascade < uiCascadeCount; ++uiCascade)
  {
    const float fT = static_cast<float>(uiCascade + 1U) / static_cast<float>(uiCascadeCount);
    const float fUniformSplit = fNearPlane + (fFarPlane - fNearPlane) * fT;
    const float fLogSplit = fNearPlane * xiiMath::Pow(fFarPlane / fNearPlane, fT);
    const float fSplitFar = xiiMath::Lerp(fUniformSplit, fLogSplit, fSplitLambda);

    xiiVec3 vCorners[8];
    if (camera.IsOrthographic())
    {
      const float fHalfWidth  = camera.GetDimensionX(fAspectRatio) * 0.5f;
      const float fHalfHeight = camera.GetDimensionY(fAspectRatio) * 0.5f;
      AddFrustumPlaneCorners(vCameraPosition + vCameraForward * fPreviousSplit, vCameraRight, vCameraUp, fHalfWidth, fHalfHeight, &vCorners[0]);
      AddFrustumPlaneCorners(vCameraPosition + vCameraForward * fSplitFar, vCameraRight, vCameraUp, fHalfWidth, fHalfHeight, &vCorners[4]);
    }
    else
    {
      const float fTanHalfFovY = xiiMath::Tan(camera.GetFovY(fAspectRatio) * 0.5f);
      const float fTanHalfFovX = xiiMath::Tan(camera.GetFovX(fAspectRatio) * 0.5f);
      AddFrustumPlaneCorners(vCameraPosition + vCameraForward * fPreviousSplit, vCameraRight, vCameraUp, fPreviousSplit * fTanHalfFovX, fPreviousSplit * fTanHalfFovY, &vCorners[0]);
      AddFrustumPlaneCorners(vCameraPosition + vCameraForward * fSplitFar, vCameraRight, vCameraUp, fSplitFar * fTanHalfFovX, fSplitFar * fTanHalfFovY, &vCorners[4]);
    }

    xiiVec3 vCascadeCenter = xiiVec3::MakeZero();
    for (const xiiVec3& vCorner : vCorners)
      vCascadeCenter += vCorner;
    vCascadeCenter /= 8.0f;

    float fRadius = 0.0f;
    for (const xiiVec3& vCorner : vCorners)
      fRadius = xiiMath::Max(fRadius, (vCorner - vCascadeCenter).GetLength());
    fRadius = xiiMath::Ceil(fRadius * 16.0f) / 16.0f;

    xiiVec3 vLightUp = vCameraUp;
    if (xiiMath::Abs(vLightUp.Dot(vSunDirection)) > 0.95f)
      vLightUp = vCameraRight;

    const xiiMat3 mLightRotation = xiiGraphicsUtils::CreateLookAtViewMatrix(vSunDirection, vLightUp, xiiHandedness::LeftHanded);
    const xiiVec3 vLightSpaceCenter = mLightRotation * vCascadeCenter;
    const float fWorldUnitsPerTexel = (2.0f * fRadius) / fResolution;
    const float fSnappedX = xiiMath::Floor(vLightSpaceCenter.x / fWorldUnitsPerTexel) * fWorldUnitsPerTexel;
    const float fSnappedY = xiiMath::Floor(vLightSpaceCenter.y / fWorldUnitsPerTexel) * fWorldUnitsPerTexel;
    vCascadeCenter += mLightRotation.GetRow(0) * (fSnappedX - vLightSpaceCenter.x);
    vCascadeCenter += mLightRotation.GetRow(1) * (fSnappedY - vLightSpaceCenter.y);

    const xiiVec3 vLightEye = vCascadeCenter - vSunDirection * (fRadius + fDepthPadding);
    const xiiMat4 mLightView = xiiGraphicsUtils::CreateLookAtViewMatrix(vLightEye, vCascadeCenter, vLightUp, xiiHandedness::LeftHanded);

    float fMinDepth = xiiMath::MaxValue<float>();
    float fMaxDepth = -xiiMath::MaxValue<float>();
    for (const xiiVec3& vCorner : vCorners)
    {
      const float fDepth = mLightView.TransformPosition(vCorner).z;
      fMinDepth = xiiMath::Min(fMinDepth, fDepth);
      fMaxDepth = xiiMath::Max(fMaxDepth, fDepth);
    }

    // Reversed-Z projection matches the engine depth convention and a clear value of zero.
    const float fNearDepth = fMaxDepth + fDepthPadding;
    const float fFarDepth  = xiiMath::Max(fMinDepth - fDepthPadding, 0.001f);
    const xiiMat4 mLightProjection = xiiGraphicsUtils::CreateOrthographicProjectionMatrix(-fRadius, fRadius, -fRadius, fRadius,
      fNearDepth, fFarDepth, xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded);

    xiiShadowCascadeDescription& cascade = out_cascades.ExpandAndGetRef();
    cascade.m_mViewProjection = mLightProjection * mLightView;
    cascade.m_fSplitNear      = fPreviousSplit;
    cascade.m_fSplitFar       = fSplitFar;
    fPreviousSplit            = fSplitFar;
  }

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_ShadowCascade);
