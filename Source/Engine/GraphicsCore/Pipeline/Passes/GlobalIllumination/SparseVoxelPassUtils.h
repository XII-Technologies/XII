/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/SparseVoxelFrameData.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/LightClusterPassUtils.h>
namespace xiiSparseVoxelPassUtils
{
  inline xiiSharedPtr<const xiiSparseVoxelFrameData> Snapshot(const xiiRenderPipelinePassContext& context, const xiiRenderPipelineDataInputPin& pin)
  {
    const auto data = context.GetInputData(pin);
    if (!data || !data->GetDynamicRTTI()->IsDerivedFrom<xiiSparseVoxelFrameData>())
      return {};
    const auto  snapshot = data.Downcast<const xiiSparseVoxelFrameData>();
    const auto& c        = snapshot->m_Constants.ClipmapAndUpdateCounts;
    const auto& p        = snapshot->m_Constants.PoolLayout;
    return c.x && c.x <= 8 && c.y >= 2 && c.y <= 64 && c.z >= 2 && c.z <= 32 && snapshot->m_Levels.GetCount() == c.x && snapshot->m_Pages.GetCount() == c.x * c.y * c.y * c.y &&
        snapshot->m_Updates.GetCount() == c.w && c.w <= p.y && p.x == c.z * c.z * c.z && xiiUInt64(p.x) * p.y == snapshot->m_PoolCount && snapshot->m_PoolCount ?
      snapshot :
      xiiSharedPtr<const xiiSparseVoxelFrameData>();
  }
} // namespace xiiSparseVoxelPassUtils
