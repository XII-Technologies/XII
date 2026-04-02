#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Pass 50 — Opaque Final Composite.
///
/// Graphics/Compute. Central fusion point before transparents.
/// Combines: direct lighting + indirect lighting + emissive + volumetric scattering + sky.
/// Output is the authoritative HDR scene color buffer.
class XII_GRAPHICSCORE_DLL xiiOpaqueCompositePass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiOpaqueCompositePass, xiiRenderPipelinePass);

public:
  xiiOpaqueCompositePass();
  virtual ~xiiOpaqueCompositePass();
  virtual void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard) override;
};
