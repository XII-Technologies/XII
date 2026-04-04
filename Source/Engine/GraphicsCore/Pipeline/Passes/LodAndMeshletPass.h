#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// \brief Pass 6 â€” LOD Selection and Meshlet Classification.
///
/// Async Compute. Uses updated bounds and camera data to select per-instance LOD levels
/// and classify geometry into draw metadata bins. Kept branchless and cache-friendly:
/// one thread per instance, outputs compacted into a tightly-packed structured buffer.
struct XII_GRAPHICSCORE_DLL xiiLodAndMeshletPass
{
  float m_fLOD0Distance = 10.0f;
  float m_fLOD1Distance = 30.0f;
  float m_fLOD2Distance = 80.0f;
  float m_fLOD3Distance = 200.0f;
  xiiSharedPtr<xiiGALBuffer> m_pLODMetadataBuffer; ///< RW: per-instance (lodLevel:8, materialBin:8, flags:16) packed uint.
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "LodAndMeshletPass";
  bool      m_bActive = true;
};

void xiiPopulateLodAndMeshletPass(xiiLodAndMeshletPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

