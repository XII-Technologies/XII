/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Pipeline/RenderSettings.h>

/// Typed ambient occlusion policy. Only explicitly overridden properties replace authored defaults.
class XII_GRAPHICSCORE_DLL xiiAmbientOcclusionRenderSettings final : public xiiRenderSubsystemSettings
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAmbientOcclusionRenderSettings, xiiRenderSubsystemSettings);

public:
  bool      m_bEnabled               = true;
  bool      m_bUseRayTracing         = true;
  bool      m_bAllowScreenSpace      = true;
  float     m_fRadius                = 2.0f;
  float     m_fRayOriginBias         = 0.005f;
  float     m_fRayMinimumDistance    = 0.001f;
  float     m_fNormalBias            = 0.05f;
  float     m_fPixelStride           = 6.0f;
  float     m_fMinimumSampleDistance = 0.005f;
  xiiUInt32 m_uiDirectionCount       = 8;
  xiiUInt32 m_uiStepCount            = 4;
  xiiResult Validate() const override;
};
