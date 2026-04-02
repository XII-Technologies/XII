#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Passes 46-47 — Direct + Indirect Lighting Combine.
///
/// Graphics/Compute. Evaluates the GGX BRDF using GBuffer data against all
/// visible lights from the cluster list, shadow maps, contact shadows, and
/// optional RT shadows. Adds RT or SSR reflections and AO-modulated indirect
/// from RT GI / probe data. Outputs two separate HDR buffers (direct + indirect)
/// to allow flexible blending in the composite stage.
class XII_GRAPHICSCORE_DLL xiiLightingCombinePass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLightingCombinePass, xiiRenderPipelinePass);

public:
  xiiLightingCombinePass();
  virtual ~xiiLightingCombinePass();
  virtual void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard) override;

  bool m_bEnableRTShadows     = true;
  bool m_bEnableContactShadows = true;
  bool m_bEnableRTReflections  = true;
  bool m_bEnableSSRFallback    = true;
};
