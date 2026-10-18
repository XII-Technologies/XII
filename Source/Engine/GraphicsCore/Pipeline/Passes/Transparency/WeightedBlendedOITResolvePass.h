/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Composites supplied weighted accumulation and revealage over HDR scene radiance.
/// Accumulation, geometry rendering and target initialization are separate graph operations.
class XII_GRAPHICSCORE_DLL xiiWeightedBlendedOITResolvePass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiWeightedBlendedOITResolvePass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_InputAccumulation{xiiGALResourceFormat::RGBA16Float, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputRevealage{xiiGALResourceFormat::R8UNormalized, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputHDR{xiiGALResourceFormat::RGBA16Float, false, xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputHDR{xiiGALResourceFormat::RGBA16Float, false, xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::ReadWrite};
  bool                              m_bBypass = false;

  xiiWeightedBlendedOITResolvePass();
  ~xiiWeightedBlendedOITResolvePass();
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return m_bBypass ? xiiRenderPipelineDeviceFeature::None : xiiRenderPipelineDeviceFeature::Compute; }
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;

private:
  struct State;
  xiiUniquePtr<State> m_pState;
};
