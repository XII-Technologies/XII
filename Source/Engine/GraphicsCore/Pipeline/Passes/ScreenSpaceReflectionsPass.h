#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Pass 31 — Screen-Space Reflections Fallback.
///
/// Async Compute. Traces screen-space reflection rays using the Hi-Z pyramid for
/// hierarchical DDA ray marching. Produces a reflection radiance term used as a
/// high-quality fallback when the RT reflection budget is exhausted or RT is unavailable.
class XII_GRAPHICSCORE_DLL xiiScreenSpaceReflectionsPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiScreenSpaceReflectionsPass, xiiRenderPipelinePass);

public:
  xiiScreenSpaceReflectionsPass();
  virtual ~xiiScreenSpaceReflectionsPass();
  void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

  xiiUInt32 m_uiMaxRaySteps     = 64u;
  float     m_fRoughnessThreshold = 0.5f; ///< Pixels rougher than this are skipped.
  float     m_fThickness         = 0.05f; ///< Depth intersection thickness (world units).
};
