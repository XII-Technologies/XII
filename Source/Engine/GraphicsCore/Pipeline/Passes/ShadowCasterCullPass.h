#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// \brief Pass 16 â€” Directional Shadow Culling.
///
/// Async Compute. For each cascade volume, tests scene bounds against the cascade
/// AABB and marks casters as visible. Per-cascade cull avoids full overdraw in
/// shadow maps. Outputs per-cascade draw lists consumed by DirectionalShadowRenderPass.
struct XII_GRAPHICSCORE_DLL xiiShadowCasterCullPass
{
  xiiSharedPtr<xiiGALBuffer> m_pShadowCasterBuffer; ///< RW: per-cascade visible caster lists (flattened).
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "ShadowCasterCullPass";
  bool      m_bActive = true;
};

void xiiPopulateShadowCasterCullPass(xiiShadowCasterCullPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

