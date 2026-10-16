/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiScreenSpaceGlobalIlluminationCompositePass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiScreenSpaceGlobalIlluminationCompositePass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_InputSSGITerm{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputHDRSceneColor{xiiGALResourceFormat::RGBA16Float, false, xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputHDRSceneColor{xiiGALResourceFormat::RGBA16Float, false, {}, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputHardwareGI{xiiGALResourceFormat::Unknown, true, {}, xiiGALResourceDimension::Texture2D, 1};

  xiiScreenSpaceGlobalIlluminationCompositePass();
  ~xiiScreenSpaceGlobalIlluminationCompositePass();
  bool m_bEnabled            = true;
  bool m_bSkipWithHardwareGI = false;

  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  struct Data;
  void Setup(Data& data, xiiRenderGraphBuilder& builder);
  void Execute(const Data& data, xiiRenderGraphPassContext& context);
  struct State;
  xiiUniquePtr<State> m_pState;
};
