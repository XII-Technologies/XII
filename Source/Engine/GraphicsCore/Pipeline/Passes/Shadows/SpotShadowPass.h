/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Pipeline/Passes/Shadows/ShadowFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Prepares perspective projections from authoritative spotlight extraction.
class XII_GRAPHICSCORE_DLL xiiSpotShadowPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSpotShadowPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineDataOutputPin           m_Shadows{xiiGetStaticRTTI<xiiSpotShadowSnapshot>(), true};
  bool                                     m_bEnabled     = true;
  xiiUInt32                                m_uiResolution = 1024, m_uiMaximumLights = 4;
  float                                    m_fNearPlane = 0.02f, m_fDepthBias = 0.0005f, m_fNormalBias = 0.01f, m_fNormalBiasTexels = 1;
  xiiResult                                Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderExtractionCategory> GetExtractionRequirements() const override { return xiiRenderExtractionCategory::Geometry | xiiRenderExtractionCategory::Lights; }
  xiiBitflags<xiiRenderPipelineFeature>    GetRuntimeFeatures() const override { return xiiRenderPipelineFeature::GpuScene | xiiRenderPipelineFeature::Lighting; }
};
