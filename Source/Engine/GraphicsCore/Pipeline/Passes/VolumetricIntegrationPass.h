#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Pass 48 — Volumetric Lighting Integration.
///
/// Async Compute. Integrates froxel scattering + extinction data accumulated from
/// all contributing lights (directional + local) along each froxel ray segment.
/// Reprojects previous-frame froxel results for temporal stability.
/// Output is composited into the opaque scene HDR in OpaqueCompositePass.
class XII_GRAPHICSCORE_DLL xiiVolumetricIntegrationPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVolumetricIntegrationPass, xiiRenderPipelinePass);

public:
  xiiVolumetricIntegrationPass();
  virtual ~xiiVolumetricIntegrationPass();
  virtual void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard) override;

  float m_fScatteringCoefficient = 0.02f;
  float m_fAbsorptionCoefficient = 0.005f;
  float m_fGlobalDensity         = 1.0f;
  bool  m_bEnableTemporal        = true;
};
