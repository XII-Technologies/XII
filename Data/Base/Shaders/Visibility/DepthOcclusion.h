/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
// Shared conservative projection/coverage tests for independent instance and meshlet operations.
bool KeepWithoutOcclusion(GpuSceneInstance instance)
{
  // A temporal client must invalidate stale occluders. Moving/posed candidates are also kept conservatively.
  if ((instance.Flags & 16u) != 0u || !all(isfinite(instance.BoundsCenterRadius.xyz)) ||
      !all(isfinite(instance.BoundsExtents.xyz)) || any(instance.BoundsExtents.xyz < 0.0f) || instance.SkinningCount != 0u || instance.PreviousSkinningCount != 0u ||
      any(instance.GlobalTransform[0] != instance.PreviousGlobalTransform[0]) ||
      any(instance.GlobalTransform[1] != instance.PreviousGlobalTransform[1]) ||
      any(instance.GlobalTransform[2] != instance.PreviousGlobalTransform[2]) ||
      any(instance.GlobalTransform[3] != instance.PreviousGlobalTransform[3])) return true;
  return false;
}

bool BoundsVisibleInDepth(Texture2D<float> depth, float3 center, float3 extents, float4x4 projection, float bias)
{
  if (!all(isfinite(center)) || !all(isfinite(extents)) || any(extents < 0.0f)) return true;
  float2 lo = 1.0f, hi = 0.0f;
  float nearest = 0.0f;
  [unroll] for (uint corner = 0; corner < 8; ++corner)
  {
    const float3 sign = float3((corner & 1u) ? 1 : -1, (corner & 2u) ? 1 : -1, (corner & 4u) ? 1 : -1);
    const float4 clip = mul(projection, float4(center + sign * extents, 1));
    if (!all(isfinite(clip)) || clip.w <= 0.0001f || clip.z < 0.0f || clip.z > clip.w) return true;
    const float3 ndc = clip.xyz / clip.w;
    const float2 uv = ndc.xy * float2(0.5f, -0.5f) + 0.5f;
    lo = min(lo, uv); hi = max(hi, uv); nearest = max(nearest, ndc.z);
  }
  // Outside-image coverage is unknown, especially when a graph supplies another camera's image.
  if (any(lo < 0.0f) || any(hi > 1.0f) || any(hi < lo)) return true;
  uint width, height, levels;
  depth.GetDimensions(0, width, height, levels);
  if (!width || !height || !levels) return true;
  const float extent = max((hi.x - lo.x) * width, (hi.y - lo.y) * height);
  uint mip = min((uint)max(0.0f, ceil(log2(max(extent, 1.0f)))), levels - 1u);
  int2 first, last;
  for (;;)
  {
    uint ignored;
    depth.GetDimensions(mip, width, height, ignored);
    const int2 maximum = int2(width - 1u, height - 1u);
    first = clamp(int2(lo * float2(width, height)), int2(0, 0), maximum);
    last = clamp(int2(hi * float2(width, height)), int2(0, 0), maximum);
    // Four corners cover the complete rectangle only when it spans at most two texels per axis.
    if (all(last - first <= 1)) break;
    if (mip + 1u == levels) return true;
    ++mip;
  }
  const float farthest = min(min(depth.Load(int3(first, mip)), depth.Load(int3(last.x, first.y, mip))),
                             min(depth.Load(int3(first.x, last.y, mip)), depth.Load(int3(last, mip))));
  return !isfinite(farthest) || nearest >= farthest - bias;
}
