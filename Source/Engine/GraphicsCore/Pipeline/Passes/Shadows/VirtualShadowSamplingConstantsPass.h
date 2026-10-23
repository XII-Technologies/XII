/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/ShadowFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
/// Captures sampling addresses from explicitly supplied immutable device residency.
class XII_GRAPHICSCORE_DLL xiiVirtualShadowSamplingConstantsPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVirtualShadowSamplingConstantsPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineDataInputPin    m_Residency{xiiGetStaticRTTI<xiiVirtualShadowResidencySnapshot>()};
  xiiRenderPipelineBufferOutputPin m_Constants{false, xiiGALBindFlags::UniformBuffer};
  xiiUInt32                        m_uiLightId = 1;
  bool                             m_bEnabled  = true;
  xiiResult                        Configure(const xiiVariantDictionary& properties) override;
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override;
};
