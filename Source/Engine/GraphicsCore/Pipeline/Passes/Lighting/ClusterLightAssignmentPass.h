/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/Passes/Lighting/LightClusterFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/States/PipelineState.h>
/// Assigns explicit analytical light bounds to initialized lists; overflow must be consumed.
class XII_GRAPHICSCORE_DLL xiiClusterLightAssignmentPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiClusterLightAssignmentPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineDataInputPin   m_Layout{xiiGetStaticRTTI<xiiLightClusterFrameData>()};
  xiiRenderPipelineBufferInputPin m_Lights{false, xiiGALBindFlags::ShaderResource};
  /// Optional exact view selection; invalid indices are ignored, including the empty sentinel.
  xiiRenderPipelineBufferInputPin             m_Candidates{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Clusters{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_InputGrid{false, xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineBufferInputPin             m_InputIndices{false, xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineBufferInputPin             m_InputOverflow{false, xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineBufferOutputPin            m_Grid{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineBufferOutputPin            m_Indices{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineBufferOutputPin            m_Overflow{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  bool                                        m_bAsyncCompute = true;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
};
