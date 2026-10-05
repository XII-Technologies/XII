/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

// Shared view-independent atmosphere evaluation. Including shaders must expose
// g_Transmittance and g_MultiScatter together with the frame and atmosphere
// constant buffers. Keeping the ray direction explicit allows raster, compute,
// and ray-tracing stages to evaluate the same physical sky model.

#ifndef XII_ENVIRONMENT_LINEAR_CLAMP_SAMPLER
  #define XII_ENVIRONMENT_LINEAR_CLAMP_SAMPLER LinearClampSampler
#endif

float xiiAtmosphereRayleighPhase(float cosine)
{
  return (3.0f / (16.0f * 3.14159265359f)) * (1.0f + cosine * cosine);
}

float xiiAtmosphereMiePhase(float cosine, float anisotropy)
{
  const float g2 = anisotropy * anisotropy;
  const float denominator = max((2.0f + g2) * pow(max(1.0f + g2 - 2.0f * anisotropy * cosine, 1e-4f), 1.5f), 1e-4f);
  return (3.0f / (8.0f * 3.14159265359f)) * (1.0f - g2) * (1.0f + cosine * cosine) / denominator;
}

float xiiAtmosphereDistanceToExit(float radius, float cosine)
{
  const float atmosphereRadius = PlanetAtmosphereRadiiScaleHeights.y;
  const float b = radius * cosine;
  const float c = radius * radius - atmosphereRadius * atmosphereRadius;
  return max(-b + sqrt(max(b * b - c, 0.0f)), 0.0f);
}

float3 xiiSampleAtmosphereTransmittance(float radius, float cosine)
{
  const float2 uv = float2(
    saturate((radius - PlanetAtmosphereRadiiScaleHeights.x) /
             max(PlanetAtmosphereRadiiScaleHeights.y - PlanetAtmosphereRadiiScaleHeights.x, 1e-4f)),
    saturate(cosine * 0.5f + 0.5f));
  return g_Transmittance.SampleLevel(XII_ENVIRONMENT_LINEAR_CLAMP_SAMPLER, uv, 0.0f).rgb;
}

float3 xiiEvaluateAtmosphereRadiance(float3 worldPosition, float3 rayDirection)
{
  rayDirection = normalize(rayDirection);
  const float3 sunDirection = normalize(-g_MainLightDirectionAndIntensity.xyz);
  const float3 planetUp = normalize(PlanetUpAndGroundAltitudeMeters.xyz);
  const float altitudeMeters = dot(worldPosition, planetUp) - PlanetUpAndGroundAltitudeMeters.w;
  const float radius = PlanetAtmosphereRadiiScaleHeights.x + max(altitudeMeters * 0.001f, 0.0f);
  const float altitude = max(radius - PlanetAtmosphereRadiiScaleHeights.x, 0.0f);
  const float viewZenithCosine = dot(rayDirection, planetUp);
  const float sunZenithCosine = dot(sunDirection, planetUp);
  const float scatteringCosine = dot(rayDirection, sunDirection);

  const float rayleighDensity = exp(-altitude / PlanetAtmosphereRadiiScaleHeights.z);
  const float mieDensity = exp(-altitude / PlanetAtmosphereRadiiScaleHeights.w);
  const float3 scattering = RayleighScattering.xyz * rayleighDensity * xiiAtmosphereRayleighPhase(scatteringCosine) +
                            MieScatteringAndPhase.xyz * mieDensity * xiiAtmosphereMiePhase(scatteringCosine, MieScatteringAndPhase.w);
  const float3 viewTransmittance = xiiSampleAtmosphereTransmittance(radius, viewZenithCosine);
  const float3 sunTransmittance = xiiSampleAtmosphereTransmittance(radius, sunZenithCosine);
  const float pathLength = xiiAtmosphereDistanceToExit(radius, viewZenithCosine);
  const float3 solarIlluminance = max(g_MainLightColor.rgb * g_MainLightDirectionAndIntensity.w, 0.0f);
  float3 radiance = solarIlluminance * scattering * pathLength * viewTransmittance * sunTransmittance;

  const float2 multiScatterUV = float2(saturate(sunZenithCosine * 0.5f + 0.5f),
    saturate((radius - PlanetAtmosphereRadiiScaleHeights.x) /
             max(PlanetAtmosphereRadiiScaleHeights.y - PlanetAtmosphereRadiiScaleHeights.x, 1e-4f)));
  radiance += g_MultiScatter.SampleLevel(XII_ENVIRONMENT_LINEAR_CLAMP_SAMPLER, multiScatterUV, 0.0f).rgb * solarIlluminance * pathLength * viewTransmittance;

  const float sourceRadius = saturate(g_DirectionalShadowSourceRadius);
  if (sourceRadius > 1e-5f && scatteringCosine > sqrt(max(1.0f - sourceRadius * sourceRadius, 0.0f)))
  {
    radiance += solarIlluminance * viewTransmittance / max(3.14159265359f * sourceRadius * sourceRadius, 1e-6f);
  }

  return max(radiance, 0.0f);
}
