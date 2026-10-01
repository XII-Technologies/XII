/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// Shared Type-C photometric profile sampling for surface and participating-
/// media lighting. A zero encoded profile selects a uniform emitter.
StructuredBuffer<float> g_IESProfiles;

static const uint XII_IES_VERTICAL_SAMPLES   = 64u;
static const uint XII_IES_HORIZONTAL_SAMPLES = 32u;
static const uint XII_IES_PROFILE_SAMPLES    = XII_IES_VERTICAL_SAMPLES * XII_IES_HORIZONTAL_SAMPLES;

/// Inverse-square attenuation with a smooth authored-range window. Inside a
/// finite emitter the distance is clamped by its physical radius instead of an
/// arbitrary world-space constant, preserving scale in calibrated scenes.
float EvaluateFiniteEmitterAttenuation(float distanceToLight, float range, float sourceRadius)
{
  const float normalizedDistance = saturate(distanceToLight / max(range, 1e-3f));
  float       rangeWindow        = saturate(1.0f - normalizedDistance * normalizedDistance);
  rangeWindow *= rangeWindow;
  const float minimumDistanceSquared = max(sourceRadius * sourceRadius, 1e-6f);
  return rangeWindow / max(distanceToLight * distanceToLight, minimumDistanceSquared);
}

float EvaluateSpotCone(xiiGpuLightData lightData, float3 pointToLightDirection)
{
  const float cosTheta = dot(-pointToLightDirection, normalize(lightData.DirectionAndType.xyz));
  const float cosInner = lightData.SpotAnglesAndRectSize.x;
  const float cosOuter = lightData.SpotAnglesAndRectSize.y;
  const float cone     = saturate((cosTheta - cosOuter) / max(cosInner - cosOuter, 1e-4f));
  return cone * cone * (3.0f - 2.0f * cone);
}

float SampleIESProfile(xiiGpuLightData lightData, float3 pointToLightDirection)
{
  const uint encodedProfile = (uint)(lightData.OrientationRightAndIES.w + 0.5f);
  if (encodedProfile == 0u)
    return 1.0f;

  // Type-C vertical zero follows the luminaire's local forward axis. The
  // stored right vector preserves roll for asymmetric industrial luminaires.
  const float3 emissionDirection = -pointToLightDirection;
  const float3 forward           = normalize(lightData.DirectionAndType.xyz);
  const float3 right             = normalize(lightData.OrientationRightAndIES.xyz);
  const float3 up                = normalize(cross(forward, right));

  const float vertical = acos(clamp(dot(emissionDirection, forward), -1.0f, 1.0f)) *
    ((float)(XII_IES_VERTICAL_SAMPLES - 1u) / 3.14159265f);
  float horizontalAngle = atan2(dot(emissionDirection, up), dot(emissionDirection, right));
  if (horizontalAngle < 0.0f)
    horizontalAngle += 6.28318531f;
  const float horizontal = horizontalAngle * ((float)XII_IES_HORIZONTAL_SAMPLES / 6.28318531f);

  const uint  vertical0        = min((uint)floor(vertical), XII_IES_VERTICAL_SAMPLES - 1u);
  const uint  vertical1        = min(vertical0 + 1u, XII_IES_VERTICAL_SAMPLES - 1u);
  const uint  horizontal0      = (uint)floor(horizontal) % XII_IES_HORIZONTAL_SAMPLES;
  const uint  horizontal1      = (horizontal0 + 1u) % XII_IES_HORIZONTAL_SAMPLES;
  const float verticalWeight   = frac(vertical);
  const float horizontalWeight = frac(horizontal);
  const uint  profileBase      = (encodedProfile - 1u) * XII_IES_PROFILE_SAMPLES;

  const float sample00 = g_IESProfiles[profileBase + horizontal0 * XII_IES_VERTICAL_SAMPLES + vertical0];
  const float sample01 = g_IESProfiles[profileBase + horizontal0 * XII_IES_VERTICAL_SAMPLES + vertical1];
  const float sample10 = g_IESProfiles[profileBase + horizontal1 * XII_IES_VERTICAL_SAMPLES + vertical0];
  const float sample11 = g_IESProfiles[profileBase + horizontal1 * XII_IES_VERTICAL_SAMPLES + vertical1];
  return lerp(lerp(sample00, sample01, verticalWeight), lerp(sample10, sample11, verticalWeight), horizontalWeight);
}
