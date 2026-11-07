/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/Passes/Denoising/TemporalSignalDenoisePass.h>

/// Reconstructs stochastic clustered local lighting before direct-light composition.
class XII_GRAPHICSCORE_DLL xiiLocalLightDenoisePass final : public xiiTemporalSignalDenoisePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLocalLightDenoisePass, xiiTemporalSignalDenoisePass);

public:
  xiiLocalLightDenoisePass();
};
