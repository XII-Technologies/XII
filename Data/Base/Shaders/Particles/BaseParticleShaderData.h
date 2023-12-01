#pragma once

#include "ParticleSystemConstants.h"

struct XII_SHADER_STRUCT xiiBaseParticleShaderData
{
  PACKEDCOLOR4H(Color);
  PACKEDHALF2(Life, Size, LifeAndSize); // Life: 1 to 0
  UINT1(Variation); // only lower 8 bit
};

// this is only defined during shader compilation
#if XII_ENABLED(PLATFORM_SHADER)

StructuredBuffer<xiiBaseParticleShaderData> particleBaseData;

#else // C++

XII_CHECK_AT_COMPILETIME(sizeof(xiiBaseParticleShaderData) == 16);

#endif

