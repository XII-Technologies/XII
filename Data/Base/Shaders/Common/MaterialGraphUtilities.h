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
