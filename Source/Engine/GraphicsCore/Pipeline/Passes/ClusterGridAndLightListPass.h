#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Passes 21-22 â€” Cluster Grid Build and Light List Construction.
///
/// Async Compute. Builds the Forward+ cluster grid from camera frustum and depth range
/// (pass 21), then assigns lights to clusters via a prefix-sum approach (pass 22).
/// Outputs compact cluster descriptors + per-cluster light index buffers consumed
/// by the opaque shading and volumetric passes.
struct XII_GRAPHICSCORE_DLL xiiClusterGridAndLightListPass
{
  xiiUInt32 m_uiClusterCountX  = 16u;
  xiiUInt32 m_uiClusterCountY  = 8u;
  xiiUInt32 m_uiClusterCountZ  = 24u;
  xiiUInt32 m_uiMaxLightsPerCluster = 256u;
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "ClusterGridAndLightListPass";
  bool      m_bActive = true;
};

void xiiPopulateClusterGridAndLightListPass(xiiClusterGridAndLightListPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

