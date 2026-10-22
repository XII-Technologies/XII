/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Lighting/DisplayOutput.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiColorGradingPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiColorGradingPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineOrderInputPin    m_Before;
  xiiRenderPipelineOrderOutputPin   m_After;
  xiiRenderPipelineTextureInputPin  m_InputDisplayLinearColor{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputGradedColor{xiiGALResourceFormat::RGBA16Float, false, {}, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineBufferOutputPin  m_OutputColorGradingConstants{true};
  xiiRenderPipelineTextureOutputPin m_OutputDisplayLinearColor{xiiGALResourceFormat::Unknown, true, {}, xiiGALResourceDimension::Undefined, 0, xiiRenderPipelinePinAccess::Read};

  xiiColorGradingPass();
  ~xiiColorGradingPass();
  bool                    m_bUseViewDefaults = false;
  xiiColorGradingSettings m_Settings;

  xiiResult Configure(const xiiVariantDictionary& properties) override;

  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;

private:
  struct Data;
  struct State;
  void                Setup(Data& data, xiiRenderGraphBuilder& builder);
  static void         Execute(const Data& data, xiiRenderGraphPassContext& context);
  xiiUniquePtr<State> m_pState;
};
