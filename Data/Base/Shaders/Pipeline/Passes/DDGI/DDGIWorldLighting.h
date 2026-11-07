/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#define XII_BRDF_VIEW_MODE ViewRenderMode_DiffuseOnly
#include <Shaders/Pipeline/SurfaceLocalLight.h>
// Emitter sampling consumes the canonical light records and finite-area helpers.
#include <Shaders/Pipeline/Passes/RayTracing/RayTracingEmitterSampling.h>

StructuredBuffer<xiiGpuLightData> g_Lights;
StructuredBuffer<uint4>           g_LightCounts;
StructuredBuffer<uint>            g_LightCandidates;

float ProbeLightRandom(inout uint seed)
{
  seed = HashProbe(seed + 0x9e3779b9u);
  return float(seed & 0x00ffffffu) / 16777216.0f;
}

float3 SampleProbeLight(xiiGpuLightData light, ProbeHit hit, float3 position, float3 viewDirection, float2 emitterSample, float reach)
{
  const uint type = GetLightType(light);
  float3     direction;
  float3     contribution;
  float      distance = reach;
  if (type == XII_LIGHT_TYPE_DIRECTIONAL)
  {
    direction    = normalize(-light.DirectionAndType.xyz);
    contribution = BRDFLayered(hit.Albedo, hit.Material.x, hit.Material.y, hit.Material.z, hit.Material.w, 0.0f, 0.1f, hit.Normal, direction, viewDirection) *
      max(light.ColorAndIntensity.rgb * light.ColorAndIntensity.w, 0.0f);
  }
  else if (type >= XII_LIGHT_TYPE_POINT && type <= XII_LIGHT_TYPE_TUBE)
  {
    float3 emitter;
    float  projectedArea;
    xiiSampleRayTracingLocalEmitter(light, position, emitterSample, emitter, projectedArea);
    const float3 toEmitter = emitter - position;
    distance               = length(toEmitter);
    if (distance <= 0.015f)
      return 0;
    direction = toEmitter / distance;
    if (type >= XII_LIGHT_TYPE_RECTANGLE)
      contribution = EvaluateAreaLightSample(light, emitter, projectedArea, position, hit.Normal, viewDirection, hit.Albedo,
                                             hit.Material.x, hit.Material.y, hit.Material.z, hit.Material.w, 0.0f, 0.1f);
    else
    {
      light.PositionAndInvRange.xyz = emitter;
      contribution                  = EvaluateLocalLightLayered(light, position, hit.Normal, viewDirection, hit.Albedo,
                                                                hit.Material.x, hit.Material.y, hit.Material.z, hit.Material.w, 0.0f, 0.1f);
    }
  }
  else
    return 0;
  if (dot(hit.Normal, direction) <= 0 || !all(isfinite(contribution)) || !any(contribution > 0))
    return 0;
  if (hit.ReceivesShadows && light.ShadowData.x > 0.5f)
  {
    // Rays outside the uploaded world domain cannot establish visibility.
    if (distance > reach)
      return 0;
    ProbeHit blocker;
    if (!TraceProbeScene(position + hit.Normal * 0.02f, direction, distance - 0.02f, blocker, true) || blocker.Found)
      return 0;
  }
  return contribution;
}

float3 ShadeProbeWorldHit(ProbeHit hit, float3 position, float3 viewDirection, uint seed, float reach)
{
  if (!any(hit.Normal != 0))
    return 0;
  uint recordCount, stride, candidateCount;
  g_Lights.GetDimensions(recordCount, stride);
  g_LightCandidates.GetDimensions(candidateCount, stride);
  const uint activeCount = min(g_LightCounts[0].x, recordCount);
  if (activeCount == 0u || candidateCount == 0u)
    return 0;
  const uint samples  = min(candidateCount, 2u);
  float3     radiance = 0;
  [loop] for (uint i = 0u; i < samples; ++i)
  {
    const float stratum = (float(i) + ProbeLightRandom(seed)) / float(samples);
    const uint  index   = g_LightCandidates[min(uint(stratum * float(candidateCount)), candidateCount - 1u)];
    if (index >= activeCount)
      continue;
    const float  first         = ProbeLightRandom(seed);
    const float2 emitterSample = float2(first, ProbeLightRandom(seed));
    radiance += SampleProbeLight(g_Lights[index], hit, position, viewDirection, emitterSample, reach);
  }
  return radiance * (float(candidateCount) / float(samples)) * hit.Occlusion;
}
