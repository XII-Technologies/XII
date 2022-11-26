#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/SortingFunctions.h>

namespace
{
  XII_ALWAYS_INLINE xiiUInt32 CalculateTypeHash(const xiiRenderData* pRenderData)
  {
    xiiUInt32 uiTypeHash = xiiHashingUtils::StringHashTo32(pRenderData->GetDynamicRTTI()->GetTypeNameHash());
    return (uiTypeHash >> 16) ^ (uiTypeHash & 0xFFFF);
  }

  XII_FORCE_INLINE xiiUInt32 CalculateDistance(const xiiRenderData* pRenderData, const xiiCamera& camera)
  {
    ///\todo far-plane is not enough to normalize distance
    const float fDistance           = (camera.GetPosition() - pRenderData->m_GlobalTransform.m_vPosition).GetLength();
    const float fNormalizedDistance = xiiMath::Clamp(fDistance / camera.GetFarPlane(), 0.0f, 1.0f);
    return static_cast<xiiUInt32>(fNormalizedDistance * 65535.0f);
  }
} // namespace

// static
xiiUInt64 xiiRenderSortingFunctions::ByRenderDataThenFrontToBack(
  const xiiRenderData* pRenderData,
  xiiUInt32            uiRenderDataSortingKey,
  const xiiCamera&     camera)
{
  const xiiUInt64 uiTypeHash               = CalculateTypeHash(pRenderData);
  const xiiUInt64 uiRenderDataSortingKey64 = uiRenderDataSortingKey;
  const xiiUInt64 uiDistance               = CalculateDistance(pRenderData, camera);

  const xiiUInt64 uiSortingKey = (uiTypeHash << 48) | (uiRenderDataSortingKey64 << 16) | uiDistance;
  return uiSortingKey;
}

// static
xiiUInt64 xiiRenderSortingFunctions::BackToFrontThenByRenderData(
  const xiiRenderData* pRenderData,
  xiiUInt32            uiRenderDataSortingKey,
  const xiiCamera&     camera)
{
  const xiiUInt64 uiTypeHash               = CalculateTypeHash(pRenderData);
  const xiiUInt64 uiRenderDataSortingKey64 = uiRenderDataSortingKey;
  const xiiUInt64 uiInvDistance            = 0xFFFF - CalculateDistance(pRenderData, camera);

  const xiiUInt64 uiSortingKey = (uiInvDistance << 48) | (uiTypeHash << 32) | uiRenderDataSortingKey64;
  return uiSortingKey;
}



XII_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_SortingFunctions);
