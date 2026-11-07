/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Lighting/AtmosphereEnvironment.h>
#include <GraphicsCore/Pipeline/Passes/Atmosphere/AtmosphereFrameData.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/DirectionalLightFrameData.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/ReflectionProbeFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Extracts authored local probes and a deterministic global sky for shared environment lighting.
/// More than the supported descriptor capacity fails explicitly instead of dropping influences.
class XII_GRAPHICSCORE_DLL xiiWorldReflectionProbesPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiWorldReflectionProbesPass, xiiRenderPipelinePass);

public:
  bool                                     m_bEnabled = true;
  xiiRenderPipelineDataOutputPin           m_Probes{xiiGetStaticRTTI<xiiReflectionProbeFrameData>()};
  xiiRenderPipelineDataInputPin               m_Atmosphere{xiiGetStaticRTTI<xiiAtmosphereFrameData>(), true};
  xiiRenderPipelineDataInputPin               m_Directional{xiiGetStaticRTTI<xiiDirectionalLightFrameData>(), true};
  xiiResult                                Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderExtractionCategory> GetExtractionRequirements() const override { return xiiRenderExtractionCategory::Lights; }
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return m_bEnabled ? xiiRenderPipelineDeviceFeature::Compute : xiiRenderPipelineDeviceFeature::None; }

private:
  xiiSharedPtr<const xiiAtmosphereEnvironment> m_AtmosphereEnvironment;
};
