#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Pass 26 — Volumetric Froxel Grid Setup.
///
/// Async Compute. Precomputes froxel (frustum-space voxel) bounds and phase terms
/// for the volumetric lighting integration pass. Froxel layout is deferred until
/// the camera and dynamic-resolution scale are known.
class XII_GRAPHICSCORE_DLL xiiVolumetricFroxelSetupPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVolumetricFroxelSetupPass, xiiRenderPipelinePass);

public:
  xiiVolumetricFroxelSetupPass();
  virtual ~xiiVolumetricFroxelSetupPass();
  virtual void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard) override;

  xiiUInt32 m_uiFroxelCountX   = 160u;
  xiiUInt32 m_uiFroxelCountY   = 90u;
  xiiUInt32 m_uiFroxelCountZ   = 64u;
  float     m_fNearPlane       = 0.1f;
  float     m_fFarPlane        = 300.0f;
};
