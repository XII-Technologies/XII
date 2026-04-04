#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Pass 52 â€” Particles and VFX Composite.
///
/// Graphics/Compute. Simulates and renders particle systems. Split into two material bins:
/// - Opaque-like particles: depth tested, written before transparents.
/// - Blended particles: alpha-blended into the HDR scene color after opaques.
struct XII_GRAPHICSCORE_DLL xiiParticleVFXPass
{
  xiiUInt32 m_uiMaxParticles = 1u << 20u; ///< 1M particles maximum.
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "ParticleVFXPass";
  bool      m_bActive = true;
};

void xiiPopulateParticleVFXPass(xiiParticleVFXPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

