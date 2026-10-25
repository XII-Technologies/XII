/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/States/PipelineState.h>

/// Applies frustum/flags/residency tests and generation-validated LOD selection.
/// Counter must initially contain zero. LOD storage must be initialized by its producer.
class XII_GRAPHICSCORE_DLL xiiInstanceCullPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiInstanceCullPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferInputPin             m_Instances{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_View{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Geometry{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Candidates{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Counter{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineBufferInputPin             m_InitializedLod{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineBufferInputPin             m_PreviousLod{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin            m_Visible{true, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineBufferOutputPin            m_Count{true, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiRenderPipelinePinAccess::ReadWrite};
  xiiRenderPipelineBufferOutputPin            m_Lod{true, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiRenderPipelinePinAccess::ReadWrite};
  bool                                        m_bAsyncCompute = true;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
};
