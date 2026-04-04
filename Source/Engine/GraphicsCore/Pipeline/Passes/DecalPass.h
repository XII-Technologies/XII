#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Passes 23-24 — Decal Classification and Resolve.
///
/// Async Compute (classification) + Compute (resolve).
/// Pass 23 reads decal volumes and depth to build per-tile decal index lists.
/// Pass 24 applies decal material attributes (albedo, normal, roughness) to
/// GBuffer targets using the tile lists — prefer compute resolve for bandwidth control.
class XII_GRAPHICSCORE_DLL xiiDecalPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDecalPass, xiiRenderPipelinePass);

public:
  xiiDecalPass();
  virtual ~xiiDecalPass();
  void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

  xiiUInt32 m_uiMaxDecals          = 512u;
  xiiUInt32 m_uiMaxDecalsPerTile   = 32u;
  xiiUInt32 m_uiTileSize           = 8u;  ///< Screen-space tile size in pixels.
};
