/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Lighting/DisplayOutput.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiUpscalePass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiUpscalePass, xiiRenderPipelinePass);

public:
  xiiUpscalePass();
  ~xiiUpscalePass();
  xiiUInt32 m_uiWidth  = 0;
  xiiUInt32 m_uiHeight = 0;

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
