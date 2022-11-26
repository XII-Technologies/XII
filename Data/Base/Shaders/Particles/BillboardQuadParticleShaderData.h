#pragma once

#include "ParticleSystemConstants.h"
#include "BaseParticleShaderData.h"

struct XII_SHADER_STRUCT xiiBillboardQuadParticleShaderData
{
  FLOAT3(Position);
  PACKEDHALF2(RotationOffset, RotationSpeed, RotationOffsetAndSpeed);
};

// this is only defined during shader compilation
#if XII_ENABLED(PLATFORM_SHADER)

StructuredBuffer<xiiBillboardQuadParticleShaderData> particleBillboardQuadData;

#else // C++

XII_CHECK_AT_COMPILETIME(sizeof(xiiBillboardQuadParticleShaderData) == 16);

#endif

