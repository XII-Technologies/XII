#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Passes 23-24 â€” Decal Classification and Resolve.
///
/// Async Compute (classification) + Compute (resolve).
/// Pass 23 reads decal volumes and depth to build per-tile decal index lists.
/// Pass 24 applies decal material attributes (albedo, normal, roughness) to
/// GBuffer targets using the tile lists â€” prefer compute resolve for bandwidth control.
struct XII_GRAPHICSCORE_DLL xiiDecalPass
{
  xiiUInt32 m_uiMaxDecals          = 512u;
  xiiUInt32 m_uiMaxDecalsPerTile   = 32u;
  xiiUInt32 m_uiTileSize           = 8u;  ///< Screen-space tile size in pixels.
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "DecalPass";
  bool      m_bActive = true;
};

void xiiPopulateDecalPass(xiiDecalPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

