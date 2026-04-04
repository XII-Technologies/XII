#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// \brief Pass 11 â€” Draw Indirect Command Build and Compaction.
///
/// Async Compute. Takes surviving visible instances, bins them by material,
/// and writes packed DrawIndexedIndirect argument buffers + draw counts.
/// This is the critical pass enabling GPU-driven rendering â€” the CPU never
/// touches per-draw data after this point.
struct XII_GRAPHICSCORE_DLL xiiDrawCommandBuildPass
{
  xiiUInt32 m_uiMaxDrawCommands = 65536u; ///< Maximum DrawIndexedIndirect commands per frame.
  xiiUInt32 m_uiMaxMaterialBins = 256u;   ///< Maximum distinct material bins.
  xiiSharedPtr<xiiGALBuffer> m_pDrawIndirectArgsBuffer; ///< RW: packed DrawIndexedIndirect structs.
  xiiSharedPtr<xiiGALBuffer> m_pDrawCountBuffer;         ///< RW: per-material-bin draw count (uint).
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "DrawCommandBuildPass";
  bool      m_bActive = true;
};

void xiiPopulateDrawCommandBuildPass(xiiDrawCommandBuildPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

