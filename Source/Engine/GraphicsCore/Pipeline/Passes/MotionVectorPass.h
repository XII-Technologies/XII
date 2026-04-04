#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Pass 13 — Motion Vector Pass.
///
/// Graphics queue. Renders per-pixel screen-space velocity from current and
/// previous-frame transforms. Mandatory for TAA, temporal reprojection, and
/// most denoising passes.
class XII_GRAPHICSCORE_DLL xiiMotionVectorPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMotionVectorPass, xiiRenderPipelinePass);

public:
  xiiMotionVectorPass();
  virtual ~xiiMotionVectorPass();
  void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);
};
