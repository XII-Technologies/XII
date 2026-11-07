/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>
DECLARE_CONSTANT_BUFFER_AUTO(xiiRayTracedIrradianceConstants)
{
  FLOAT4(RayDistances); ///< Maximum length and normal bias in world units; remaining values reserved.
  UINT4(PathSamples);   ///< Primary sample count, maximum surface hits, authored sequence offset, analytic-light samples.
};
