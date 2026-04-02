#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Pass 62 — Sharpening.
///
/// Async Compute / Graphics. Adaptive sharpening applied as a final post-upscale pass.
/// Strength scales with the upscaler mode: more sharpening needed at lower render scales.
/// Uses CAS (Contrast-Adaptive Sharpening) which avoids sharpening noise/grain.
class XII_GRAPHICSCORE_DLL xiiSharpeningPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSharpeningPass, xiiRenderPipelinePass);

public:
  xiiSharpeningPass();
  virtual ~xiiSharpeningPass();
  virtual void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard) override;

  float m_fBaseStrength = 0.3f; ///< Sharpening strength at 100% render scale.
  float m_fScaleBoost   = 0.5f; ///< Additional strength added per 0.1 render scale reduction.
};
