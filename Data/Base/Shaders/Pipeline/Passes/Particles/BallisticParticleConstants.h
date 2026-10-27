/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>

struct XII_SHADER_STRUCT xiiParticleTrajectoryBatch
{
  MAT4(LocalToWorld);
  FLOAT4(Tint);
  FLOAT4(SizeAndSoftness); ///< x = radius, y = soft-intersection distance, z = current local-space scale.
  UINT4(Policy);           ///< x = local-space initial conditions, y = material index, zw reserved.
};
DECLARE_CONSTANT_BUFFER_AUTO(xiiBallisticParticleConstants)
{
  FLOAT4(Acceleration);
  UINT4(Dispatch); ///< x = count, y = tiled thread width, zw reserved.
};
#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiParticleTrajectoryBatch) == 112);
static_assert(sizeof(xiiBallisticParticleConstants) == 32);
#endif
