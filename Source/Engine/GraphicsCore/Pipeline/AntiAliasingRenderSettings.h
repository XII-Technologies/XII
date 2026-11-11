/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Pipeline/RenderSettings.h>

/// Typed anti-aliasing/temporal policy. Only explicitly overridden properties replace authored defaults.
class XII_GRAPHICSCORE_DLL xiiAntiAliasingRenderSettings final : public xiiRenderSubsystemSettings
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAntiAliasingRenderSettings, xiiRenderSubsystemSettings);

public:
  bool  m_bEnabled          = true;
  float m_fDepthThreshold   = 0.01f;
  float m_fBaseBlendAlpha   = 0.08f;
  float m_fMotionBlendScale = 12.0f;
};
