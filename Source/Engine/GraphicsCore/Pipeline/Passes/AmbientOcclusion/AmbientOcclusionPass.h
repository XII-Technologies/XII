/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Ambient visibility from explicit depth/normal/frame inputs and an optional ray scene.
class XII_GRAPHICSCORE_DLL xiiAmbientOcclusionPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAmbientOcclusionPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin               m_Depth{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin               m_Normal{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineBufferInputPin                m_Camera{false, xiiGALBindFlags::UniformBuffer}, m_Global{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineAccelerationStructureInputPin m_Scene{true};
  xiiRenderPipelineBufferInputPin                m_Materials{true, xiiGALBindFlags::ShaderResource}, m_Geometry{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineTextureOutputPin              m_Output{xiiGALResourceFormat::R8UNormalized, false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineBufferOutputPin               m_Constants{false, xiiGALBindFlags::UniformBuffer};
  bool                                           m_bUseRayTracing    = true;
  bool                                           m_bAllowScreenSpace = true;
  float                                          m_fRadius = 2.0f, m_fRayOriginBias = 0.005f, m_fRayTMin = 0.001f, m_fNormalBias = 0.05f, m_fPixelStride = 6.0f, m_fMinSampleDistance = 0.005f;
  xiiUInt32                                      m_uiDirectionCount = 8, m_uiStepCount = 4;
  xiiAmbientOcclusionPass();
  ~xiiAmbientOcclusionPass();
  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;

private:
  struct State;
  xiiUniquePtr<State> m_pState;
};
