#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// \brief Pass 16 — Directional Shadow Culling.
///
/// Async Compute. For each cascade volume, tests scene bounds against the cascade
/// AABB and marks casters as visible. Per-cascade cull avoids full overdraw in
/// shadow maps. Outputs per-cascade draw lists consumed by DirectionalShadowRenderPass.
class XII_GRAPHICSCORE_DLL xiiShadowCasterCullPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiShadowCasterCullPass, xiiRenderPipelinePass);

public:
  xiiShadowCasterCullPass();
  virtual ~xiiShadowCasterCullPass();
  void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

private:
  xiiSharedPtr<xiiGALBuffer> m_pShadowCasterBuffer; ///< RW: per-cascade visible caster lists (flattened).
};
