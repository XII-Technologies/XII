#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Pass 20 — Contact Shadow Pass.
///
/// Async Compute. Cheap screen-space ray march along the main light direction
/// to generate per-pixel contact shadow term. Provides micro-occlusion detail
/// that cascade shadow maps miss at typical texel densities.
class XII_GRAPHICSCORE_DLL xiiContactShadowPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiContactShadowPass, xiiRenderPipelinePass);

public:
  xiiContactShadowPass();
  virtual ~xiiContactShadowPass();
  virtual void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard) override;

  xiiUInt32 m_uiRaySteps      = 16u;   ///< Ray march step count (8–32 typical).
  float     m_fMaxRayDistance = 0.5f;  ///< World-space max ray distance.
  float     m_fStrength       = 1.0f;
};
