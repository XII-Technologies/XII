/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/States/PipelineState.h>

/// Produces DrawIndirect vertex-pull arguments from an explicitly supplied compact meshlet list.
/// This operation prepares commands only; it owns no geometry residency or raster execution.
class XII_GRAPHICSCORE_DLL xiiVertexPullDrawCommandsPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVertexPullDrawCommandsPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferInputPin             m_Scene{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_View{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Geometry{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Meshlets{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Visible{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Count{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin            m_Commands{true, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments};
  xiiRenderPipelineBufferOutputPin            m_CommandCount{true, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments};
  bool                                        m_bAsyncCompute = true;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
};
