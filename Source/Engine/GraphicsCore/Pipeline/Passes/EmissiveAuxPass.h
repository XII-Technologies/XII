#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Pass 28 — Emissive and Material Aux Pass.
///
/// Graphics queue. Separate emissive and special-material terms into a dedicated
/// R16G16B16A16F target. Keeping this separate improves culling (many passes have
/// no emissive contribution) and allows independent blending strategies.
class XII_GRAPHICSCORE_DLL xiiEmissiveAuxPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEmissiveAuxPass, xiiRenderPipelinePass);

public:
  xiiEmissiveAuxPass();
  virtual ~xiiEmissiveAuxPass();
  void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);
};
