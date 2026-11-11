/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Pipeline/RenderSettings.h>

/// Typed visibility and geometry policy. Only explicitly overridden properties replace authored defaults.
class XII_GRAPHICSCORE_DLL xiiVisibilityRenderSettings final : public xiiRenderSubsystemSettings
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisibilityRenderSettings, xiiRenderSubsystemSettings);

public:
  float m_fLODHysteresis      = 0.1f;
  float m_fLODScreenScale     = 1.0f;
  bool  m_bUseMeshShaders     = true;
  float m_fOcclusionDepthBias = 0.0f;
};
