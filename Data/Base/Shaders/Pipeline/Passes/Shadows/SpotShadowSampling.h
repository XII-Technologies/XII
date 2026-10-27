/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Pipeline/Passes/Shadows/SpotShadowConstants.h>

Texture2D<float> g_SpotShadowAtlas;

bool xiiHasSpotShadow(uint lightIndex)
{
  [loop] for (uint view = 0; view < min(SpotShadowLayout.x, 4u); ++view) if (SpotLightIndices[view] == lightIndex) return true;
  return false;
}

float xiiSampleSpotShadow(uint lightIndex, float3 world, float3 normal, float3 camera)
{
  [loop] for (uint view = 0; view < min(SpotShadowLayout.x, 4u); ++view)
  {
    if (SpotLightIndices[view] != lightIndex)
      continue;
    const float4 unbiased   = mul(SpotViewProjection[view], float4(world, 1));
    const float  texelWorld = max(unbiased.w, 0) * SpotPositionAndTexelScale[view].w;
    const float  offset     = SpotShadowBias.y + texelWorld * SpotShadowBias.z;
    const float4 clip       = mul(SpotViewProjection[view], float4(world + normal * offset, 1));
    if (!all(isfinite(clip)) || clip.w <= 0)
      return 1;
    const float3 projection = clip.xyz / clip.w;
    const float2 uv         = projection.xy * float2(0.5f, -0.5f) + 0.5f;
    if (any(uv < 0) || any(uv > 1) || projection.z < 0 || projection.z > 1)
      return 1;
    const int    resolution     = int(SpotShadowLayout.y);
    const int2   origin         = int2(view % SpotShadowLayout.z, view / SpotShadowLayout.z) * resolution;
    const float2 samplePosition = uv * float(resolution) - 0.5f;
    const int2   base           = int2(floor(samplePosition));
    const float2 fraction       = frac(samplePosition);
    float        visibility     = 0;
    [unroll] for (int y = 0; y < 2; ++y)
      [unroll] for (int x = 0; x < 2; ++x)
    {
      const int2  pixel  = origin + clamp(base + int2(x, y), 0, resolution - 1);
      const float stored = g_SpotShadowAtlas.Load(int3(pixel, 0));
      const float weight = (x ? fraction.x : 1 - fraction.x) * (y ? fraction.y : 1 - fraction.y);
      visibility += (projection.z + SpotShadowBias.x >= stored ? 1.0f : 0.0f) * weight;
    }
    // The CPU resolves an authored zero to the light's effective range. Fade the
    // whole light's shadow over the last twenty percent of that distance.
    const float fadeDistance = SpotFadeDistances[view];
    const float fade         = fadeDistance > 0 ? saturate((distance(camera, SpotPositionAndTexelScale[view].xyz) / fadeDistance - 0.8f) * 5.0f) : 0;
    return lerp(visibility, 1, fade);
  }
  return 1;
}
