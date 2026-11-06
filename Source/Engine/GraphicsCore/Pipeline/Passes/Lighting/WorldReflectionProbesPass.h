/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/GraphicsCoreDLL.h>

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
  xiiResult                                Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderExtractionCategory> GetExtractionRequirements() const override { return xiiRenderExtractionCategory::Lights; }
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return m_bEnabled ? xiiRenderPipelineDeviceFeature::Compute : xiiRenderPipelineDeviceFeature::None; }
};
