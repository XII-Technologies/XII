#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// \brief Pass 7 — Coarse Frustum Culling.
///
/// Async Compute. Tests per-instance AABBs against the six camera frustum planes.
/// This is the first reduction pass before any depth tests. Outputs a compact list
/// of candidate instances that survive frustum rejection.
class XII_GRAPHICSCORE_DLL xiiCoarseFrustumCullPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCoarseFrustumCullPass, xiiRenderPipelinePass);

public:
  xiiCoarseFrustumCullPass();
  virtual ~xiiCoarseFrustumCullPass();

  virtual void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard) override;

private:
  xiiSharedPtr<xiiGALBuffer> m_pVisibleCandidateBuffer; ///< RW: surviving instance indices (uint per instance).
  xiiSharedPtr<xiiGALBuffer> m_pFrustumPlanesBuffer;    ///< CBV: 6 float4 frustum planes, written CPU-side.
};
