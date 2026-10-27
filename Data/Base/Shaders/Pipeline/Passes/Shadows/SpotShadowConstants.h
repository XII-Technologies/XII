/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiSpotShadowConstants)
{
  MAT4(SpotViewProjection)
  [4];
  FLOAT4(SpotPositionAndTexelScale)
  [4];
  FLOAT4(SpotFadeDistances);
  UINT4(SpotLightIndices);
  FLOAT4(SpotShadowBias);  ///< Normalized reversed depth, world normal bias, normal-bias texels, reserved.
  UINT4(SpotShadowLayout); ///< Active views, tile resolution, atlas columns, atlas rows.
};
