#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Pass 65 — Present.
///
/// Graphics queue. Final synchronisation point: transitions the backbuffer to present
/// layout, acquires the swap-chain image, and enqueues presentation. Has mandatory
/// side effects — must never be culled by the render graph compiler.
class XII_GRAPHICSCORE_DLL xiiPresentPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPresentPass, xiiRenderPipelinePass);

public:
  xiiPresentPass();
  virtual ~xiiPresentPass();
  void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

  bool m_bEnableVSync = true;
};
