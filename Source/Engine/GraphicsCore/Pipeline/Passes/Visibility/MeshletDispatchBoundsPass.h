/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/States/PipelineState.h>

/// Reduces the meshlet dispatch bound from the currently visible, selected LODs.
/// InitializedMaximum must contain zero before the reduction.
class XII_GRAPHICSCORE_DLL xiiMeshletDispatchBoundsPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMeshletDispatchBoundsPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineOrderInputPin              m_Before;
  xiiRenderPipelineOrderOutputPin             m_After;
  xiiRenderPipelineBufferInputPin             m_Scene{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_View{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Geometry{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Meshlets{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Visible{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Count{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Lod{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Initial{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineBufferOutputPin            m_Maximum{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiRenderPipelinePinAccess::ReadWrite};
  bool                                        m_bAsyncCompute = true;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
};
