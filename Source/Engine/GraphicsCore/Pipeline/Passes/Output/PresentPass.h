/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Lighting/DisplayOutput.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiPresentPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPresentPass, xiiRenderPipelinePass);

public:
  xiiPresentPass();
  ~xiiPresentPass();
  bool                          m_bUseViewDefaults = false;
  xiiEnum<xiiDisplayOutputMode> m_OutputMode;
  float                         m_fPaperWhiteNits     = 203.0f;
  float                         m_fMaximumDisplayNits = 1000.0f;
  xiiResult                     Configure(const xiiVariantDictionary& properties) override;
  void                          GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override;
  xiiResult                     Build(xiiRenderPipelinePassContext& context) override;

private:
  struct Data;
  struct State;
  void                Setup(Data& data, xiiRenderGraphBuilder& builder);
  void                Execute(const Data& data, xiiRenderGraphPassContext& context);
  xiiUniquePtr<State> m_pState;
};
