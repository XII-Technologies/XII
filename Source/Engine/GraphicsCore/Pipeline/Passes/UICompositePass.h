#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Pass 63 — UI and Debug Overlay Composite.
///
/// Graphics queue. Composites UI draw data and any optional debug overlays onto the
/// final sharpened backbuffer. Applied last to preserve UI clarity (UI is not
/// affected by any post-process operators). Has side effects and must never be culled.
class XII_GRAPHICSCORE_DLL xiiUICompositePass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiUICompositePass, xiiRenderPipelinePass);

public:
  xiiUICompositePass();
  virtual ~xiiUICompositePass();
  virtual void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard) override;

  bool m_bEnableDebugOverlay = false; ///< Enable GPU performance counters / pass timeline overlay.
};
