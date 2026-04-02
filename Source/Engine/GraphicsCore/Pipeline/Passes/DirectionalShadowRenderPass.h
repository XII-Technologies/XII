#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Passes 17 — Directional Shadow Rendering.
///
/// Graphics queue. Renders per-cascade shadow maps into a texture atlas using
/// indirect draws from the shadow caster draw lists. Uses atlas packing and
/// stable texel snapping to minimize shadow shimmer.
class XII_GRAPHICSCORE_DLL xiiDirectionalShadowRenderPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDirectionalShadowRenderPass, xiiRenderPipelinePass);

public:
  xiiDirectionalShadowRenderPass();
  virtual ~xiiDirectionalShadowRenderPass();
  virtual void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard) override;

  xiiUInt32 m_uiAtlasSize = 4096u; ///< Shadow atlas resolution (square).
};
