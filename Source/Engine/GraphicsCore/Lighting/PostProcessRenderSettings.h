/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Lighting/DisplayOutput.h>
#include <GraphicsCore/Pipeline/RenderSettings.h>

/// Typed post-processing and output policy. Only explicitly overridden properties replace authored defaults.
class XII_GRAPHICSCORE_DLL xiiPostProcessRenderSettings final : public xiiRenderSubsystemSettings
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPostProcessRenderSettings, xiiRenderSubsystemSettings);

public:
  xiiDisplayOutputSettings m_Display = {};
  xiiResult                Validate() const override;
};
