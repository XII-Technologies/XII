/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Pipeline/Passes/Lighting/DirectionalLightFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Selects the brightest eligible directional emitter in this view, with deterministic component-ID ties.
/// Requires light packets only; no GPU light table, IES, mesh, material, GI or shadow initialization.
class XII_GRAPHICSCORE_DLL xiiWorldDirectionalLightPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiWorldDirectionalLightPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineDataOutputPin           m_Light{xiiGetStaticRTTI<xiiDirectionalLightFrameData>()};
  xiiResult                                Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderExtractionCategory> GetExtractionRequirements() const override { return xiiRenderExtractionCategory::Lights; }
};
