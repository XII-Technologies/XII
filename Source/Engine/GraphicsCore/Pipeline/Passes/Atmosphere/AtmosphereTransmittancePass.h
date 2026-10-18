/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/States/PipelineState.h>
/// Compute a graph-owned atmosphere lookup image from explicit physical parameters.
class XII_GRAPHICSCORE_DLL xiiAtmosphereTransmittancePass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAtmosphereTransmittancePass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferInputPin m_Atmosphere{false, xiiGALBindFlags::UniformBuffer};

  xiiRenderPipelineTextureOutputPin           m_Output{xiiGALResourceFormat::RGBA32Float, false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1};
  xiiUInt32                                   m_uiWidth = 256, m_uiHeight = 64;
  bool                                        m_bAsyncCompute = false;
  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
};
