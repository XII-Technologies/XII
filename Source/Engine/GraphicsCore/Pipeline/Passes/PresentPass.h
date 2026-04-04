#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Pass 65 â€” Present.
///
/// Graphics queue. Final synchronisation point: transitions the backbuffer to present
/// layout, acquires the swap-chain image, and enqueues presentation. Has mandatory
/// side effects â€” must never be culled by the render graph compiler.
struct XII_GRAPHICSCORE_DLL xiiPresentPass
{
  bool m_bEnableVSync = true;
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "PresentPass";
  bool      m_bActive = true;
};

void xiiPopulatePresentPass(xiiPresentPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

