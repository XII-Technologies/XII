/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Pipeline/Passes/ShadowCascade/ShadowCascadeConstants.h>
Texture2D<float> g_ShadowAtlas;
float            xiiSampleDirectionalShadow(float3 world, float3 normal, float distance, uint resolution, float3 bias)
{
  float visibility = 1;
  uint  cascade    = 0u;
  while (cascade < ActiveCascadeCount)
  {
    if (distance <= CascadeSplitDepths[cascade])
      break;
    ++cascade;
  }
  if (cascade < ActiveCascadeCount)
  {

    const float  texelWorldSize = 2.0f * CascadeWorldRadii[cascade] / float(resolution);
    const float  normalBias     = bias.y + texelWorldSize * bias.z;
    const float4 clipPosition   = mul(CascadeViewProjection[cascade], float4(world + normal * normalBias, 1));
    const float3 projection     = clipPosition.xyz / clipPosition.w;
    const float2 localUV        = float2(projection.x * 0.5f + 0.5f, 0.5f - projection.y * 0.5f);
    if (all(localUV >= 0) && all(localUV <= 1) && projection.z >= 0 && projection.z <= 1)
    {
      const int    tileSize       = int(resolution);
      const int2   origin         = int2(cascade % 2u, cascade / 2u) * tileSize;
      const float2 samplePosition = localUV * float(tileSize) - 0.5f;
      const int2   base           = int2(floor(samplePosition));
      const float2 fraction       = frac(samplePosition);
      visibility                  = 0;
      // Bilinear comparison filtering uses four explicit texel loads. Each tap is
      // clamped inside its own cascade, so filtering cannot leak between atlas tiles.
      [unroll] for (int y = 0; y < 2; ++y)
        [unroll] for (int x = 0; x < 2; ++x)
      {
        const int2  pixel  = origin + clamp(base + int2(x, y), 0, tileSize - 1);
        const float stored = g_ShadowAtlas.Load(int3(pixel, 0));
        const float weight = (x ? fraction.x : 1 - fraction.x) * (y ? fraction.y : 1 - fraction.y);
        visibility += (projection.z + bias.x >= stored ? 1.0f : 0.0f) * weight;
      }
    }
  }
  return visibility;
}
