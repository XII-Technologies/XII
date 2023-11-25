#pragma once

#include "ParticleSystemConstants.h"
#include "BaseParticleShaderData.h"

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

StructuredBuffer<xiiTrailParticleShaderData> particleTrailData;

#if PARTICLE_TRAIL_POINTS == PARTICLE_TRAIL_POINTS_COUNT8
  StructuredBuffer<xiiTrailParticlePointsData8> particlePointsData;
#endif

#if PARTICLE_TRAIL_POINTS == PARTICLE_TRAIL_POINTS_COUNT16
  StructuredBuffer<xiiTrailParticlePointsData16> particlePointsData;
#endif

#if PARTICLE_TRAIL_POINTS == PARTICLE_TRAIL_POINTS_COUNT32
  StructuredBuffer<xiiTrailParticlePointsData32> particlePointsData;
#endif

#if PARTICLE_TRAIL_POINTS == PARTICLE_TRAIL_POINTS_COUNT64
  StructuredBuffer<xiiTrailParticlePointsData64> particlePointsData;
#endif

#else // C++

#endif

