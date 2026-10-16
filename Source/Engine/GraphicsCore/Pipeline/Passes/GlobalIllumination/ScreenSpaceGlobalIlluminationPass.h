/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiScreenSpaceGlobalIlluminationPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiScreenSpaceGlobalIlluminationPass, xiiRenderPipelinePass);

public:
  xiiScreenSpaceGlobalIlluminationPass();
  ~xiiScreenSpaceGlobalIlluminationPass();
  float                                       m_fRayLength          = 2.0f;
  xiiUInt32                                   m_uiSampleCount       = 8;
  float                                       m_fThickness          = 0.15f;
  float                                       m_fIntensity          = 1.0f;
  bool                                        m_bSkipWithHardwareGI = false;
  void                                        GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override;
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
