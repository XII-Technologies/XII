#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Passes 17 â€” Directional Shadow Rendering.
///
/// Graphics queue. Renders per-cascade shadow maps into a texture atlas using
/// indirect draws from the shadow caster draw lists. Uses atlas packing and
/// stable texel snapping to minimize shadow shimmer.
struct XII_GRAPHICSCORE_DLL xiiDirectionalShadowRenderPass
{
  xiiUInt32 m_uiAtlasSize = 4096u; ///< Shadow atlas resolution (square).
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "DirectionalShadowRenderPass";
  bool      m_bActive = true;
};

void xiiPopulateDirectionalShadowRenderPass(xiiDirectionalShadowRenderPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

