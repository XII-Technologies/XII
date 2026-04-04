#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Pass 63 â€” UI and Debug Overlay Composite.
///
/// Graphics queue. Composites UI draw data and any optional debug overlays onto the
/// final sharpened backbuffer. Applied last to preserve UI clarity (UI is not
/// affected by any post-process operators). Has side effects and must never be culled.
struct XII_GRAPHICSCORE_DLL xiiUICompositePass
{
  bool m_bEnableDebugOverlay = false; ///< Enable GPU performance counters / pass timeline overlay.
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "UICompositePass";
  bool      m_bActive = true;
};

void xiiPopulateUICompositePass(xiiUICompositePass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

