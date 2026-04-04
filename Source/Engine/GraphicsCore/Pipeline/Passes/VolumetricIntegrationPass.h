#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Pass 48 â€” Volumetric Lighting Integration.
///
/// Async Compute. Integrates froxel scattering + extinction data accumulated from
/// all contributing lights (directional + local) along each froxel ray segment.
/// Reprojects previous-frame froxel results for temporal stability.
/// Output is composited into the opaque scene HDR in OpaqueCompositePass.
struct XII_GRAPHICSCORE_DLL xiiVolumetricIntegrationPass
{
  float m_fScatteringCoefficient = 0.02f;
  float m_fAbsorptionCoefficient = 0.005f;
  float m_fGlobalDensity         = 1.0f;
  bool  m_bEnableTemporal        = true;
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "VolumetricIntegrationPass";
  bool      m_bActive = true;
};

void xiiPopulateVolumetricIntegrationPass(xiiVolumetricIntegrationPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

