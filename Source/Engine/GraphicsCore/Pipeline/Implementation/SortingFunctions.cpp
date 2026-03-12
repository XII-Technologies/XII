#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <GraphicsCore/Pipeline/RenderData.h>
#include <GraphicsCore/Pipeline/SortingFunctions.h>

namespace
{
  XII_ALWAYS_INLINE xiiUInt32 CalculateTypeHash(const xiiRenderData* pRenderData)
  {
    xiiUInt32 uiTypeHash = xiiHashingUtils::StringHashTo32(pRenderData->GetDynamicRTTI()->GetTypeNameHash());
    return (uiTypeHash >> 16) ^ (uiTypeHash & 0xFFFFU);
  }

  XII_FORCE_INLINE xiiUInt32 CalculateDistance(const xiiRenderData* pRenderData, const xiiCamera& camera)
  {
    // Use view-space depth (distance along the camera forward direction) and normalize between the camera near and far planes.
    // This is more correct than using the Euclidean distance to the camera position.
    const xiiVec3 vToObject = pRenderData->m_GlobalTransform.m_vPosition - camera.GetPosition();
    const float   fDepth    = vToObject.Dot(camera.GetDirForwards());

    const float fDepthWithOffset = fDepth + pRenderData->m_fSortingDepthOffset;

    const float fNear = camera.GetNearPlane();
    const float fFar  = camera.GetFarPlane();

    const float fNormalizedDistance = xiiMath::Clamp((fDepthWithOffset - fNear) / (fFar - fNear), 0.0f, 1.0f);
    return static_cast<xiiUInt32>(fNormalizedDistance * 65535.0f);
  }
} // namespace

// static
xiiUInt64 xiiRenderSortingFunctions::ByRenderDataThenFrontToBack(const xiiRenderData* pRenderData, const xiiCamera& camera)
{
  const xiiUInt64 uiTypeHash               = CalculateTypeHash(pRenderData);
  const xiiUInt64 uiRenderDataSortingKey64 = pRenderData->m_uiSortingKey;
  const xiiUInt64 uiDistance               = CalculateDistance(pRenderData, camera);

  const xiiUInt64 uiSortingKey = (uiTypeHash << 48) | (uiRenderDataSortingKey64 << 16) | uiDistance;
  return uiSortingKey;
}

// static
xiiUInt64 xiiRenderSortingFunctions::BackToFrontThenByRenderData(const xiiRenderData* pRenderData, const xiiCamera& camera)
{
  const xiiUInt64 uiTypeHash               = CalculateTypeHash(pRenderData);
  const xiiUInt64 uiRenderDataSortingKey64 = pRenderData->m_uiSortingKey;
  const xiiUInt64 uiInvDistance            = 0xFFFFU - CalculateDistance(pRenderData, camera);

  const xiiUInt64 uiSortingKey = (uiInvDistance << 48) | (uiTypeHash << 32) | uiRenderDataSortingKey64;
  return uiSortingKey;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_SortingFunctions);
