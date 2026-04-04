#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// \brief Pass 5 — Instance Transform and Bounds Update.
///
/// Async Compute. Reads scene transform data and animation outputs (skinning results),
/// writes world matrices and updated axis-aligned bounds used by all culling passes.
class XII_GRAPHICSCORE_DLL xiiInstanceTransformPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiInstanceTransformPass, xiiRenderPipelinePass);

public:
  xiiInstanceTransformPass();
  virtual ~xiiInstanceTransformPass();

  void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

private:
  xiiSharedPtr<xiiGALBuffer> m_pWorldMatrixBuffer;  ///< RW: float4x3 per-instance world matrices.
  xiiSharedPtr<xiiGALBuffer> m_pBoundsBuffer;        ///< RW: float4 center + float4 extents per instance (AABB).
  xiiSharedPtr<xiiGALBuffer> m_pSceneTransformBuffer; ///< SRV: CPU-uploaded raw scene node transforms.
};
