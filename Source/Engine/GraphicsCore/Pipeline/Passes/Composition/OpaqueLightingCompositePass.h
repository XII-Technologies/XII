/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiOpaqueLightingCompositePass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiOpaqueLightingCompositePass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_InputDirectLighting{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputIndirectLighting{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputGBufferEmissive{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputRTFinalGI{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputVolumetricScattering{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputHDRSceneColor{xiiGALResourceFormat::RGBA16Float, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Automatic};
  xiiRenderPipelineTextureOutputPin m_OutputDirectLighting{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiRenderPipelineTextureOutputPin m_OutputIndirectLighting{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiRenderPipelineTextureOutputPin m_OutputGBufferEmissive{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiRenderPipelineTextureOutputPin m_OutputRTFinalGI{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiRenderPipelineTextureOutputPin m_OutputVolumetricScattering{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiOpaqueLightingCompositePass();
  ~xiiOpaqueLightingCompositePass();
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;

private:
  struct Data;
  struct State;
  void                Setup(Data& data, xiiRenderGraphBuilder& builder);
  void                Execute(const Data& data, xiiRenderGraphPassContext& context);
  xiiUniquePtr<State> m_pState;
};
