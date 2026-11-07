/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Pipeline/Passes/DDGI/DDGIConstants.h>

float3 xiiDDGIAxis(uint direction)
{
  const float sign = (direction & 1u) == 0u ? 1.0f : -1.0f;
  const uint  axis = direction / 2u;
  return sign * float3(axis == 0u, axis == 1u, axis == 2u);
}

uint2 xiiDDGIDirectionalTexel(uint2 probe, uint direction)
{
  return uint2(probe.x * XII_DDGI_DIRECTION_COUNT + direction, probe.y);
}

float3 xiiDDGIAxisWeights(float3 direction)
{
  const float3 weights = abs(direction);
  return weights / max(dot(weights, 1.0f), 1e-6f);
}

uint xiiDDGISignedAxis(uint axis, float direction)
{
  return axis * 2u + (direction < 0.0f ? 1u : 0u);
}
