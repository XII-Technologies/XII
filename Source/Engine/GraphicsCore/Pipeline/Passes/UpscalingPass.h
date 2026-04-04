#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Pass 56 â€” Upscaling (Internal CAS-based or external DLSS/FSR/XeSS).
///
/// Async Compute / Graphics. Upscales from render resolution to native display resolution.
/// Default implementation uses a Contrast-Adaptive Sharpening (CAS) spatial upscaler.
/// External integrations (DLSS/FSR 3/XeSS) can override by disabling this pass and
/// injecting their own graph node via a plugin mechanism.
struct XII_GRAPHICSCORE_DLL xiiUpscalingPass
{
  float m_fSharpeningStrength = 0.4f; ///< CAS sharpening amount [0=off, 1=maximum].
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "UpscalingPass";
  bool      m_bActive = true;
};

void xiiPopulateUpscalingPass(xiiUpscalingPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

