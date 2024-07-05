#pragma once

#include "BaseParticleShaderData.h"
#include "ParticleSystemConstants.h"

struct XII_SHADER_STRUCT xiiBillboardQuadParticleShaderData
{
  FLOAT3(Position);
  PACKEDHALF2(RotationOffset, RotationSpeed, RotationOffsetAndSpeed);
};

// this is only defined during shader compilation
#if XII_ENABLED(PLATFORM_SHADER)

StructuredBuffer<xiiBillboardQuadParticleShaderData> particleBillboardQuadData;

#else // C++

static_assert(sizeof(xiiBillboardQuadParticleShaderData) == 16);

#endif
