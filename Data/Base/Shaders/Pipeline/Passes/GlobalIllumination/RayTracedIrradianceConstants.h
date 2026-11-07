/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>
DECLARE_CONSTANT_BUFFER_AUTO(xiiRayTracedIrradianceConstants)
{
  FLOAT4(RayDistances);   ///< Maximum length and normal bias in world units, power-weighted light flag, reserved.
  UINT4(EmissiveSamples); ///< Emissive samples per vertex, current graph frame, environment samples per secondary vertex, reserved.
  UINT4(PathSamples);     ///< Primary sample count, maximum surface hits, authored sequence offset, analytic-light samples.
};
