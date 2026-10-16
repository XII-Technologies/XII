/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Select a supplied texture or its required fallback without copying or submitting GPU work.
class XII_GRAPHICSCORE_DLL xiiTextureSelectionPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTextureSelectionPass, xiiRenderPipelinePass);

public:
  bool      m_bPreferPrimary = true;
  void      GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override;
  xiiResult Build(xiiRenderPipelinePassContext& context) override;
};
