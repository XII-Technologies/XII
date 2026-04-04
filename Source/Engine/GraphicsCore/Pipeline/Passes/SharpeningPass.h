#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Pass 62 â€” Sharpening.
///
/// Async Compute / Graphics. Adaptive sharpening applied as a final post-upscale pass.
/// Strength scales with the upscaler mode: more sharpening needed at lower render scales.
/// Uses CAS (Contrast-Adaptive Sharpening) which avoids sharpening noise/grain.
struct XII_GRAPHICSCORE_DLL xiiSharpeningPass
{
  float m_fBaseStrength = 0.3f; ///< Sharpening strength at 100% render scale.
  float m_fScaleBoost   = 0.5f; ///< Additional strength added per 0.1 render scale reduction.
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "SharpeningPass";
  bool      m_bActive = true;
};

void xiiPopulateSharpeningPass(xiiSharpeningPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

