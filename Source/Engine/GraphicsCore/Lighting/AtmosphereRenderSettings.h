/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Pipeline/RenderSettings.h>

/// Typed atmosphere policy. Only explicitly overridden properties replace authored defaults.
class XII_GRAPHICSCORE_DLL xiiAtmosphereRenderSettings final : public xiiRenderSubsystemSettings
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAtmosphereRenderSettings, xiiRenderSubsystemSettings);

public:
  bool      m_bEnabled              = true;
  bool      m_bCacheLookups         = true;
  xiiUInt32 m_uiTransmittanceWidth  = 256;
  xiiUInt32 m_uiTransmittanceHeight = 64;
  xiiUInt32 m_uiMultiScatterWidth   = 32;
  xiiUInt32 m_uiMultiScatterHeight  = 32;
};
