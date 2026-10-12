/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/ViewData.h>

xiiResult xiiViewData::ComputePickingRay(float x, float y, xiiVec3& out_vRayStartPos, xiiVec3& out_vRayDir, xiiCameraEye eye) const
{
  if (!xiiMath::IsFinite(x) || !xiiMath::IsFinite(y) || static_cast<xiiUInt32>(eye) >= 2)
    return XII_FAILURE;

  const auto& inverse = m_InverseViewProjectionMatrix[static_cast<xiiUInt32>(eye)];
  xiiVec3     start, interior;
  XII_SUCCEED_OR_RETURN(ComputeWorldSpacePos(x, y, start, eye));
  // An interior depth avoids the point at infinity of an infinite-far projection. Increasing
  // depth points toward the camera under reversed Z, so Foundation's conventional ray helper
  // cannot determine this direction.
  XII_SUCCEED_OR_RETURN(xiiGraphicsUtils::ConvertScreenPosToWorldPos(inverse, xiiVec3(x, y, 0.5f), interior, nullptr, xiiClipSpaceDepthRange::ZeroToOne));
  xiiVec3 direction = interior - start;
  if (!direction.IsValid() || direction.NormalizeIfNotZero().Failed())
    return XII_FAILURE;
  out_vRayStartPos = start;
  out_vRayDir      = direction;
  return XII_SUCCESS;
}

xiiResult xiiViewData::ComputeWorldSpacePos(float x, float y, xiiVec3& out_vWorldPos, xiiCameraEye eye) const
{
  if (!xiiMath::IsFinite(x) || !xiiMath::IsFinite(y) || static_cast<xiiUInt32>(eye) >= 2)
    return XII_FAILURE;
  xiiVec3 position;
  XII_SUCCEED_OR_RETURN(xiiGraphicsUtils::ConvertScreenPosToWorldPos(m_InverseViewProjectionMatrix[static_cast<xiiUInt32>(eye)],
                                                                     xiiVec3(x, y, 1.0f), position, nullptr, xiiClipSpaceDepthRange::ZeroToOne));
  if (!position.IsValid())
    return XII_FAILURE;
  out_vWorldPos = position;
  return XII_SUCCESS;
}
