/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiHybridReflectionCompositePass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiHybridReflectionCompositePass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_InputEnvironmentSpecular{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputSSRTerm{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputRTFinalReflections{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputHDRSceneColor{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputHDRSceneColor{xiiGALResourceFormat::RGBA16Float, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Automatic};
  xiiRenderPipelineTextureOutputPin m_OutputEnvironmentSpecular{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiRenderPipelineTextureOutputPin m_OutputSSRTerm{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiRenderPipelineTextureOutputPin m_OutputRTFinalReflections{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiHybridReflectionCompositePass();
  ~xiiHybridReflectionCompositePass();
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;

private:
  struct Data;
  struct State;
  void                Setup(Data& data, xiiRenderGraphBuilder& builder);
  void                Execute(const Data& data, xiiRenderGraphPassContext& context);
  xiiUniquePtr<State> m_pState;
};
