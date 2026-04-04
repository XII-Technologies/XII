#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Passes 46-47 â€” Direct + Indirect Lighting Combine.
///
/// Graphics/Compute. Evaluates the GGX BRDF using GBuffer data against all
/// visible lights from the cluster list, shadow maps, contact shadows, and
/// optional RT shadows. Adds RT or SSR reflections and AO-modulated indirect
/// from RT GI / probe data. Outputs two separate HDR buffers (direct + indirect)
/// to allow flexible blending in the composite stage.
struct XII_GRAPHICSCORE_DLL xiiLightingCombinePass
{
  bool m_bEnableRTShadows     = true;
  bool m_bEnableContactShadows = true;
  bool m_bEnableRTReflections  = true;
  bool m_bEnableSSRFallback    = true;
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "LightingCombinePass";
  bool      m_bActive = true;
};

void xiiPopulateLightingCombinePass(xiiLightingCombinePass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

