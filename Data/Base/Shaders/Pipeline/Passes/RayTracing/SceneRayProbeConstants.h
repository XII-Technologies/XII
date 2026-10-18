/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>
DECLARE_CONSTANT_BUFFER_AUTO(xiiSceneRayProbeConstants)
{
  FLOAT4(OriginAndMinimumDistance);
  FLOAT4(DirectionAndMaximumDistance);
  UINT4(InstanceMaskAndPadding);
};
/// Identity: hit flag, generation-validated world GPU identity, primitive index, hardware hit kind.
/// Intersection: metric distance, barycentric U/V, reserved.
struct XII_SHADER_STRUCT xiiSceneRayProbeResult
{
  UINT4(Identity);
  FLOAT4(Intersection);
};
#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiSceneRayProbeResult) == 32);
#endif
