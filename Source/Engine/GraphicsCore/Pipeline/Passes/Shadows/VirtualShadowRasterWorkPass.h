/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/ShadowFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Selects a bounded immutable batch from explicit projections and device residency.
class XII_GRAPHICSCORE_DLL xiiVirtualShadowRasterWorkPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVirtualShadowRasterWorkPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineOrderInputPin        m_Before;
  xiiRenderPipelineOrderOutputPin       m_After;
  xiiRenderPipelineDataInputPin         m_Cascades{xiiGetStaticRTTI<xiiShadowCascadeSnapshot>()};
  xiiRenderPipelineDataInputPin         m_Residency{xiiGetStaticRTTI<xiiVirtualShadowResidencySnapshot>()};
  xiiRenderPipelineDataOutputPin        m_Pages{xiiGetStaticRTTI<xiiVirtualShadowRasterBatch>()};
  xiiUInt32                             m_uiLightId = 0, m_uiMaxPages = 128;
  xiiResult                             Configure(const xiiVariantDictionary& properties) override;
  xiiResult                             Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineFeature> GetRuntimeFeatures() const override { return xiiRenderPipelineFeature::VirtualShadows; }
};
