#pragma once

#include "BaseParticleShaderData.h"

struct XII_SHADER_STRUCT xiiTangentQuadParticleShaderData
{
  FLOAT3(Position);
  FLOAT1(dummy1);

  FLOAT3(TangentX);
  FLOAT1(dummy2);

  FLOAT3(TangentZ);
  FLOAT1(dummy3);
};

// this is only defined during shader compilation
#if XII_ENABLED(PLATFORM_SHADER)

DECLARE_STRUCTURED_BUFFER_AUTO(particleTangentQuadData, xiiTangentQuadParticleShaderData);

#else // C++

static_assert(sizeof(xiiTangentQuadParticleShaderData) == 48);

#endif
