#pragma once

#include "../../../Base/Shaders/Common/Platforms.h"

#include "../../../Base/Shaders/Common/ConstantBufferMacros.h"

struct XII_SHADER_STRUCT xiiTestShaderData
{
  FLOAT4(InstanceColor);
  TRANSFORM(InstanceTransform);
};

// this is only defined during shader compilation
#if XII_ENABLED(PLATFORM_SHADER)

StructuredBuffer<xiiTestShaderData> instancingData;

#else // C++

XII_CHECK_AT_COMPILETIME(sizeof(xiiTestShaderData) == 64);

#endif