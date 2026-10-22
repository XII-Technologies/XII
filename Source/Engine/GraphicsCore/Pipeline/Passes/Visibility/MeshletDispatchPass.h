/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/States/PipelineState.h>

/// Builds safe indirect meshlet work dimensions without a CPU count readback.
class XII_GRAPHICSCORE_DLL xiiMeshletDispatchPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMeshletDispatchPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineOrderInputPin              m_Before;
  xiiRenderPipelineOrderOutputPin             m_After;
  xiiRenderPipelineBufferInputPin             m_Visible{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Count{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Meshlets{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Maximum{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin            m_Arguments{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments};
  bool                                        m_bAsyncCompute = true;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
};
