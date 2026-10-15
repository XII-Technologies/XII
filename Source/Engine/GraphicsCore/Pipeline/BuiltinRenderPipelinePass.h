/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Reusable built-in rendering operations. Composite stages preserve device-dependent rendering
/// choices; individual post-processing passes can be inserted and rewired independently.
class XII_GRAPHICSCORE_DLL xiiBuiltinRenderPipelinePass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBuiltinRenderPipelinePass, xiiRenderPipelinePass);

public:
  xiiString                                m_sStage = "Bloom";
  xiiBitflags<xiiRenderExtractionCategory> GetExtractionRequirements() const override;
  xiiBitflags<xiiRenderPipelineFeature>    GetRuntimeFeatures() const override;
  static xiiArrayPtr<const xiiStringView>  GetStages();
  xiiResult                                Configure(const xiiVariantDictionary& properties) override;
  void                                     GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override;
  xiiResult                                Build(xiiRenderPipelinePassContext& context) override;
  xiiResult                                ValidatePipeline(const xiiRenderPipelineResourceDescriptor& descriptor, xiiArrayPtr<const xiiUInt32> order, xiiUInt32 index, xiiStringBuilder* error) const override;
};
