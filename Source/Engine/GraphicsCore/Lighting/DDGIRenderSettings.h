/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Lighting/DynamicGlobalIllumination.h>
#include <GraphicsCore/Pipeline/RenderSettings.h>

/// Typed global illumination/ddgi policy. Only explicitly overridden properties replace authored defaults.
class XII_GRAPHICSCORE_DLL xiiDDGIRenderSettings final : public xiiRenderSubsystemSettings
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDDGIRenderSettings, xiiRenderSubsystemSettings);

public:
  xiiDDGISettings m_Probes                        = {};
  bool            m_bDirectionalAtlas             = false;
  float           m_fTraceDistanceInProbeSpacings = 4.0f;
  float           m_fForwardOffset                = 0.0f;
  float           m_fIntensity                    = 1.0f;
  xiiResult       Validate() const override;
};
