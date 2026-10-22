/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Motion-aware ambient visibility filtering with occurrence-local mask and surface history.
class XII_GRAPHICSCORE_DLL xiiAmbientOcclusionTemporalPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAmbientOcclusionTemporalPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineOrderInputPin    m_Before;
  xiiRenderPipelineOrderOutputPin   m_After;
  xiiRenderPipelineTextureInputPin  m_Raw{xiiGALResourceFormat::R8UNormalized, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_Depth{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_Normal{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_Velocity{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineBufferInputPin   m_Camera{false, xiiGALBindFlags::UniformBuffer}, m_Global{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineTextureOutputPin m_Output{xiiGALResourceFormat::R8UNormalized, false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_Previous{xiiGALResourceFormat::R8UNormalized, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read, true};
  xiiRenderPipelineTextureOutputPin m_Surface{xiiGALResourceFormat::RGBA16Float, false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1};
  float                             m_fHistoryWeight   = 0.92f;
  float                             m_fDepthThreshold  = 0.08f;
  float                             m_fNormalThreshold = 0.82f;
  float                             m_fSpatialWeight   = 0.35f;
  xiiAmbientOcclusionTemporalPass();
  ~xiiAmbientOcclusionTemporalPass();
  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }
  xiiResult                                   BuildTemporalHistory(xiiRenderPipelinePassContext& context) override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  void                                        InvalidateTemporalHistory() override;

private:
  struct State;
  xiiUniquePtr<State> m_pState;
};
