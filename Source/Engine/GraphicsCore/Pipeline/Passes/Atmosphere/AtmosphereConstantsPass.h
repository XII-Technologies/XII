/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Lighting/Atmosphere.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiAtmosphereConstantsPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAtmosphereConstantsPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferOutputPin m_OutputAtmosphereConstants{true, xiiGALBindFlags::UniformBuffer};

  bool                  m_bUseWorldAtmosphere = false;
  bool                  m_bEnabled            = true;
  /// Publish no atmosphere when no world component is selected, instead of using authored fallback settings.
  bool                  m_bRequireWorldAtmosphere = false;
  xiiAtmosphereSettings m_Settings;

  xiiResult                                Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderExtractionCategory> GetExtractionRequirements() const override { return m_bEnabled && m_bUseWorldAtmosphere ? xiiRenderExtractionCategory::Atmosphere : xiiRenderExtractionCategory::None; }
};
