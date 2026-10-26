/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Pipeline/Passes/Atmosphere/AtmosphereConstants.h>
Texture2D<float4> g_Transmittance;
Texture2D<float4> g_MultiScatter;
SamplerState      LinearClampSampler;

// Sample the transmittance LUT for a given camera height and sun zenith.
float3 SampleTransmittance(float heightKm, float cosZenith)
{
  float2 uv = float2(
    saturate((heightKm - PlanetAtmosphereRadiiScaleHeights.x) / (PlanetAtmosphereRadiiScaleHeights.y - PlanetAtmosphereRadiiScaleHeights.x)),
    saturate(cosZenith * 0.5f + 0.5f));
  return g_Transmittance.SampleLevel(LinearClampSampler, uv, 0).rgb;
}

// Rayleigh phase.
float RayleighPhase(float cosTheta) { return (3.0f / (16.0f * 3.14159265f)) * (1.0f + cosTheta * cosTheta); }

float MiePhase(float cosTheta, float g)
{
  float g2 = g * g;
  return (3.0f / (8.0f * 3.14159265f)) * ((1.0f - g2) * (1.0f + cosTheta * cosTheta)) / max((2.0f + g2) * pow(max(1.0f + g2 - 2.0f * g * cosTheta, 1e-4f), 1.5f), 1e-4f);
}

float DistanceToSphereExit(float3 origin, float3 direction, float radius)
{
  float b = dot(origin, direction);
  float c = dot(origin, origin) - radius * radius;
  return max(-b + sqrt(max(b * b - c, 0.0f)), 0.0f);
}

float DistanceToGround(float3 origin, float3 direction, float groundRadius)
{
  if (length(origin) <= groundRadius + 1e-4f)
    return dot(origin, direction) < 0.0f ? 0.0f : -1.0f;

  const float b = dot(origin, direction);
  const float c = dot(origin, origin) - groundRadius * groundRadius;
  const float d = b * b - c;
  if (d < 0.0f)
    return -1.0f;

  const float nearDistance = -b - sqrt(d);
  return nearDistance >= 0.0f ? nearDistance : -1.0f;
}

void IntegrateAtmosphereSegment(float3 rayOrigin, float3 rayDirection, float rayLengthKm, float3 sunDirection,
                                float cosTheta, float3 solarIlluminance, uint stepCount,
                                out float3 transmittance, out float3 radiance)
{
  transmittance = 1.0f;
  radiance      = 0.0f;
  if (rayLengthKm <= 0.0f || stepCount == 0u)
    return;

  const float dt            = rayLengthKm / float(stepCount);
  const float rayleighPhase = RayleighPhase(cosTheta);
  const float miePhase      = MiePhase(cosTheta, MieScatteringAndPhase.w);

  [loop] for (uint step = 0u; step < stepCount; ++step)
  {
    const float3 samplePosition  = rayOrigin + rayDirection * ((float(step) + 0.5f) * dt);
    const float  sampleRadius    = max(length(samplePosition), PlanetAtmosphereRadiiScaleHeights.x);
    const float  altitudeKm      = max(sampleRadius - PlanetAtmosphereRadiiScaleHeights.x, 0.0f);
    const float3 sampleUp        = samplePosition / sampleRadius;
    const float  rayleighDensity = exp(-altitudeKm / PlanetAtmosphereRadiiScaleHeights.z);
    const float  mieDensity      = exp(-altitudeKm / PlanetAtmosphereRadiiScaleHeights.w);
    const float  ozoneDensity    = saturate(1.0f - abs(altitudeKm - 25.0f) / 15.0f);

    const float3 rayleigh           = RayleighScattering.xyz * rayleighDensity;
    const float3 mieScatter         = MieScatteringAndPhase.xyz * mieDensity;
    const float3 extinction         = rayleigh + mieScatter + MieAbsorption.xyz * mieDensity + OzoneAbsorption.xyz * ozoneDensity;
    const float3 stepTransmittance  = exp(-extinction * dt);
    const float3 integratedDistance = (1.0f - stepTransmittance) / max(extinction, 1e-6f);

    const float  cosSunZenith     = dot(sunDirection, sampleUp);
    const float3 sunTransmittance = SampleTransmittance(sampleRadius, cosSunZenith);
    const float2 msUV             = float2(saturate(cosSunZenith * 0.5f + 0.5f),
                                           saturate((sampleRadius - PlanetAtmosphereRadiiScaleHeights.x) /
                                                    (PlanetAtmosphereRadiiScaleHeights.y - PlanetAtmosphereRadiiScaleHeights.x)));
    const float3 multiScatter     = g_MultiScatter.SampleLevel(LinearClampSampler, msUV, 0).rgb;
    const float3 singleScatter    = (rayleigh * rayleighPhase + mieScatter * miePhase) * sunTransmittance;
    const float3 source           = solarIlluminance * (singleScatter + multiScatter);

    radiance += transmittance * source * integratedDistance;
    transmittance *= stepTransmittance;
  }
}
