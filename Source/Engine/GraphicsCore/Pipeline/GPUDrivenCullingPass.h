#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/IndirectDrawBatchBuilder.h>
#include <Foundation/Math/Frustum.h>

struct xiiRGPassContext;
class  xiiRenderGraphBlackboard;

// ============================================================
//  GPU-Driven Culling Pass
// ============================================================

/// \brief Two-phase GPU compute culling pass that fills the indirect draw argument buffer.
///
/// ## Algorithm
///
/// **Phase 1 — Early cull (before HZB is available)**
/// - Frustum cull all instances against the six camera planes.
/// - Cone cull meshlet instances against the backface cone stored in xiiMeshletBounds.
/// - Write surviving instance indices into a compact AppendBuffer.
/// - Issue indirect draw / dispatch-mesh calls for early survivors.
///
/// **Phase 2 — Late cull (after early-Z / depth pre-pass produces an HZB)**
/// - Re-test all instances that were NOT drawn in Phase 1 against the HZB.
/// - Append any newly visible instances to the draw buffer.
/// - Emit a second set of indirect draw calls for Phase 2 survivors.
///
/// **Per-meshlet cull** (inside the task/amplification shader):
/// - Frustum test per meshlet bounding sphere.
/// - Backface cone test.
/// - HZB occlusion test (depth sample at expected screen footprint).
///
/// This two-phase approach matches Nanite / Unreal's occlusion strategy.
class XII_GRAPHICSCORE_DLL xiiGPUDrivenCullingPass
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGPUDrivenCullingPass);

public:
  xiiGPUDrivenCullingPass();
  ~xiiGPUDrivenCullingPass();

  void Initialize();
  void Deinitialize();

  /// \brief Executed at the start of each frame before any draw calls.
  ///
  /// \param builder  The batch builder whose instance buffer is already uploaded.
  /// \param ctx      Render graph execution context.
  /// \param frustum  Camera frustum in world space.
  void ExecutePhase1(xiiIndirectDrawBatchBuilder& builder,
                     xiiRGPassContext&             ctx,
                     const xiiFrustum&             frustum);

  /// \brief Executed after the early depth pre-pass; uses the HZB from phase 1.
  void ExecutePhase2(xiiIndirectDrawBatchBuilder& builder,
                     xiiRGPassContext&             ctx);

  // ---- Settings ----
  void SetFrustumCullingEnabled(bool b)   { m_bFrustumCulling   = b; }
  void SetOcclusionCullingEnabled(bool b) { m_bOcclusionCulling = b; }
  void SetMeshletCullingEnabled(bool b)   { m_bMeshletCulling   = b; }

  bool GetFrustumCullingEnabled()   const { return m_bFrustumCulling; }
  bool GetOcclusionCullingEnabled() const { return m_bOcclusionCulling; }
  bool GetMeshletCullingEnabled()   const { return m_bMeshletCulling; }

  /// \brief Returns the number of instances surviving the cull this frame.
  xiiUInt32 GetVisibleInstanceCount() const { return m_uiVisibleInstances; }
  xiiUInt32 GetCulledInstanceCount()  const { return m_uiCulledInstances; }

private:
  void BuildHZB(xiiRGPassContext& ctx);

  // Compute pipelines
  xiiGALPipelineStateHandle m_hFrustumCullPSO;     ///< Phase 1 — frustum + cone cull
  xiiGALPipelineStateHandle m_hOcclusionCullPSO;   ///< Phase 2 — HZB occlusion cull
  xiiGALPipelineStateHandle m_hHZBBuildPSO;        ///< HZB mip chain generation

  // Per-frame GPU buffers
  xiiGALBufferHandle m_hVisibilityBuffer;          ///< One bit per instance (visible / culled)
  xiiGALBufferHandle m_hPhase1DrawCountBuffer;     ///< Atomic counter for Phase 1 surviving draws
  xiiGALBufferHandle m_hPhase2DrawCountBuffer;     ///< Atomic counter for Phase 2 surviving draws

  // Stats
  xiiUInt32 m_uiVisibleInstances = 0;
  xiiUInt32 m_uiCulledInstances  = 0;

  bool m_bFrustumCulling   = true;
  bool m_bOcclusionCulling = true;
  bool m_bMeshletCulling   = true;
  bool m_bInitialised      = false;
};
