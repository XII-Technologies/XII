#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Texture.h>

/// \brief Pass 61 â€” Color Grading and Filmic Finishing.
///
/// Graphics/Compute. Applies a 3D LUT sampled at the LDR colour, plus:
/// - Vignette (radial darkening toward screen edges).
/// - Grain (temporal noise for analogue feel).
/// - Gamut mapping (Rec.709 or Display-P3 based on output surface caps).
struct XII_GRAPHICSCORE_DLL xiiColorGradingPass
{
  float m_fVignetteStrength  = 0.3f;
  float m_fGrainStrength     = 0.02f;
  float m_fSaturation        = 1.0f;
  float m_fContrast          = 1.0f;
  xiiSharedPtr<xiiGALTexture> m_pColorLUT; ///< Optional 33^3 RGB LUT. If null, identity is used.
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "ColorGradingPass";
  bool      m_bActive = true;
};

void xiiPopulateColorGradingPass(xiiColorGradingPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

