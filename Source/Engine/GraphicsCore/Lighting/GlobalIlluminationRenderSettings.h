/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Pipeline/RenderConfiguration.h>
#include <GraphicsCore/Pipeline/RenderSettings.h>

/// Typed global illumination policy. Only explicitly overridden properties replace authored defaults.
class XII_GRAPHICSCORE_DLL xiiGlobalIlluminationRenderSettings final : public xiiRenderSubsystemSettings
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGlobalIlluminationRenderSettings, xiiRenderSubsystemSettings);

public:
  xiiEnum<xiiRenderGlobalIllumination> m_Technique                 = xiiRenderGlobalIllumination::PipelineDefault;
  float                                m_fScreenRayLength          = 2.0f;
  float                                m_fScreenThickness          = 0.15f;
  float                                m_fScreenSurfaceBias        = 0.02f;
  xiiUInt32                            m_uiScreenSampleCount       = 8;
  xiiUInt32                            m_uiScreenStepCount         = 6;
  bool                                 m_bScreenAnimateSamples     = false;
  float                                m_fRayLength                = 20.0f;
  float                                m_fRaySurfaceBias           = 0.02f;
  xiiUInt32                            m_uiRaySampleCount          = 8;
  xiiUInt32                            m_uiRayMaximumHits          = 2;
  xiiUInt32                            m_uiAnalyticLightSamples    = 4;
  xiiUInt32                            m_uiEmissiveLightSamples    = 0;
  xiiUInt32                            m_uiEnvironmentLightSamples = 0;
  bool                                 m_bPowerWeightedLights      = true;
  bool                                 m_bRayAnimateSamples        = false;
  bool                                 m_bPathReservoirs           = false;
};
