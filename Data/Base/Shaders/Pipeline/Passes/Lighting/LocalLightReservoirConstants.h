/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>

#define XII_LOCAL_LIGHT_RESERVOIR_MIN_LIGHTS 8
#define XII_LOCAL_LIGHT_RESERVOIR_TEXELS     2
#define XII_LOCAL_LIGHT_SHADOW_DOMAIN       0x80000000u

DECLARE_CONSTANT_BUFFER_AUTO(xiiLocalLightReservoirConstants)
{
  UINT4(LocalReservoirGrid);          ///< Tile size, frame, previous frame, history available.
  FLOAT4(LocalReservoirReprojection); ///< Previous minus current NDC jitter in xy.
};
