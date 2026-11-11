/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Lighting/ShadowCascade.h>
#include <GraphicsCore/Pipeline/RenderSettings.h>

/// Typed shadows policy. Only explicitly overridden properties replace authored defaults.
class XII_GRAPHICSCORE_DLL xiiShadowRenderSettings final : public xiiRenderSubsystemSettings
{
  XII_ADD_DYNAMIC_REFLECTION(xiiShadowRenderSettings, xiiRenderSubsystemSettings);

public:
  bool                     m_bEnabled            = true;
  xiiShadowCascadeSettings m_Cascades            = {};
  xiiUInt32                m_uiSpotResolution    = 1024;
  xiiUInt32                m_uiMaximumSpotLights = 4;
  float                    m_fSpotNearPlane      = 0.02f;
  float                    m_fDepthBias          = 0.0005f;
  float                    m_fNormalBias         = 0.01f;
  float                    m_fNormalBiasTexels   = 1.0f;
};
