/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Pipeline/RenderSettings.h>

/// Typed reflections/screen space policy. Only explicitly overridden properties replace authored defaults.
class XII_GRAPHICSCORE_DLL xiiReflectionRenderSettings final : public xiiRenderSubsystemSettings
{
  XII_ADD_DYNAMIC_REFLECTION(xiiReflectionRenderSettings, xiiRenderSubsystemSettings);

public:
  bool      m_bEnabled           = true;
  float     m_fMaximumDistance   = 50.0f;
  float     m_fSurfaceBias       = 0.01f;
  xiiUInt32 m_uiMaximumSteps     = 96;
  float     m_fThickness         = 0.0025f;
  float     m_fMaximumRoughness  = 0.72f;
  float     m_fStrideDepthCutoff = 0.25f;
  bool      m_bTemporalEnabled   = true;
  float     m_fHistoryWeight     = 0.92f;
  float     m_fConfidenceDecay   = 0.96f;
  float     m_fDepthThreshold    = 0.02f;
  float     m_fNormalThreshold   = 0.95f;
};
