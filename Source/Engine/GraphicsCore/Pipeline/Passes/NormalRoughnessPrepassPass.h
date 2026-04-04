#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Pass 14 â€” Optional Normal-Roughness Prepass.
///
/// Graphics queue. Writes compact normal-roughness GBuffer target used by denoisers
/// and GTAO to improve quality. Skipped if art target does not require it (m_bEnabled = false).
struct XII_GRAPHICSCORE_DLL xiiNormalRoughnessPrepassPass
{
  bool m_bEnabled = true;
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "NormalRoughnessPrepassPass";
  bool      m_bActive = true;
};

void xiiPopulateNormalRoughnessPrepassPass(xiiNormalRoughnessPrepassPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

