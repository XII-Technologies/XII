/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Pipeline/SkyLightData.h>

TextureCube<float4>     g_GIEnvironment;
SamplerState            g_sGIEnvironmentLinear;
StructuredBuffer<float> g_EnvironmentCDF;
StructuredBuffer<float> g_EnvironmentRowCDF;

float3 IrradianceEnvironmentDirection(float3 worldDirection)
{
  const float3 local = mul((float3x3)SkyWorldToEnvironment, worldDirection);
  return float3(local.x, local.z, -local.y);
}

float3 IrradianceEnvironmentRadiance(float3 worldDirection)
{
  float3 value          = g_GIEnvironment.SampleLevel(g_sGIEnvironmentLinear, IrradianceEnvironmentDirection(worldDirection), 0).rgb;
  value                 = all(isfinite(value)) ? max(value, 0) : 0;
  const float luminance = dot(value, float3(0.2126f, 0.7152f, 0.0722f));
  return lerp(luminance.xxx, value, SkyLightParameters.y) * SkyLightParameters.x;
}

float IrradianceEnvironmentCellProbability(uint x, uint y)
{
  const float total = g_EnvironmentRowCDF[31];
  if (total <= 0 || !isfinite(total))
    return 1.0f / 2048.0f;
  const float row      = g_EnvironmentRowCDF[y] - (y > 0 ? g_EnvironmentRowCDF[y - 1] : 0);
  const uint  index    = y * 64 + x;
  const float mass     = g_EnvironmentCDF[index] - (x > 0 ? g_EnvironmentCDF[index - 1] : 0);
  const float rowTotal = g_EnvironmentCDF[y * 64 + 63];
  // Use the actual stored interval widths, including scan rounding. The uniform
  // component preserves support for sub-cell detail and intervals rounded away.
  return 0.05f / 2048.0f + (rowTotal > 0 ? 0.95f * (row / total) * (mass / rowTotal) : 0);
}

float IrradianceEnvironmentPDF(float3 worldDirection)
{
  const float3 direction = IrradianceEnvironmentDirection(worldDirection);
  float        phi       = atan2(direction.z, direction.x);
  if (phi < 0)
    phi += 6.28318530718f;
  const uint x = min(uint(phi * (64.0f / 6.28318530718f)), 63u);
  const uint y = min(uint(saturate((1 - direction.y) * 0.5f) * 32.0f), 31u);
  return IrradianceEnvironmentCellProbability(x, y) * (2048.0f / 12.56637061436f);
}

float3 IrradianceSampleEnvironment(inout uint randomState, out float pdf)
{
  uint        x, y;
  const float total = g_EnvironmentRowCDF[31];
  if (IrradianceRandom(randomState) < 0.95f && total > 0 && isfinite(total))
  {
    float target = min(IrradianceRandom(randomState) * total, asfloat(asuint(total) - 1u));
    uint  lo = 0, hi = 31;
    while (lo < hi)
    {
      const uint mid = (lo + hi) / 2;
      if (g_EnvironmentRowCDF[mid] <= target)
        lo = mid + 1;
      else
        hi = mid;
    }
    y                    = lo;
    const float rowTotal = g_EnvironmentCDF[y * 64 + 63];
    target               = min(IrradianceRandom(randomState) * rowTotal, asfloat(asuint(rowTotal) - 1u));
    lo                   = 0;
    hi                   = 63;
    while (lo < hi)
    {
      const uint mid = (lo + hi) / 2;
      if (g_EnvironmentCDF[y * 64 + mid] <= target)
        lo = mid + 1;
      else
        hi = mid;
    }
    x = lo;
  }
  else
  {
    x = min(uint(IrradianceRandom(randomState) * 64.0f), 63u);
    y = min(uint(IrradianceRandom(randomState) * 32.0f), 31u);
  }
  const float  z             = 1 - 2 * (float(y) + IrradianceRandom(randomState)) / 32.0f;
  const float  phi           = 6.28318530718f * (float(x) + IrradianceRandom(randomState)) / 64.0f;
  const float  radius        = sqrt(max(1 - z * z, 0));
  const float3 cubeDirection = float3(radius * cos(phi), z, radius * sin(phi));
  const float3 local         = float3(cubeDirection.x, -cubeDirection.z, cubeDirection.y);
  pdf                        = IrradianceEnvironmentCellProbability(x, y) * (2048.0f / 12.56637061436f);
  return mul(transpose((float3x3)SkyWorldToEnvironment), local);
}
