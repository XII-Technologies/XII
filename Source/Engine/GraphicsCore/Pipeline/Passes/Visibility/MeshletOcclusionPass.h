/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/States/PipelineState.h>

/// Filters supplied absolute (instance, meshlet) pairs against a matching reversed-Z depth image and projection.
/// Temporal validity/invalidation belongs to the graph supplying those explicit inputs.
/// InitialCount must contain zero; this operation owns no scene state or history.
class XII_GRAPHICSCORE_DLL xiiMeshletOcclusionPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMeshletOcclusionPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferInputPin             m_Instances{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Meshlets{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_View{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Candidates{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_CandidateCount{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_InitialCount{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineTextureInputPin            m_Depth{xiiGALResourceFormat::R32Float, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineBufferOutputPin            m_Visible{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineBufferOutputPin            m_Count{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiRenderPipelinePinAccess::ReadWrite};
  float                                       m_fDepthBias    = 0.0005f;
  bool                                        m_bAsyncCompute = true;
  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
};
