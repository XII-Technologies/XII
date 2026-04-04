#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Pass 31 â€” Screen-Space Reflections Fallback.
///
/// Async Compute. Traces screen-space reflection rays using the Hi-Z pyramid for
/// hierarchical DDA ray marching. Produces a reflection radiance term used as a
/// high-quality fallback when the RT reflection budget is exhausted or RT is unavailable.
struct XII_GRAPHICSCORE_DLL xiiScreenSpaceReflectionsPass
{
  xiiUInt32 m_uiMaxRaySteps     = 64u;
  float     m_fRoughnessThreshold = 0.5f; ///< Pixels rougher than this are skipped.
  float     m_fThickness         = 0.05f; ///< Depth intersection thickness (world units).
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "ScreenSpaceReflectionsPass";
  bool      m_bActive = true;
};

void xiiPopulateScreenSpaceReflectionsPass(xiiScreenSpaceReflectionsPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

