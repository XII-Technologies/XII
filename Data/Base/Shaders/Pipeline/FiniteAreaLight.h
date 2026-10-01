/// Copyright (c) Theophilus Eriata. All Rights Reserved.

// Deterministic finite-emitter quadrature for direct lighting.
//
// Each sample integrates emitted luminance over projected source area:
//   dE = Le * BRDF(N, L, V) * cos(thetaLight) * dA / distance^2
// BRDF() already contains the receiver cosine. Rectangle and disc emitters use
// surface-area quadrature, while sphere and capsule emitters integrate their
// projected silhouettes to fold cos(thetaLight) into dA.
#pragma once

float AreaLightRangeWindow(float distanceToLight, float range)
{
  const float normalizedDistance = saturate(distanceToLight / max(range, 1e-3f));
  const float window             = saturate(1.0f - normalizedDistance * normalizedDistance);
  return window * window;
}

float3 NormalizeAreaLightDirection(float3 direction, float3 fallback)
{
  const float lengthSquared = dot(direction, direction);
  return lengthSquared > 1e-8f ? direction * rsqrt(lengthSquared) : fallback;
}

void BuildAreaLightBasis(float3 normal, out float3 tangent, out float3 bitangent)
{
  const float3 helper = abs(normal.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
  tangent             = normalize(cross(helper, normal));
  bitangent           = cross(normal, tangent);
}

float2 GetAreaDiskSample(uint sampleIndex)
{
  const float radius = 0.70710678118f;
  if (sampleIndex == 0u)
    return float2(radius, 0.0f);
  if (sampleIndex == 1u)
    return float2(0.0f, radius);
  if (sampleIndex == 2u)
    return float2(-radius, 0.0f);
  return float2(0.0f, -radius);
}

float3 EvaluateAreaLightSample(xiiGpuLightData lightData, float3 samplePosition, float projectedSampleArea,
                               float3 worldPosition, float3 N, float3 V, float3 albedo, float roughness, float metallic)
{
  const float3 toLight                  = samplePosition - worldPosition;
  const float  unboundedDistanceSquared = dot(toLight, toLight);
  const float  distanceSquared          = max(unboundedDistanceSquared, 1e-6f);
  const float  distanceToLight          = sqrt(distanceSquared);
  const float3 L                        = unboundedDistanceSquared > 1e-8f ? toLight * rsqrt(unboundedDistanceSquared) : N;
  const float  rangeWindow              = AreaLightRangeWindow(distanceToLight, lightData.AttenuationAndSize.x);
  const float  iesScale                 = SampleIESProfile(lightData, L);
  const float3 emittedLuminance         = lightData.ColorAndIntensity.rgb * lightData.ColorAndIntensity.w;
  const float3 incidentRadiance         = emittedLuminance * (projectedSampleArea * rangeWindow * iesScale / distanceSquared);
  return BRDF(albedo, roughness, metallic, N, L, V) * incidentRadiance;
}

float3 EvaluateRectangleAreaLight(xiiGpuLightData lightData, float3 worldPosition, float3 N, float3 V, float3 albedo, float roughness, float metallic)
{
  const float3 center        = lightData.PositionAndInvRange.xyz;
  const float3 emitterNormal = normalize(lightData.DirectionAndType.xyz);
  const float3 right         = normalize(lightData.OrientationRightAndIES.xyz);
  const float3 up            = normalize(cross(right, emitterNormal));
  const float  width         = max(lightData.SpotAnglesAndRectSize.z, 1e-4f);
  const float  height        = max(lightData.SpotAnglesAndRectSize.w, 1e-4f);
  const float  sampleArea    = width * height * 0.25f;

  float3 result = 0.0f;
  [unroll] for (uint sampleIndex = 0u; sampleIndex < 4u; ++sampleIndex)
  {
    const float2 cell = float2((sampleIndex & 1u) != 0u ? 0.25f : -0.25f,
                               (sampleIndex & 2u) != 0u ? 0.25f : -0.25f);
    const float3 samplePosition = center + right * (cell.x * width) + up * (cell.y * height);
    const float3 sampleToPoint  = normalize(worldPosition - samplePosition);
    const float  projectedArea  = sampleArea * saturate(dot(emitterNormal, sampleToPoint));
    result += EvaluateAreaLightSample(lightData, samplePosition, projectedArea, worldPosition, N, V, albedo, roughness, metallic);
  }
  return result;
}

float3 EvaluateDiscAreaLight(xiiGpuLightData lightData, float3 worldPosition, float3 N, float3 V, float3 albedo, float roughness, float metallic)
{
  const float3 center        = lightData.PositionAndInvRange.xyz;
  const float3 emitterNormal = normalize(lightData.DirectionAndType.xyz);
  const float3 right         = normalize(lightData.OrientationRightAndIES.xyz);
  const float3 up            = normalize(cross(right, emitterNormal));
  const float  radius        = max(lightData.AttenuationAndSize.y, 1e-4f);
  const float  sampleArea    = 3.14159265f * radius * radius * 0.25f;

  float3 result = 0.0f;
  [unroll] for (uint sampleIndex = 0u; sampleIndex < 4u; ++sampleIndex)
  {
    const float2 diskSample     = GetAreaDiskSample(sampleIndex) * radius;
    const float3 samplePosition = center + right * diskSample.x + up * diskSample.y;
    const float3 sampleToPoint  = normalize(worldPosition - samplePosition);
    const float  projectedArea  = sampleArea * saturate(dot(emitterNormal, sampleToPoint));
    result += EvaluateAreaLightSample(lightData, samplePosition, projectedArea, worldPosition, N, V, albedo, roughness, metallic);
  }
  return result;
}

float3 EvaluateSphereAreaLight(xiiGpuLightData lightData, float3 worldPosition, float3 N, float3 V, float3 albedo, float roughness, float metallic)
{
  const float3 center        = lightData.PositionAndInvRange.xyz;
  const float  radius        = max(lightData.AttenuationAndSize.y, 1e-4f);
  const float3 centerToPoint = NormalizeAreaLightDirection(worldPosition - center, float3(0.0f, 0.0f, 1.0f));
  float3       tangent;
  float3       bitangent;
  BuildAreaLightBasis(centerToPoint, tangent, bitangent);
  const float projectedSampleArea = 3.14159265f * radius * radius * 0.25f;

  float3 result = 0.0f;
  [unroll] for (uint sampleIndex = 0u; sampleIndex < 4u; ++sampleIndex)
  {
    const float2 diskSample     = GetAreaDiskSample(sampleIndex) * radius;
    const float  surfaceHeight  = sqrt(max(radius * radius - dot(diskSample, diskSample), 0.0f));
    const float3 samplePosition = center + tangent * diskSample.x + bitangent * diskSample.y + centerToPoint * surfaceHeight;
    result += EvaluateAreaLightSample(lightData, samplePosition, projectedSampleArea, worldPosition, N, V, albedo, roughness, metallic);
  }
  return result;
}

float3 EvaluateTubeAreaLight(xiiGpuLightData lightData, float3 worldPosition, float3 N, float3 V, float3 albedo, float roughness, float metallic)
{
  const float3 center        = lightData.PositionAndInvRange.xyz;
  const float3 axis          = normalize(lightData.DirectionAndType.xyz);
  const float  radius        = max(lightData.AttenuationAndSize.y, 1e-4f);
  const float  length        = max(lightData.AttenuationAndSize.z, 1e-4f);
  const float3 centerToPoint = NormalizeAreaLightDirection(worldPosition - center, float3(0.0f, 0.0f, 1.0f));
  const float  axisCosine    = dot(centerToPoint, axis);
  const float  sideOn        = sqrt(saturate(1.0f - axisCosine * axisCosine));
  float3       radialToPoint = centerToPoint - axis * axisCosine;
  if (dot(radialToPoint, radialToPoint) < 1e-6f)
  {
    float3 unused;
    BuildAreaLightBasis(axis, radialToPoint, unused);
  }
  else
  {
    radialToPoint = normalize(radialToPoint);
  }

  float3      result               = 0.0f;
  const float projectedSegmentArea = 2.0f * radius * length * sideOn * 0.25f;
  [unroll] for (uint sampleIndex = 0u; sampleIndex < 4u; ++sampleIndex)
  {
    const float  axialOffset    = (float(sampleIndex) + 0.5f) * (length * 0.25f) - length * 0.5f;
    const float3 samplePosition = center + axis * axialOffset + radialToPoint * radius;
    result += EvaluateAreaLightSample(lightData, samplePosition, projectedSegmentArea, worldPosition, N, V, albedo, roughness, metallic);
  }

  // A capsule's two hemispherical end caps form one projected disk from any
  // direction. Place that disk on the end facing the receiver.
  const float  endSign           = axisCosine >= 0.0f ? 1.0f : -1.0f;
  const float3 capCenter         = center + axis * (endSign * length * 0.5f);
  const float3 capSamplePosition = capCenter + centerToPoint * radius;
  result += EvaluateAreaLightSample(lightData, capSamplePosition, 3.14159265f * radius * radius, worldPosition, N, V, albedo, roughness, metallic);
  return result;
}

float3 EvaluateFiniteAreaLight(xiiGpuLightData lightData, float3 worldPosition, float3 N, float3 V, float3 albedo, float roughness, float metallic)
{
  const uint lightType = GetLightType(lightData);
  if (lightType == XII_LIGHT_TYPE_RECTANGLE)
    return EvaluateRectangleAreaLight(lightData, worldPosition, N, V, albedo, roughness, metallic);
  if (lightType == XII_LIGHT_TYPE_DISC)
    return EvaluateDiscAreaLight(lightData, worldPosition, N, V, albedo, roughness, metallic);
  if (lightType == XII_LIGHT_TYPE_SPHERE || lightType == XII_LIGHT_TYPE_EMISSIVE_MESH)
    return EvaluateSphereAreaLight(lightData, worldPosition, N, V, albedo, roughness, metallic);
  return EvaluateTubeAreaLight(lightData, worldPosition, N, V, albedo, roughness, metallic);
}
