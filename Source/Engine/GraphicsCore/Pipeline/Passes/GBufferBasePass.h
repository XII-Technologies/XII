#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Pass 27 — GBuffer Base Pass (Deferred Opaque).
///
/// Graphics queue. Main deferred shading payload generation via indirect draws.
/// Writes albedo, oct-encoded normals, material parameters, and depth using
/// the packed draw commands from DrawCommandBuildPass.
class XII_GRAPHICSCORE_DLL xiiGBufferBasePass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGBufferBasePass, xiiRenderPipelinePass);

public:
  xiiGBufferBasePass();
  virtual ~xiiGBufferBasePass();
  void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);
};
