/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>
DECLARE_CONSTANT_BUFFER_AUTO(xiiForwardRefractionConstants)
{
  FLOAT4(RefractionTrace); ///< World ray distance, view-depth hit tolerance, step count, reserved.
};
#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiForwardRefractionConstants) == 16);
#endif
