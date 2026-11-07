/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/Passes/Denoising/TemporalSignalDenoisePass.h>

/// Ambient visibility specialization of the shared temporal signal filter.
class XII_GRAPHICSCORE_DLL xiiAmbientOcclusionTemporalPass final : public xiiTemporalSignalDenoisePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAmbientOcclusionTemporalPass, xiiTemporalSignalDenoisePass);

public:
  xiiAmbientOcclusionTemporalPass();
};
