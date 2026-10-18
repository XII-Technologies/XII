/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/DDGIProbeFrameData.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
namespace xiiDDGIProbePassUtils
{
  inline xiiSharedPtr<const xiiDDGIProbeFrameData> Snapshot(const xiiRenderPipelinePassContext& context, const xiiRenderPipelineDataInputPin& pin)
  {
    const auto data = context.GetInputData(pin);
    if (!data || !data->GetDynamicRTTI()->IsDerivedFrom<xiiDDGIProbeFrameData>())
      return {};
    xiiSharedPtr<const xiiDDGIProbeFrameData> snapshot = data.Downcast<const xiiDDGIProbeFrameData>();
    const auto&                               counts   = snapshot->m_Constants.ProbeCountsAndUpdateCount;
    const xiiUInt64                           count    = xiiUInt64(counts.x) * counts.y * counts.z;
    return count && count <= 1048576 && snapshot->m_Probes.GetCount() == count && snapshot->m_Updates.GetCount() == counts.w && counts.w && counts.w <= count &&
        snapshot->m_AtlasSize.HasNonZeroArea() && snapshot->m_AtlasSize.width == xiiUInt32(snapshot->m_Constants.MinimumCellAndAtlasWidth.w) &&
        xiiUInt64(snapshot->m_AtlasSize.width) * snapshot->m_AtlasSize.height >= count ?
      snapshot :
      xiiSharedPtr<const xiiDDGIProbeFrameData>();
  }
  inline bool Buffer(const xiiRenderPipelinePassContext& context, xiiRenderGraphBufferHandle handle, xiiUInt32 stride, xiiUInt32 count, xiiBitflags<xiiGALBindFlags> flags)
  {
    const auto* d = context.m_Graph.GetBufferDescription(handle);
    return d && d->m_Mode == xiiGALBufferMode::Structured && d->m_uiSize == xiiUInt64(stride) * count && d->m_uiElementByteStride == stride && d->m_BindFlags.AreAllSet(flags);
  }
} // namespace xiiDDGIProbePassUtils
