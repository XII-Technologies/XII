#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// \brief Passes 32-35 + 45 â€” Acceleration Structure Management.
///
/// Manages the full BLAS/TLAS lifecycle for ray tracing:
/// - Pass 32: BLAS refit/rebuild scheduling (CPU) â€” separates static, dynamic, skinned policies.
/// - Pass 33: BLAS build/refit execution (Async Compute or Graphics).
/// - Pass 34: TLAS build (Async Compute / Graphics) â€” barrier-sensitive.
/// - Pass 35: SBT update (Copy/Compute) â€” incremental updates only.
/// - Pass 45: AS compaction query and copy (Async Compute / Copy) â€” saves memory long-term.
///
/// Gracefully disables when the device does not report RT support.
struct XII_GRAPHICSCORE_DLL xiiAccelerationStructurePass
{
  bool m_bAllowRefit      = true;  ///< Use refit for dynamic/skinned where quality allows.
  bool m_bEnableCompaction = true; ///< Issue compaction queries on built BLAS.
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "AccelerationStructurePass";
  bool      m_bActive = true;
};

void xiiPopulateAccelerationStructurePass(xiiAccelerationStructurePass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

