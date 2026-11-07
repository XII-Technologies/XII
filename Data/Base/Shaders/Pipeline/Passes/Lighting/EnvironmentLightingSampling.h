/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Pipeline/SkyLightData.h>

TextureCube<float4> g_SkyRadiance;
TextureCube<float4> g_SkyIrradiance;

#if FORWARD_PROBES
#  include <Shaders/Pipeline/ReflectionProbeData.h>
StructuredBuffer<xiiGPUReflectionProbe> g_ReflectionProbeData;
TextureCube<float4>                     g_ReflectionProbeTextures[XII_MAX_REFLECTION_PROBES];
TextureCube<float4>                     g_ReflectionProbeIrradiance[XII_MAX_REFLECTION_PROBES];
#  define XII_REFLECTION_PROBE_IRRADIANCE
#  include <Shaders/Pipeline/Passes/Reflection/ReflectionProbeSampling.h>
#endif

float3 xiiSkyLightDirection(float3 worldDirection)
{
  const float3 local = mul((float3x3)SkyWorldToEnvironment, worldDirection);
  return float3(local.x, local.z, -local.y);
}

float3 xiiSkyLightRadiance(float3 value)
{
  value                 = max(value, 0);
  const float luminance = dot(value, float3(0.2126f, 0.7152f, 0.0722f));
  return lerp(luminance.xxx, value, SkyLightParameters.y) * SkyLightParameters.x;
}

float3 xiiSampleReflectionEnvironment(float3 position, float3 direction, float roughness, out float coverage)
{
  coverage     = 0;
  float3 local = 0;
#if FORWARD_PROBES
  local = xiiSampleLocalReflectionEnvironment(position, direction, roughness, coverage);
#endif
  [branch] if (SkyLightParameters.z > 0 && coverage < 1)
  {
    uint width, height, levels;
    g_SkyRadiance.GetDimensions(0, width, height, levels);
    const float3 sky = xiiSkyLightRadiance(g_SkyRadiance.SampleLevel(LinearClampSampler, xiiSkyLightDirection(direction), saturate(roughness) * float(max(levels, 1u) - 1u)).rgb);
    local            = local * coverage + sky * (1 - coverage);
    coverage         = 1;
  }
  return local;
}

float3 xiiSampleReflectionIrradiance(float3 position, float3 normal, out float coverage)
{
  coverage     = 0;
  float3 local = 0;
#if FORWARD_PROBES
  local = xiiSampleLocalReflectionIrradiance(position, normal, coverage);
#endif
  [branch] if (SkyLightParameters.z > 0 && coverage < 1)
  {
    const float3 sky = xiiSkyLightRadiance(g_SkyIrradiance.SampleLevel(LinearClampSampler, xiiSkyLightDirection(normal), 0).rgb);
    local            = local * coverage + sky * (1 - coverage);
    coverage         = 1;
  }
  return local;
}
