#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Texture.h>

/// \brief Pass 55 — TAA / Temporal Resolve.
///
/// Async Compute / Graphics. Full Temporal Anti-Aliasing with:
/// - Jitter subpixel sampling (Halton sequence) applied to camera each frame.
/// - Neighbourhood AABB history clamping in YCoCg colour space.
/// - Reactive mask for fast-moving/noisy regions (particles, transparents).
/// - Velocity-weighted reprojection.
/// Persistent history buffer swaps between frames.
class XII_GRAPHICSCORE_DLL xiiTemporalResolvePass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTemporalResolvePass, xiiRenderPipelinePass);

public:
  xiiTemporalResolvePass();
  virtual ~xiiTemporalResolvePass();
  virtual void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard) override;

  float     m_fBlendAlpha          = 0.1f;  ///< Per-frame blend weight [0=full history, 1=no history].
  float     m_fSharpness           = 0.25f; ///< Mitchell-Netravali sharpness kernel weight.
  bool      m_bEnableReactiveMask  = true;
  xiiUInt32 m_uiJitterSequenceLength = 16u;

private:
  xiiSharedPtr<xiiGALTexture> m_pHistoryTexture[2]; ///< Ping-pong history buffers (R16G16B16A16F).
  xiiUInt32                   m_uiCurrentHistory = 0u;
  xiiUInt32                   m_uiFrameIndex     = 0u; ///< Local counter for Halton jitter index.
};
