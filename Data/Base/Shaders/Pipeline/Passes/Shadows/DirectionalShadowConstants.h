/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>
DECLARE_CONSTANT_BUFFER_AUTO(xiiDirectionalShadowConstants)
{
  UINT4(ShadowExtent); ///< Surface width/height, per-cascade resolution, receiver count.
  FLOAT4(ShadowBias);  ///< Normalized reversed-depth bias, world normal bias, texel normal bias, reserved.
};
