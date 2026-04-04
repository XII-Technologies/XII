#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Pass 20 â€” Contact Shadow Pass.
///
/// Async Compute. Cheap screen-space ray march along the main light direction
/// to generate per-pixel contact shadow term. Provides micro-occlusion detail
/// that cascade shadow maps miss at typical texel densities.
struct XII_GRAPHICSCORE_DLL xiiContactShadowPass
{
  xiiUInt32 m_uiRaySteps      = 16u;   ///< Ray march step count (8â€“32 typical).
  float     m_fMaxRayDistance = 0.5f;  ///< World-space max ray distance.
  float     m_fStrength       = 1.0f;
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "ContactShadowPass";
  bool      m_bActive = true;
};

void xiiPopulateContactShadowPass(xiiContactShadowPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

