/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Lighting/DisplayOutput.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiAutoExposurePass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAutoExposurePass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferInputPin  m_InputLuminanceHistogram;
  xiiRenderPipelineBufferOutputPin m_OutputCurrentExposure;

  xiiAutoExposurePass();
  ~xiiAutoExposurePass();
  xiiAutoExposurePass(const xiiAutoExposurePass&)            = delete;
  xiiAutoExposurePass& operator=(const xiiAutoExposurePass&) = delete;
  bool                 m_bUseViewDefaults                    = false;
  xiiExposureSettings  m_Settings;
  xiiResult            Configure(const xiiVariantDictionary& properties) override;

  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  void                                        InvalidateTemporalHistory() override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  struct Data;
  struct State;
  void                Setup(Data& data, xiiRenderGraphBuilder& builder);
  static void         Execute(const Data& data, xiiRenderGraphPassContext& context);
  xiiSharedPtr<State> m_pState;
};
