/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/Pipeline/Passes/Atmosphere/AtmosphereFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

namespace xiiAtmosphereLookupPassUtils
{
  inline xiiResult Publish(xiiRenderPipelinePassContext& context, const xiiSharedPtr<const xiiAtmosphereFrameData>& snapshot, const xiiSharedPtr<xiiGALTexture>& texture, const xiiRenderPipelineTextureOutputPin& output, bool asynchronous)
  {
    struct Data
    {
      xiiSharedPtr<const xiiAtmosphereFrameData> m_Snapshot;
      xiiRenderGraphTextureHandle                m_Image;
    };
    const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, asynchronous ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Snapshot = snapshot;
      builder.WaitForFence(snapshot->m_Lookup->m_Ready, 1).AssertSuccess();
      // A preserved version propagates external readiness to every reader queue,
      // including a consumer whose physical image layout already matches.
      data.m_Image = builder.ReadWriteTexture(builder.ImportTexture(context.m_sPassName, texture, xiiGALResourceStateFlags::Unknown), xiiGALResourceStateFlags::ShaderResource); }, [](const Data&, xiiRenderGraphPassContext&) {});
    return context.SetOutput(output, pass.first->m_Image);
  }
} // namespace xiiAtmosphereLookupPassUtils
