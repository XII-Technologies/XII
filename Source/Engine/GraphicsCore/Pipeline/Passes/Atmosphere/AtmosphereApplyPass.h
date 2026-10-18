/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/States/PipelineState.h>
/// Apply supplied atmospheric LUTs and physical parameters to an immutable scene image.
class XII_GRAPHICSCORE_DLL xiiAtmosphereApplyPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAtmosphereApplyPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin            m_Depth{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin            m_Radiance{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin            m_Transmittance{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin            m_MultiScatter{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineBufferInputPin             m_Camera{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferInputPin             m_Light{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferInputPin             m_Atmosphere{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineTextureOutputPin           m_Output{xiiGALResourceFormat::RGBA32Float, false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1};
  bool                                        m_bAsyncCompute = false;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  xiiSharedPtr<xiiGALSampler>              m_Point, m_Linear;
};
