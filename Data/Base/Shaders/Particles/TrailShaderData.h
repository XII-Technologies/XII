#pragma once

#include "BaseParticleShaderData.h"
#include "ParticleSystemConstants.h"

struct XII_SHADER_STRUCT xiiTrailParticleShaderData
{
  INT1(NumPoints);
  FLOAT3(dummy);
};

struct XII_SHADER_STRUCT xiiTrailParticlePointsData8
{
  FLOAT4(Positions[8]);
};

struct XII_SHADER_STRUCT xiiTrailParticlePointsData16
{
  FLOAT4(Positions[16]);
};

struct XII_SHADER_STRUCT xiiTrailParticlePointsData32
{
  FLOAT4(Positions[32]);
};

struct XII_SHADER_STRUCT xiiTrailParticlePointsData64
{
  FLOAT4(Positions[64]);
};

// this is only defined during shader compilation
#if XII_ENABLED(PLATFORM_SHADER)

DECLARE_STRUCTURED_BUFFER_AUTO(particleTrailData, xiiTrailParticleShaderData);

#  if PARTICLE_TRAIL_POINTS == PARTICLE_TRAIL_POINTS_COUNT8
DECLARE_STRUCTURED_BUFFER_AUTO(particlePointsData, xiiTrailParticlePointsData8);
#  endif

#  if PARTICLE_TRAIL_POINTS == PARTICLE_TRAIL_POINTS_COUNT16
DECLARE_STRUCTURED_BUFFER_AUTO(particlePointsData, xiiTrailParticlePointsData16);
#  endif

#  if PARTICLE_TRAIL_POINTS == PARTICLE_TRAIL_POINTS_COUNT32
DECLARE_STRUCTURED_BUFFER_AUTO(particlePointsData, xiiTrailParticlePointsData32);
#  endif

#  if PARTICLE_TRAIL_POINTS == PARTICLE_TRAIL_POINTS_COUNT64
DECLARE_STRUCTURED_BUFFER_AUTO(particlePointsData, xiiTrailParticlePointsData64);
#  endif

#else // C++

#endif
