/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Lighting/DisplayOutput.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiToneMappingPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiToneMappingPass, xiiRenderPipelinePass);

public:
  xiiToneMappingPass();
  ~xiiToneMappingPass();
  bool                            m_bUseViewDefaults = false;
  xiiEnum<xiiToneMappingOperator> m_Operator;
  xiiEnum<xiiDisplayOutputMode>   m_OutputMode;
  float                           m_fBloomStrength      = 0.05f;
  float                           m_fPaperWhiteNits     = 203.0f;
  float                           m_fMaximumDisplayNits = 1000.0f;

  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  void                                        GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;

private:
  struct Data;
  struct State;
  void                Setup(Data& data, xiiRenderGraphBuilder& builder);
  void                Execute(const Data& data, xiiRenderGraphPassContext& context);
  xiiUniquePtr<State> m_pState;
};
