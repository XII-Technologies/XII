#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Pass 14 — Optional Normal-Roughness Prepass.
///
/// Graphics queue. Writes compact normal-roughness GBuffer target used by denoisers
/// and GTAO to improve quality. Skipped if art target does not require it (m_bEnabled = false).
class XII_GRAPHICSCORE_DLL xiiNormalRoughnessPrepassPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiNormalRoughnessPrepassPass, xiiRenderPipelinePass);

public:
  xiiNormalRoughnessPrepassPass();
  virtual ~xiiNormalRoughnessPrepassPass();
  void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

  bool m_bEnabled = true;
};
