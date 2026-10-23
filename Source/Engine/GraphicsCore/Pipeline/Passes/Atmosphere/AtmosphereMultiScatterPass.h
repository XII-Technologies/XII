/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/States/PipelineState.h>
/// Compute a graph-owned atmosphere lookup image from explicit physical parameters.
class XII_GRAPHICSCORE_DLL xiiAtmosphereMultiScatterPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAtmosphereMultiScatterPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferInputPin             m_Atmosphere{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineTextureInputPin            m_Transmittance{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin           m_Output{xiiGALResourceFormat::RGBA32Float, false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1};
  xiiUInt32                                   m_uiWidth = 32, m_uiHeight = 32;
  bool                                        m_bAsyncCompute = false;
  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  xiiSharedPtr<xiiGALSampler>              m_Linear;
};
