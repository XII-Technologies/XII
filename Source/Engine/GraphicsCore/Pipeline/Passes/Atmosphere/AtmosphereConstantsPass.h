/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Lighting/Atmosphere.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiAtmosphereConstantsPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAtmosphereConstantsPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineOrderInputPin   m_Before;
  xiiRenderPipelineOrderOutputPin  m_After;
  xiiRenderPipelineBufferOutputPin m_OutputAtmosphereConstants{false, xiiGALBindFlags::UniformBuffer};

  bool                  m_bUseWorldAtmosphere = false;
  xiiAtmosphereSettings m_Settings;

  xiiResult                                Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderExtractionCategory> GetExtractionRequirements() const override { return m_bUseWorldAtmosphere ? xiiRenderExtractionCategory::Atmosphere : xiiRenderExtractionCategory::None; }
};
