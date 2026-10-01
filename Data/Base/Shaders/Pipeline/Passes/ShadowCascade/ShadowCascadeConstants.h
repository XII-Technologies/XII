/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Per-frame shadow cascade data used by shadow map rasterization and shadow sampling.
///
/// Written by ShadowCascadeSetup compute pass, read by directional shadow depth passes
/// and DirectLighting compute pass.
DECLARE_CONSTANT_BUFFER_AUTO(xiiShadowCascadeConstants)
{
  MAT4(CascadeViewProjection)
  [4];                        ///< View-projection matrix for each active cascade (up to 4).
  FLOAT4(CascadeSplitDepths); ///< View-space split depths for the 4 cascade boundaries.
  FLOAT4(CascadeWorldRadii);  ///< Half-extent in metres of each cascade's orthographic projection.
  UINT1(ActiveCascadeCount);  ///< Number of valid cascades (1–4).
  FLOAT3(_Pad);
};
