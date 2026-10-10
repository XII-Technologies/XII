/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Reusable built-in rendering operations. Composite stages preserve device-dependent rendering
/// choices; individual post-processing passes can be inserted and rewired independently.
class XII_GRAPHICSCORE_DLL xiiBuiltinRenderPipelinePass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBuiltinRenderPipelinePass, xiiRenderPipelinePass);

public:
  xiiString                               m_sStage = "Bloom";
  static xiiArrayPtr<const xiiStringView> GetStages();
  xiiResult                               Configure(const xiiVariantDictionary& properties) override;
  void                                    GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override;
  xiiResult                               Build(xiiRenderPipelinePassContext& context) override;
};
