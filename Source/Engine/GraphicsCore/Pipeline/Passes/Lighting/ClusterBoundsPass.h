/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/LightClusterFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/States/PipelineState.h>
/// Builds exact viewport-clipped world-space bounds for a supplied cluster layout.
class XII_GRAPHICSCORE_DLL xiiClusterBoundsPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiClusterBoundsPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineOrderInputPin              m_Before;
  xiiRenderPipelineOrderOutputPin             m_After;
  xiiRenderPipelineDataInputPin               m_Layout{xiiGetStaticRTTI<xiiLightClusterFrameData>()};
  xiiRenderPipelineBufferOutputPin            m_Clusters{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  bool                                        m_bAsyncCompute = true;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
};
