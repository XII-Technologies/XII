/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/Passes/Denoising/TemporalSignalDenoisePass.h>

/// Material-aware spatiotemporal filtering of incident diffuse irradiance.
class XII_GRAPHICSCORE_DLL xiiIrradianceDenoisePass final : public xiiTemporalSignalDenoisePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiIrradianceDenoisePass, xiiTemporalSignalDenoisePass);

public:
  xiiIrradianceDenoisePass();
};
