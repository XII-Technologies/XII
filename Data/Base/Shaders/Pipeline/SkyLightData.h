/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiSkyLightConstants)
{
  MAT4(SkyWorldToEnvironment); ///< Rotation only: position and scale do not affect a distant environment.
  FLOAT4(SkyLightParameters);  ///< x = intensity, y = saturation, z = present, w = reserved.
};

#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiSkyLightConstants) == 80U, "Sky light constants must match the shader layout.");
#endif
