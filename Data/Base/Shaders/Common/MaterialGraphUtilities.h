/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

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
