#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Passes 39-41 â€” RT Reflection Trace, Temporal Accumulation, Multi-Stage Denoise.
///
/// Async Compute. Traces reflection rays scaled by roughness budget: smooth surfaces
/// get full budget, rough surfaces get fewer rays or are skipped in favour of SSR.
/// Temporal accumulation clamps in radiance space to avoid ghosting.
/// Multi-stage denoise for glossy stability.
/// Requires device RT support â€” no-ops if unavailable.
struct XII_GRAPHICSCORE_DLL xiiRTReflectionPass
{
  xiiUInt32 m_uiMaxRaysPerPixel     = 2u;
  float     m_fMaxRoughnessForRT    = 0.4f; ///< Rougher pixels fall back to SSR.
  xiiUInt32 m_uiTemporalHistoryLen  = 8u;
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "RTReflectionPass";
  bool      m_bActive = true;
};

void xiiPopulateRTReflectionPass(xiiRTReflectionPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

