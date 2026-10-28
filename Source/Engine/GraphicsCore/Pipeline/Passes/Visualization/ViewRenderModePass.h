/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/Declarations.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Visualizes supplied surface channels. Never discovers scene targets by global name.
class XII_GRAPHICSCORE_DLL xiiViewRenderModePass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiViewRenderModePass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_InputDisplayLinearColor{xiiGALResourceFormat::RGBA16Float, false, xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputDisplayLinearColor{xiiGALResourceFormat::RGBA16Float, false, {}, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputSceneDepth{xiiGALResourceFormat::Unknown, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputGBufferAlbedo{xiiGALResourceFormat::Unknown, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputGBufferNormal{xiiGALResourceFormat::Unknown, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputGBufferMaterial{xiiGALResourceFormat::Unknown, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputGBufferEmissive{xiiGALResourceFormat::Unknown, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputGBufferObjectId{xiiGALResourceFormat::Unknown, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputWireframeSurface{xiiGALResourceFormat::Unknown, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputOverdraw{xiiGALResourceFormat::RGBA32Float, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputDilatedVelocityBuffer{xiiGALResourceFormat::Unknown, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputStableAO{xiiGALResourceFormat::Unknown, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputIndirectLighting{xiiGALResourceFormat::Unknown, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputEnvironmentSpecular{xiiGALResourceFormat::Unknown, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputBloomTexture{xiiGALResourceFormat::Unknown, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineBufferInputPin   m_InputPerFrameCamera{true, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferInputPin   m_InputCurrentExposure{true, xiiGALBindFlags::ShaderResource};

  xiiRenderPipelineTextureInputPin m_InputDirectLighting{xiiGALResourceFormat::Unknown, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};

  xiiViewRenderModePass();
  ~xiiViewRenderModePass();
  bool                       m_bUseViewMode = false;
  xiiEnum<xiiViewRenderMode> m_Mode;
  /// UV displacement mapped to half the display range. Zero velocity is neutral gray.
  float m_fMotionVectorScale = 10.0f;

  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  struct State;
  xiiUniquePtr<State> m_State;
};
