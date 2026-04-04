#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// \brief Pass 11 — Draw Indirect Command Build and Compaction.
///
/// Async Compute. Takes surviving visible instances, bins them by material,
/// and writes packed DrawIndexedIndirect argument buffers + draw counts.
/// This is the critical pass enabling GPU-driven rendering — the CPU never
/// touches per-draw data after this point.
class XII_GRAPHICSCORE_DLL xiiDrawCommandBuildPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDrawCommandBuildPass, xiiRenderPipelinePass);

public:
  xiiDrawCommandBuildPass();
  virtual ~xiiDrawCommandBuildPass();

  void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

  xiiUInt32 m_uiMaxDrawCommands = 65536u; ///< Maximum DrawIndexedIndirect commands per frame.
  xiiUInt32 m_uiMaxMaterialBins = 256u;   ///< Maximum distinct material bins.

private:
  xiiSharedPtr<xiiGALBuffer> m_pDrawIndirectArgsBuffer; ///< RW: packed DrawIndexedIndirect structs.
  xiiSharedPtr<xiiGALBuffer> m_pDrawCountBuffer;         ///< RW: per-material-bin draw count (uint).
};
