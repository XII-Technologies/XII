#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Passes 29-30 â€” Ambient Occlusion (GTAO) + Denoise.
///
/// Async Compute. Implements Ground Truth Ambient Occlusion (GTAO) with horizon-based
/// sampling of the depth buffer. Followed by temporal + spatial denoise to produce
/// a stable, low-noise AO term. Spatial pattern is TAA-friendly (interleaved sampling).
struct XII_GRAPHICSCORE_DLL xiiAmbientOcclusionPass
{
  xiiUInt32 m_uiSliceCount     = 3u;   ///< Number of GTAO horizon integration slices.
  xiiUInt32 m_uiStepsPerSlice  = 4u;   ///< Steps per integration slice.
  float     m_fRadius          = 0.6f; ///< World-space AO radius in metres.
  float     m_fFalloffStrength = 1.0f;
  bool      m_bEnableTemporal  = true;
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "AmbientOcclusionPass";
  bool      m_bActive = true;
};

void xiiPopulateAmbientOcclusionPass(xiiAmbientOcclusionPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

