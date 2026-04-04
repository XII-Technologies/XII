#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Pass 51 — Transparent and Refractive Pass.
///
/// Graphics queue. Sorts transparent objects and composites them over the opaque HDR
/// using depth testing and additive / alpha blending. Refractive objects sample the
/// opaque HDR color buffer. OIT policy (weighted blended / linked-list) is configurable.
class XII_GRAPHICSCORE_DLL xiiTransparentPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTransparentPass, xiiRenderPipelinePass);

public:
  xiiTransparentPass();
  virtual ~xiiTransparentPass();
  void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

  bool m_bUseWeightedBlendedOIT = true; ///< false = depth-sorted back-to-front blending.
};
