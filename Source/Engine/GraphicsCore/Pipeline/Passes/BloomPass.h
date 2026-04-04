#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Passes 57-59 — Bloom (Prefilter, Downsample Chain, Upsample + Combine).
///
/// Async Compute. Three-stage physically-based bloom:
/// - Pass 57: Prefilter (bright-pass) — extracts bright pixels with a soft knee threshold.
/// - Pass 58: Progressive downsample chain — builds a mip pyramid in half-precision.
/// - Pass 59: Upsample and combine — tent filter upsample, blends bloom into scene colour.
class XII_GRAPHICSCORE_DLL xiiBloomPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBloomPass, xiiRenderPipelinePass);

public:
  xiiBloomPass();
  virtual ~xiiBloomPass();
  void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

  float     m_fThreshold     = 1.0f;   ///< Luminance threshold above which bloom is extracted.
  float     m_fKnee          = 0.5f;   ///< Soft-knee width for smooth threshold rolloff.
  float     m_fIntensity     = 0.04f;  ///< Bloom intensity multiplier.
  xiiUInt32 m_uiMipLevels    = 7u;     ///< Number of downsample mip levels.
};
