#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Passes 39-41 — RT Reflection Trace, Temporal Accumulation, Multi-Stage Denoise.
///
/// Async Compute. Traces reflection rays scaled by roughness budget: smooth surfaces
/// get full budget, rough surfaces get fewer rays or are skipped in favour of SSR.
/// Temporal accumulation clamps in radiance space to avoid ghosting.
/// Multi-stage denoise for glossy stability.
/// Requires device RT support — no-ops if unavailable.
class XII_GRAPHICSCORE_DLL xiiRTReflectionPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRTReflectionPass, xiiRenderPipelinePass);

public:
  xiiRTReflectionPass();
  virtual ~xiiRTReflectionPass();
  void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

  xiiUInt32 m_uiMaxRaysPerPixel     = 2u;
  float     m_fMaxRoughnessForRT    = 0.4f; ///< Rougher pixels fall back to SSR.
  xiiUInt32 m_uiTemporalHistoryLen  = 8u;
};
