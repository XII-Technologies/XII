#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Passes 36-38 â€” RT Shadow Trace, Temporal Reprojection, Spatial Denoise.
///
/// Async Compute. Traces one shadow ray per pixel per light against the TLAS.
/// Budget is aggressively scaled by pixel category (sky, analytical shadow, etc).
/// Temporal reprojection with robust disocclusion detection is mandatory.
/// Edge-aware spatial filter uses normal + depth gates for clean penumbra.
/// Requires device RT support â€” no-ops if unavailable.
struct XII_GRAPHICSCORE_DLL xiiRTShadowPass
{
  xiiUInt32 m_uiRaysPerPixel        = 1u;    ///< Rays per pixel (1 = hard shadows, 4+ = soft).
  float     m_fLightRadius          = 0.05f; ///< Angular radius for soft shadows (radians).
  xiiUInt32 m_uiTemporalHistoryLen  = 16u;   ///< Temporal accumulation frames.
  xiiUInt32 m_uiSpatialBlurRadius   = 3u;    ///< Spatial denoise kernel radius in pixels.
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "RTShadowPass";
  bool      m_bActive = true;
};

void xiiPopulateRTShadowPass(xiiRTShadowPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

