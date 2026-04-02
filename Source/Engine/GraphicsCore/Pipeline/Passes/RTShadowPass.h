#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Passes 36-38 — RT Shadow Trace, Temporal Reprojection, Spatial Denoise.
///
/// Async Compute. Traces one shadow ray per pixel per light against the TLAS.
/// Budget is aggressively scaled by pixel category (sky, analytical shadow, etc).
/// Temporal reprojection with robust disocclusion detection is mandatory.
/// Edge-aware spatial filter uses normal + depth gates for clean penumbra.
/// Requires device RT support — no-ops if unavailable.
class XII_GRAPHICSCORE_DLL xiiRTShadowPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRTShadowPass, xiiRenderPipelinePass);

public:
  xiiRTShadowPass();
  virtual ~xiiRTShadowPass();
  virtual void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard) override;

  xiiUInt32 m_uiRaysPerPixel        = 1u;    ///< Rays per pixel (1 = hard shadows, 4+ = soft).
  float     m_fLightRadius          = 0.05f; ///< Angular radius for soft shadows (radians).
  xiiUInt32 m_uiTemporalHistoryLen  = 16u;   ///< Temporal accumulation frames.
  xiiUInt32 m_uiSpatialBlurRadius   = 3u;    ///< Spatial denoise kernel radius in pixels.
};
