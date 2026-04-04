#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Pass 12 â€” Main Depth Prepass.
///
/// Graphics queue. Full-resolution depth-only indirect draw using the packed
/// draw command buffer from DrawCommandBuildPass. Outputs the authoritative
/// scene depth used by nearly every subsequent pass.
struct XII_GRAPHICSCORE_DLL xiiMainDepthPrepassPass
{
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "MainDepthPrepassPass";
  bool      m_bActive = true;
};

void xiiPopulateMainDepthPrepassPass(xiiMainDepthPrepassPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

