/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Pipeline/RenderSettings.h>

/// Typed lighting/clustered policy. Only explicitly overridden properties replace authored defaults.
class XII_GRAPHICSCORE_DLL xiiLightingRenderSettings final : public xiiRenderSubsystemSettings
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLightingRenderSettings, xiiRenderSubsystemSettings);

public:
  xiiUInt32 m_uiTileSize                = 16;
  xiiUInt32 m_uiDepthSlices             = 24;
  xiiUInt32 m_uiMaximumLightsPerCluster = 128;
  bool      m_bLogarithmicDepth         = true;
  bool      m_bReflectionProbes         = true;
};
