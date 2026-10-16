/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Lighting/Atmosphere.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiAtmosphereConstantsPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAtmosphereConstantsPass, xiiRenderPipelinePass);

public:
  bool                                     m_bUseWorldAtmosphere = false;
  xiiAtmosphereSettings                    m_Settings;
  void                                     GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override;
  xiiResult                                Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderExtractionCategory> GetExtractionRequirements() const override { return m_bUseWorldAtmosphere ? xiiRenderExtractionCategory::Atmosphere : xiiRenderExtractionCategory::None; }
};
