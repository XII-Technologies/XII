#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Pass 52 — Particles and VFX Composite.
///
/// Graphics/Compute. Simulates and renders particle systems. Split into two material bins:
/// - Opaque-like particles: depth tested, written before transparents.
/// - Blended particles: alpha-blended into the HDR scene color after opaques.
class XII_GRAPHICSCORE_DLL xiiParticleVFXPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleVFXPass, xiiRenderPipelinePass);

public:
  xiiParticleVFXPass();
  virtual ~xiiParticleVFXPass();
  void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

  xiiUInt32 m_uiMaxParticles = 1u << 20u; ///< 1M particles maximum.
};
