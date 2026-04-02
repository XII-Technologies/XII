#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Texture.h>

/// \brief Pass 61 — Color Grading and Filmic Finishing.
///
/// Graphics/Compute. Applies a 3D LUT sampled at the LDR colour, plus:
/// - Vignette (radial darkening toward screen edges).
/// - Grain (temporal noise for analogue feel).
/// - Gamut mapping (Rec.709 or Display-P3 based on output surface caps).
class XII_GRAPHICSCORE_DLL xiiColorGradingPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiColorGradingPass, xiiRenderPipelinePass);

public:
  xiiColorGradingPass();
  virtual ~xiiColorGradingPass();
  virtual void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard) override;

  float m_fVignetteStrength  = 0.3f;
  float m_fGrainStrength     = 0.02f;
  float m_fSaturation        = 1.0f;
  float m_fContrast          = 1.0f;

private:
  xiiSharedPtr<xiiGALTexture> m_pColorLUT; ///< Optional 33^3 RGB LUT. If null, identity is used.
};
