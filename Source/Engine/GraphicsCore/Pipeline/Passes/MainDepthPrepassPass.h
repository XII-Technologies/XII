#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Pass 12 — Main Depth Prepass.
///
/// Graphics queue. Full-resolution depth-only indirect draw using the packed
/// draw command buffer from DrawCommandBuildPass. Outputs the authoritative
/// scene depth used by nearly every subsequent pass.
class XII_GRAPHICSCORE_DLL xiiMainDepthPrepassPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMainDepthPrepassPass, xiiRenderPipelinePass);

public:
  xiiMainDepthPrepassPass();
  virtual ~xiiMainDepthPrepassPass();
  void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);
};
