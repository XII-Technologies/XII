#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Pass 56 — Upscaling (Internal CAS-based or external DLSS/FSR/XeSS).
///
/// Async Compute / Graphics. Upscales from render resolution to native display resolution.
/// Default implementation uses a Contrast-Adaptive Sharpening (CAS) spatial upscaler.
/// External integrations (DLSS/FSR 3/XeSS) can override by disabling this pass and
/// injecting their own graph node via a plugin mechanism.
class XII_GRAPHICSCORE_DLL xiiUpscalingPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiUpscalingPass, xiiRenderPipelinePass);

public:
  xiiUpscalingPass();
  virtual ~xiiUpscalingPass();
  virtual void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard) override;

  float m_fSharpeningStrength = 0.4f; ///< CAS sharpening amount [0=off, 1=maximum].
};
