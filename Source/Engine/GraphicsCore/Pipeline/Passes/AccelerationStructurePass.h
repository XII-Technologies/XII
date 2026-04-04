#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// \brief Passes 32-35 + 45 — Acceleration Structure Management.
///
/// Manages the full BLAS/TLAS lifecycle for ray tracing:
/// - Pass 32: BLAS refit/rebuild scheduling (CPU) — separates static, dynamic, skinned policies.
/// - Pass 33: BLAS build/refit execution (Async Compute or Graphics).
/// - Pass 34: TLAS build (Async Compute / Graphics) — barrier-sensitive.
/// - Pass 35: SBT update (Copy/Compute) — incremental updates only.
/// - Pass 45: AS compaction query and copy (Async Compute / Copy) — saves memory long-term.
///
/// Gracefully disables when the device does not report RT support.
class XII_GRAPHICSCORE_DLL xiiAccelerationStructurePass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAccelerationStructurePass, xiiRenderPipelinePass);

public:
  xiiAccelerationStructurePass();
  virtual ~xiiAccelerationStructurePass();
  void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

  bool m_bAllowRefit      = true;  ///< Use refit for dynamic/skinned where quality allows.
  bool m_bEnableCompaction = true; ///< Issue compaction queries on built BLAS.
};
