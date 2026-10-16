/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Lighting/DisplayOutput.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiBloomPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBloomPass, xiiRenderPipelinePass);

public:
  xiiBloomPass();
  ~xiiBloomPass();
  bool  m_bUseViewDefaults = false;
  float m_fThreshold       = 1.0f;
  float m_fKnee            = 0.5f;
  float m_fRadius          = 1.0f;

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
