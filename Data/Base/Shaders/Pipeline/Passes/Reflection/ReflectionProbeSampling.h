/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

// Shared local-reflection-probe evaluation. Including shaders must declare
// g_ReflectionProbeData and g_ReflectionProbeTextures. The exhaustive selector
// is intended for off-screen ray hits, which cannot use the camera cluster map.

#ifndef XII_ENVIRONMENT_LINEAR_CLAMP_SAMPLER
  #define XII_ENVIRONMENT_LINEAR_CLAMP_SAMPLER LinearClampSampler
#endif

float3 xiiReflectionProbeCubeDirection(float3 direction)
{
  return float3(direction.x, direction.z, -direction.y);
}

float xiiEvaluateReflectionProbeWeight(xiiGPUReflectionProbe probe, float3 worldPosition, out float3 localPosition)
{
  localPosition = mul(probe.WorldToProbe, float4(worldPosition, 1.0f)).xyz;
  const float blendDistance = probe.HalfExtentsAndBlend.w;

  if ((uint)(probe.ProbeParameters.w + 0.5f) == XII_REFLECTION_PROBE_SHAPE_SPHERE)
  {
    const float distanceToBoundary = probe.ProbeParameters.x - length(localPosition);
    if (distanceToBoundary < 0.0f)
      return 0.0f;
    return blendDistance > 1e-4f ? saturate(distanceToBoundary / blendDistance) : 1.0f;
  }

  const float3 distanceToPlanes = probe.HalfExtentsAndBlend.xyz - abs(localPosition);
  const float distanceToBoundary = min(distanceToPlanes.x, min(distanceToPlanes.y, distanceToPlanes.z));
  if (distanceToBoundary < 0.0f)
    return 0.0f;
  return blendDistance > 1e-4f ? saturate(distanceToBoundary / blendDistance) : 1.0f;
}

float3 xiiSampleReflectionProbe(uint probeIndexPlusOne, float3 worldPosition, float3 reflectionDirection, float roughness, out float weight)
{
  weight = 0.0f;
  if (probeIndexPlusOne == 0u)
    return 0.0f;

  const xiiGPUReflectionProbe probe = g_ReflectionProbeData[probeIndexPlusOne - 1u];
  float3 localPosition;
  weight = xiiEvaluateReflectionProbeWeight(probe, worldPosition, localPosition);
  if (weight <= 0.0f)
    return 0.0f;

  float3 localDirection = normalize(mul((float3x3)probe.WorldToProbe, reflectionDirection));
  if (probe.Metadata.w != 0u && (uint)(probe.ProbeParameters.w + 0.5f) == XII_REFLECTION_PROBE_SHAPE_BOX)
  {
    const float3 halfExtents = probe.HalfExtentsAndBlend.xyz;
    const float3 directionSign = step(0.0f, localDirection) * 2.0f - 1.0f;
    const float3 safeDirection = directionSign * max(abs(localDirection), 1e-5f);
    const float3 positivePlane = (halfExtents - localPosition) / safeDirection;
    const float3 negativePlane = (-halfExtents - localPosition) / safeDirection;
    const float3 forwardPlane = max(positivePlane, negativePlane);
    const float intersectionDistance = max(min(forwardPlane.x, min(forwardPlane.y, forwardPlane.z)), 0.0f);
    localDirection = normalize(localPosition + localDirection * intersectionDistance);
  }

  const uint textureIndex = NonUniformResourceIndex(probe.Metadata.y);
  float3 radiance = max(g_ReflectionProbeTextures[textureIndex].SampleLevel(XII_ENVIRONMENT_LINEAR_CLAMP_SAMPLER,
    xiiReflectionProbeCubeDirection(localDirection), saturate(roughness) * 8.0f).rgb, 0.0f);
  const float luminance = dot(radiance, float3(0.2126f, 0.7152f, 0.0722f));
  return lerp(luminance.xxx, radiance, probe.ProbeParameters.z) * probe.ProbeParameters.y;
}

bool xiiIsBetterReflectionProbe(int priority, float distanceSquared, uint stableId,
                                int currentPriority, float currentDistanceSquared, uint currentStableId)
{
  return priority > currentPriority ||
         (priority == currentPriority && distanceSquared < currentDistanceSquared) ||
         (priority == currentPriority && distanceSquared == currentDistanceSquared && stableId < currentStableId);
}

uint2 xiiFindReflectionProbesAtPosition(float3 worldPosition)
{
  uint firstProbe = 0u;
  uint secondProbe = 0u;
  int firstPriority = -2147483647;
  int secondPriority = -2147483647;
  float firstDistance = 3.402823e+38f;
  float secondDistance = 3.402823e+38f;
  uint firstStableId = 0xffffffffu;
  uint secondStableId = 0xffffffffu;

  [loop]
  for (uint probeIndex = 0u; probeIndex < ActiveProbeCount; ++probeIndex)
  {
    const xiiGPUReflectionProbe probe = g_ReflectionProbeData[probeIndex];
    float3 localPosition;
    if (xiiEvaluateReflectionProbeWeight(probe, worldPosition, localPosition) <= 0.0f)
      continue;

    const float3 toProbe = probe.PositionAndRadius.xyz - worldPosition;
    const float distanceSquared = dot(toProbe, toProbe);
    const int priority = asint(probe.Metadata.z);
    const uint stableId = probe.Metadata.x;
    if (xiiIsBetterReflectionProbe(priority, distanceSquared, stableId, firstPriority, firstDistance, firstStableId))
    {
      secondProbe = firstProbe;
      secondPriority = firstPriority;
      secondDistance = firstDistance;
      secondStableId = firstStableId;
      firstProbe = probeIndex + 1u;
      firstPriority = priority;
      firstDistance = distanceSquared;
      firstStableId = stableId;
    }
    else if (xiiIsBetterReflectionProbe(priority, distanceSquared, stableId, secondPriority, secondDistance, secondStableId))
    {
      secondProbe = probeIndex + 1u;
      secondPriority = priority;
      secondDistance = distanceSquared;
      secondStableId = stableId;
    }
  }

  return uint2(firstProbe, secondProbe);
}

float3 xiiSampleLocalReflectionEnvironment(float3 worldPosition, float3 reflectionDirection, float roughness, out float coverage)
{
  const uint2 selectedProbes = xiiFindReflectionProbesAtPosition(worldPosition);
  float firstWeight;
  float secondWeight;
  const float3 firstRadiance = xiiSampleReflectionProbe(selectedProbes.x, worldPosition, reflectionDirection, roughness, firstWeight);
  const float3 secondRadiance = xiiSampleReflectionProbe(selectedProbes.y, worldPosition, reflectionDirection, roughness, secondWeight);
  const float totalWeight = firstWeight + secondWeight;
  coverage = saturate(totalWeight);
  return totalWeight > 1e-4f ? (firstRadiance * firstWeight + secondRadiance * secondWeight) / totalWeight : 0.0f;
}
