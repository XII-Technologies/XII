#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Passes 21-22 — Cluster Grid Build and Light List Construction.
///
/// Async Compute. Builds the Forward+ cluster grid from camera frustum and depth range
/// (pass 21), then assigns lights to clusters via a prefix-sum approach (pass 22).
/// Outputs compact cluster descriptors + per-cluster light index buffers consumed
/// by the opaque shading and volumetric passes.
class XII_GRAPHICSCORE_DLL xiiClusterGridAndLightListPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiClusterGridAndLightListPass, xiiRenderPipelinePass);

public:
  xiiClusterGridAndLightListPass();
  virtual ~xiiClusterGridAndLightListPass();
  void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

  xiiUInt32 m_uiClusterCountX  = 16u;
  xiiUInt32 m_uiClusterCountY  = 8u;
  xiiUInt32 m_uiClusterCountZ  = 24u;
  xiiUInt32 m_uiMaxLightsPerCluster = 256u;
};
