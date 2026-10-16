/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Lighting/DisplayOutput.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiTemporalAntiAliasingPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTemporalAntiAliasingPass, xiiRenderPipelinePass);

public:
  xiiTemporalAntiAliasingPass();
  ~xiiTemporalAntiAliasingPass();
  float                                       m_fBaseBlendAlpha   = 0.08f;
  float                                       m_fMotionBlendScale = 12.0f;
  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                   BuildTemporalHistory(xiiRenderPipelinePassContext& context) override;
  void                                        GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  void                                        InvalidateTemporalHistory() override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  struct Data;
  struct State;
  void                Setup(Data& data, xiiRenderGraphBuilder& builder);
  void                Execute(const Data& data, xiiRenderGraphPassContext& context);
  xiiUniquePtr<State> m_pState;
};
