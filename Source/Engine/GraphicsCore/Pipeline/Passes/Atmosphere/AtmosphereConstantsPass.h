/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Lighting/Atmosphere.h>
#include <GraphicsCore/Pipeline/Passes/Atmosphere/AtmosphereFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiAtmosphereConstantsPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAtmosphereConstantsPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferOutputPin m_OutputAtmosphereConstants{true, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineDataOutputPin   m_LookupData{xiiGetStaticRTTI<xiiAtmosphereFrameData>(), true};
  /// Reuse canonical immutable LUTs instead of rebuilding them in every view recording.
  bool m_bCacheLookups = false;

  bool m_bUseWorldAtmosphere = false;
  bool m_bEnabled            = true;
  /// Publish no atmosphere when no world component is selected, instead of using authored fallback settings.
  bool                  m_bRequireWorldAtmosphere = false;
  xiiAtmosphereSettings m_Settings;

  xiiResult                                Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderExtractionCategory> GetExtractionRequirements() const override { return m_bEnabled && m_bUseWorldAtmosphere ? xiiRenderExtractionCategory::Atmosphere : xiiRenderExtractionCategory::None; }
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return m_bCacheLookups ? xiiRenderPipelineDeviceFeature::Compute : xiiRenderPipelineDeviceFeature::None; }

private:
  xiiSharedPtr<const xiiAtmosphereLookup> m_Lookup;
};
