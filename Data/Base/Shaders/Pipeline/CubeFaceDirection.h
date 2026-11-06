/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

float3 EnvironmentFaceDirection(uint2 pixel, uint face, uint faceExtent)
{
  const float2 uv = (float2(pixel) + 0.5f) * (2.0f / faceExtent) - 1.0f;
  if (face == 0u)
    return normalize(float3(1, -uv.y, -uv.x));
  if (face == 1u)
    return normalize(float3(-1, -uv.y, uv.x));
  if (face == 2u)
    return normalize(float3(uv.x, 1, uv.y));
  if (face == 3u)
    return normalize(float3(uv.x, -1, -uv.y));
  if (face == 4u)
    return normalize(float3(uv.x, -uv.y, 1));
  return normalize(float3(-uv.x, -uv.y, -1));
}
