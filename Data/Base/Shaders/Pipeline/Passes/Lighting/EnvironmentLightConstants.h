/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>
DECLARE_CONSTANT_BUFFER_AUTO(xiiEnvironmentLightConstants)
{
  FLOAT4(EnvironmentDirectionAndIlluminance); ///< Direction of travel, illuminance in lux.
  FLOAT4(EnvironmentColorAndAngularRadius); ///< Linear color, sin(source half angle).
  FLOAT4(EnvironmentAmbientRadiance); ///< Linear sky/ambient radiance; w reserved.
};
#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiEnvironmentLightConstants) == 48);
#endif
