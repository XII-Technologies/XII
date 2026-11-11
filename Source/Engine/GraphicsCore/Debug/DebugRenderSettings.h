/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Pipeline/RenderSettings.h>
#include <GraphicsCore/Pipeline/Declarations.h>

/// Typed debugging policy. Only explicitly overridden properties replace authored defaults.
class XII_GRAPHICSCORE_DLL xiiDebugRenderSettings final : public xiiRenderSubsystemSettings
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDebugRenderSettings, xiiRenderSubsystemSettings);
public:
  bool m_bOverlays = true;
  bool m_bFrameStatistics = false;
  xiiEnum<xiiViewRenderMode> m_Visualization = xiiViewRenderMode::None;
};
