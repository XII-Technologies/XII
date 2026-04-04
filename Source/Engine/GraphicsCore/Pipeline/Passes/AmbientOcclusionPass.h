#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Passes 29-30 — Ambient Occlusion (GTAO) + Denoise.
///
/// Async Compute. Implements Ground Truth Ambient Occlusion (GTAO) with horizon-based
/// sampling of the depth buffer. Followed by temporal + spatial denoise to produce
/// a stable, low-noise AO term. Spatial pattern is TAA-friendly (interleaved sampling).
class XII_GRAPHICSCORE_DLL xiiAmbientOcclusionPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAmbientOcclusionPass, xiiRenderPipelinePass);

public:
  xiiAmbientOcclusionPass();
  virtual ~xiiAmbientOcclusionPass();
  void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

  xiiUInt32 m_uiSliceCount     = 3u;   ///< Number of GTAO horizon integration slices.
  xiiUInt32 m_uiStepsPerSlice  = 4u;   ///< Steps per integration slice.
  float     m_fRadius          = 0.6f; ///< World-space AO radius in metres.
  float     m_fFalloffStrength = 1.0f;
  bool      m_bEnableTemporal  = true;
};
