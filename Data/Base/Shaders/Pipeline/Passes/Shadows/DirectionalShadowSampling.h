/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Pipeline/Passes/ShadowCascade/ShadowCascadeConstants.h>
Texture2D<float> g_ShadowAtlas;

float xiiDirectionalShadowCompare(float2 position, int2 origin, int tileSize, float receiverDepth)
{
  const int2 base = int2(floor(position));
  const float2 fraction = frac(position);
  float visibility = 0;
  // Clamp every tap to its own tile, including the bilinear footprint.
  [unroll] for (int y = 0; y < 2; ++y)
    [unroll] for (int x = 0; x < 2; ++x)
  {
    const int2 pixel = origin + clamp(base + int2(x, y), 0, tileSize - 1);
    const float weight = (x ? fraction.x : 1 - fraction.x) * (y ? fraction.y : 1 - fraction.y);
    visibility += (receiverDepth >= g_ShadowAtlas.Load(int3(pixel, 0)) ? 1.0f : 0.0f) * weight;
  }
  return visibility;
}

float xiiSampleDirectionalCascade(float3 world, float3 normal, uint cascade, uint resolution, float3 bias)
{
  const float texelWorldSize = 2.0f * CascadeWorldRadii[cascade] / float(resolution);
  const float normalBias = bias.y + texelWorldSize * bias.z;
  const float4 clipPosition = mul(CascadeViewProjection[cascade], float4(world + normal * normalBias, 1));
  const float3 projection = clipPosition.xyz / clipPosition.w;
  const float2 localUV = float2(projection.x * 0.5f + 0.5f, 0.5f - projection.y * 0.5f);
  if (any(localUV < 0) || any(localUV > 1) || projection.z < 0 || projection.z > 1)
    return 1;

  const int tileSize = int(resolution);
  const int2 origin = int2(cascade % 2u, cascade / 2u) * tileSize;
  const float2 position = localUV * float(tileSize) - 0.5f;
  float radius = CascadeFiltering.x;
  [branch] if (CascadeFiltering.y > 0)
  {
    // Orthographic reversed depth is linear. Convert blocker separation to metres
    // using the world-to-depth gradient before applying the light's angular size.
    const float depthPerMetre = length(float3(CascadeViewProjection[cascade][2][0], CascadeViewProjection[cascade][2][1], CascadeViewProjection[cascade][2][2]));
    float blockerDepth = 0;
    uint blockers = 0;
    [unroll] for (uint sampleIndex = 0; sampleIndex < 16u; ++sampleIndex)
    {
      const float angle = float(sampleIndex) * 2.39996323f;
      const float2 offset = sampleIndex == 0u ? float2(0, 0) : float2(cos(angle), sin(angle)) * sqrt(float(sampleIndex) / 15.0f) * CascadeFiltering.z;
      const int2 pixel = origin + clamp(int2(floor(position + offset + 0.5f)), 0, tileSize - 1);
      const float stored = g_ShadowAtlas.Load(int3(pixel, 0));
      if (stored > projection.z + bias.x)
      {
        blockerDepth += stored;
        ++blockers;
      }
    }
    if (blockers == 0u)
      return 1;
    const float separation = max(blockerDepth / float(blockers) - projection.z, 0) / max(depthPerMetre, 1e-8f);
    radius = clamp(separation * CascadeFiltering.y / texelWorldSize, radius, CascadeFiltering.z);
  }
  if (radius <= 0)
    return xiiDirectionalShadowCompare(position, origin, tileSize, projection.z + bias.x);

  // Normalized separable tent preserves constant incident energy at every radius.
  float visibility = 0;
  [unroll] for (int y = -1; y <= 1; ++y)
    [unroll] for (int x = -1; x <= 1; ++x)
  {
    const float weight = (x == 0 ? 2.0f : 1.0f) * (y == 0 ? 2.0f : 1.0f) / 16.0f;
    visibility += weight * xiiDirectionalShadowCompare(position + float2(x, y) * radius, origin, tileSize, projection.z + bias.x);
  }
  return visibility;
}

float xiiSampleDirectionalShadow(float3 world, float3 normal, float distance, uint resolution, float3 bias)
{
  uint cascade = 0u;
  while (cascade < ActiveCascadeCount)
  {
    if (distance <= CascadeSplitDepths[cascade])
      break;
    ++cascade;
  }
  if (cascade >= ActiveCascadeCount)
    return 1;

  const float visibility = xiiSampleDirectionalCascade(world, normal, cascade, resolution, bias);
  const float blendStart = CascadeBlendStarts[cascade];
  const float blendWidth = CascadeSplitDepths[cascade] - blendStart;
  if (blendWidth <= 0 || distance <= blendStart)
    return visibility;

  // Fade the final cascade to unshadowed light at the maximum shadow distance.
  float nextVisibility = 1;
  [branch] if (cascade + 1u < ActiveCascadeCount)
    nextVisibility = xiiSampleDirectionalCascade(world, normal, cascade + 1u, resolution, bias);
  return lerp(visibility, nextVisibility, smoothstep(blendStart, CascadeSplitDepths[cascade], distance));
}
