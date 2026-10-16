/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Lighting/DisplayOutput.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiColorGradingPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiColorGradingPass, xiiRenderPipelinePass);

public:
  xiiColorGradingPass();
  ~xiiColorGradingPass();
  bool                    m_bUseViewDefaults = false;
  xiiColorGradingSettings m_Settings;

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
