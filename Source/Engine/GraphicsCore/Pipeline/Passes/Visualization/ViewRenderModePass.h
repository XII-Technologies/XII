/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/Declarations.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Visualizes supplied surface channels. Never discovers scene targets by global name.
class XII_GRAPHICSCORE_DLL xiiViewRenderModePass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiViewRenderModePass, xiiRenderPipelinePass);

public:
  xiiViewRenderModePass();
  ~xiiViewRenderModePass();
  bool                                        m_bUseViewMode = false;
  xiiEnum<xiiViewRenderMode>                  m_Mode;
  void                                        GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override;
  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  struct State;
  xiiUniquePtr<State> m_State;
};
