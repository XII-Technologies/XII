/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Visibility/GpuSceneCommon.h>

float3 xiiMaterialProject(float4x4 transform, float3 position)
{
  const float4 projected = mul(transform, float4(position, 1));
  return abs(projected.w) > 1e-7f ? projected.xyz / projected.w : 0;
}

float3 xiiMaterialWorldToObject(GpuSceneInstance instance, float3 value, bool position)
{
  if (position)
    value -= float3(instance.GlobalTransform[0][3], instance.GlobalTransform[1][3], instance.GlobalTransform[2][3]);
  // Normal rows contain the inverse transpose. Row-vector multiplication
  // applies the inverse linear transform without another per-pixel inverse.
  return mul(value, float3x3(instance.NormalTransformRow0.xyz, instance.NormalTransformRow1.xyz, instance.NormalTransformRow2.xyz));
}

float2 xiiMaterialRotateUV(float2 uv, float angle)
{
  float sine, cosine;
  sincos(angle, sine, cosine);
  return float2(cosine * uv.x - sine * uv.y, sine * uv.x + cosine * uv.y);
}

float xiiMaterialNoiseHash(float2 position)
{
  float3 value = frac(float3(position.xyx) * 0.1031f);
  value += dot(value, value.yzx + 33.33f);
  return frac((value.x + value.y) * value.z);
}

// Smooth value noise has four scalar hashes, no resources and no shader permutations.
float xiiMaterialValueNoise2D(float2 position)
{
  float2 cell   = floor(position);
  float2 weight = frac(position);
  weight        = weight * weight * (3.0f - 2.0f * weight);
  return lerp(lerp(xiiMaterialNoiseHash(cell), xiiMaterialNoiseHash(cell + float2(1, 0)), weight.x),
              lerp(xiiMaterialNoiseHash(cell + float2(0, 1)), xiiMaterialNoiseHash(cell + float2(1, 1)), weight.x), weight.y);
}

float3 xiiMaterialSafeNormal(float3 value)
{
  const float squaredLength = dot(value, value);
  return squaredLength > 1e-12f ? value * rsqrt(squaredLength) : float3(0, 0, 1);
}

float xiiMaterialFractalNoise2D(float2 position)
{
  float result = 0;
  float weight = 0.53333333f;
  [unroll] for (uint octave = 0; octave < 4; ++octave)
  {
    result += weight * xiiMaterialValueNoise2D(position);
    position = position * 2.03f + float2(17.1f, 9.2f);
    weight *= 0.5f;
  }
  return result;
}

float xiiMaterialVoronoi2D(float2 position)
{
  const float2 cell            = floor(position);
  const float2 local           = frac(position);
  float        distanceSquared = 8;
  [unroll] for (int y = -1; y <= 1; ++y)
  {
    [unroll] for (int x = -1; x <= 1; ++x)
    {
      const float2 neighbor = float2(x, y);
      const float2 seed     = float2(xiiMaterialNoiseHash(cell + neighbor), xiiMaterialNoiseHash(cell + neighbor + float2(37, 59)));
      const float2 delta    = neighbor + seed - local;
      distanceSquared       = min(distanceSquared, dot(delta, delta));
    }
  }
  return sqrt(distanceSquared);
}

float4 xiiMaterialSampleTriplanar(Texture2D tex, SamplerState samplerState, float3 worldNormal, float3 worldPosition, float tiling)
{
  float3 blendWeights = abs(worldNormal);
  blendWeights        = max((blendWeights - 0.2) * 7.0, 0.0);
  blendWeights /= (blendWeights.x + blendWeights.y + blendWeights.z);

  float3 ns = sign(worldNormal) * tiling;

  float4 color1 = tex.Sample(samplerState, worldPosition.yz * float2(-ns.x, -tiling));
  float4 color2 = tex.Sample(samplerState, worldPosition.xz * float2(ns.y, -tiling));
  float4 color3 = tex.Sample(samplerState, worldPosition.xy * float2(ns.z, tiling));

  return color1 * blendWeights.x + color2 * blendWeights.y + color3 * blendWeights.z;
}

float3 xiiMaterialBlendNormals(float3 baseNormal, float3 detailNormal)
{
  float3 t = baseNormal + float3(0, 0, 1);
  float3 u = detailNormal * float3(-1, -1, 1);
  return t * dot(t, u) - u * t.z;
}
