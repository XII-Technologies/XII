/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Pipeline/RenderSettings.h>

/// Typed resolution policy. Only explicitly overridden properties replace authored defaults.
class XII_GRAPHICSCORE_DLL xiiDynamicResolutionSettings final : public xiiRenderSubsystemSettings
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDynamicResolutionSettings, xiiRenderSubsystemSettings);

public:
  bool      m_bEnabled            = true;
  float     m_fRenderScale        = 1.0f;
  float     m_fTargetFrameTimeMs  = 16.0f;
  float     m_fMinimumRenderScale = 1.0f;
  float     m_fMaximumRenderScale = 1.0f;
  float     m_fProportionalGain   = 0.35f;
  float     m_fIntegralGain       = 0.05f;
  float     m_fDerivativeGain     = 0.15f;
  float     m_fIntegralLimit      = 1.0f;
  float     m_fMaximumScaleStep   = 0.1f;
  float     m_fSmoothing          = 0.2f;
  xiiResult Validate() const override;
};
