/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiAtmosphereCompositePass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAtmosphereCompositePass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_InputSceneDepth{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputAtmTransmittance{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputAtmMultiScatter{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputHDRSceneColor{xiiGALResourceFormat::RGBA16Float, false, xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineBufferInputPin   m_InputPerFrameCamera{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferInputPin   m_InputPerFrameLight{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineTextureOutputPin m_OutputHDRSceneColor{xiiGALResourceFormat::RGBA16Float, false, {}, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputAtmTransmittance{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiRenderPipelineTextureOutputPin m_OutputAtmMultiScatter{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiRenderPipelineBufferInputPin   m_InputAtmosphereConstants{false, xiiGALBindFlags::UniformBuffer};

  xiiAtmosphereCompositePass();
  ~xiiAtmosphereCompositePass();

  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  struct Data;
  void Setup(Data& data, xiiRenderGraphBuilder& builder);
  void Execute(const Data& data, xiiRenderGraphPassContext& context);
  struct State;
  xiiUniquePtr<State> m_pState;
};
