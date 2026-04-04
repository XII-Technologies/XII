#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Pass 26 â€” Volumetric Froxel Grid Setup.
///
/// Async Compute. Precomputes froxel (frustum-space voxel) bounds and phase terms
/// for the volumetric lighting integration pass. Froxel layout is deferred until
/// the camera and dynamic-resolution scale are known.
struct XII_GRAPHICSCORE_DLL xiiVolumetricFroxelSetupPass
{
  xiiUInt32 m_uiFroxelCountX   = 160u;
  xiiUInt32 m_uiFroxelCountY   = 90u;
  xiiUInt32 m_uiFroxelCountZ   = 64u;
  float     m_fNearPlane       = 0.1f;
  float     m_fFarPlane        = 300.0f;
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "VolumetricFroxelSetupPass";
  bool      m_bActive = true;
};

void xiiPopulateVolumetricFroxelSetupPass(xiiVolumetricFroxelSetupPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

