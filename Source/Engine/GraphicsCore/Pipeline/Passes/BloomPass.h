#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Passes 57-59 â€” Bloom (Prefilter, Downsample Chain, Upsample + Combine).
///
/// Async Compute. Three-stage physically-based bloom:
/// - Pass 57: Prefilter (bright-pass) â€” extracts bright pixels with a soft knee threshold.
/// - Pass 58: Progressive downsample chain â€” builds a mip pyramid in half-precision.
/// - Pass 59: Upsample and combine â€” tent filter upsample, blends bloom into scene colour.
struct XII_GRAPHICSCORE_DLL xiiBloomPass
{
  float     m_fThreshold     = 1.0f;   ///< Luminance threshold above which bloom is extracted.
  float     m_fKnee          = 0.5f;   ///< Soft-knee width for smooth threshold rolloff.
  float     m_fIntensity     = 0.04f;  ///< Bloom intensity multiplier.
  xiiUInt32 m_uiMipLevels    = 7u;     ///< Number of downsample mip levels.
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "BloomPass";
  bool      m_bActive = true;
};

void xiiPopulateBloomPass(xiiBloomPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

