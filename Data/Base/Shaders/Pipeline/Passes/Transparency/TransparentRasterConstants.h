/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>
DECLARE_CONSTANT_BUFFER_AUTO(xiiTransparentRasterConstants)
{
  FLOAT4(TransparentWeightsAndScale); ///< Depth scale, maximum weight, lighting scale, reserved.
  FLOAT4(TransparentShadowBias);      ///< Depth bias, normal bias, normal-bias texels, cascade resolution.
};
#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiTransparentRasterConstants) == 32);
#endif
