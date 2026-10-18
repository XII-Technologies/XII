/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Pipeline/Passes/Lighting/LightClusterFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/States/PipelineState.h>
/// Builds emitted-power CDF entries without modifying the supplied raw light list.
class XII_GRAPHICSCORE_DLL xiiClusterLightSamplingPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiClusterLightSamplingPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineDataInputPin               m_Layout{xiiGetStaticRTTI<xiiLightClusterFrameData>()};
  xiiRenderPipelineBufferInputPin             m_Lights{false, xiiGALBindFlags::ShaderResource}, m_Grid{false, xiiGALBindFlags::ShaderResource}, m_Indices{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin            m_Packed{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess}, m_Totals{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  bool                                        m_bAsyncCompute = true;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
};
