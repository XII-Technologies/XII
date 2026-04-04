#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// \brief Pass 10 — Hi-Z Occlusion Culling.
///
/// Async Compute. Tests candidate instances (from coarse frustum cull) against the Hi-Z pyramid.
/// Culls instances whose projected bounds are fully occluded. Large performance win at scene scale.
class XII_GRAPHICSCORE_DLL xiiHiZOcclusionCullPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiHiZOcclusionCullPass, xiiRenderPipelinePass);

public:
  xiiHiZOcclusionCullPass();
  virtual ~xiiHiZOcclusionCullPass();

  void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

private:
  xiiSharedPtr<xiiGALBuffer> m_pSurvivingInstanceBuffer; ///< RW: instances that passed Hi-Z test.
};
