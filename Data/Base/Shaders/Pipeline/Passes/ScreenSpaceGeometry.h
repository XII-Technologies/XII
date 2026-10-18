/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Pipeline/Passes/Lighting/SurfaceLightingConstants.h>
bool xiiScreenWorldPosition(float2 uv, float depth, out float3 position)
{
  const float4 world = mul(SurfaceInverseViewProjection, float4(uv.x * 2 - 1, 1 - uv.y * 2, depth, 1));
  position           = 0;
  if (!all(isfinite(world)) || abs(world.w) < 1e-8f)
    return false;
  position = world.xyz / world.w;
  return all(isfinite(position));
}
float3 xiiScreenNormal(float2 encoded)
{
  const float2 oct    = encoded * 2 - 1;
  float3       normal = float3(oct, 1 - abs(oct.x) - abs(oct.y));
  if (normal.z < 0)
    normal.xy = (1 - abs(normal.yx)) * float2(normal.x >= 0 ? 1 : -1, normal.y >= 0 ? 1 : -1);
  return normalize(normal);
}
float xiiScreenLinearDepth(float depth) { return SurfaceCameraPositionNear.w / max(depth, 1e-7f); }
