/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Pipeline/LightingData.h>
#include <Shaders/Pipeline/PhotometricLightSampling.h>
#include <Shaders/Pipeline/SurfaceBRDF.h>
// FiniteAreaLight.h intentionally consumes the canonical BRDF and photometric
// profile functions above rather than carrying a renderer-specific copy.
#include <Shaders/Pipeline/FiniteAreaLight.h>

float3 EvaluateLocalLightLayered(xiiGpuLightData lightData, float3 worldPos, float3 N, float3 V,
                                 float3 albedo, float roughness, float metallic, float dielectricSpecular,
                                 float transmission, float clearCoat, float clearCoatRoughness)
{
  const uint lightType = GetLightType(lightData);

  if (lightType >= XII_LIGHT_TYPE_RECTANGLE)
    return EvaluateFiniteAreaLight(lightData, worldPos, N, V, albedo, roughness, metallic, dielectricSpecular, transmission, clearCoat, clearCoatRoughness);

  const float3 representativePosition = GetRepresentativeLightPosition(lightData, worldPos);
  const float3 toLight                = representativePosition - worldPos;
  const float  distanceToLight        = length(toLight);
  const float3 L                      = distanceToLight > 1e-4f ? toLight / distanceToLight : normalize(-lightData.DirectionAndType.xyz);

  float attenuation = EvaluateFiniteEmitterAttenuation(distanceToLight, lightData.AttenuationAndSize.x, lightData.AttenuationAndSize.y);
  if (lightType == XII_LIGHT_TYPE_SPOT)
    attenuation *= EvaluateSpotCone(lightData, L);

  attenuation *= SampleIESProfile(lightData, L);
  attenuation *= GetPhotometricEmitterScale(lightData, L);

  const float3 radiance = lightData.ColorAndIntensity.rgb * lightData.ColorAndIntensity.w * attenuation;
  return BRDFLayered(albedo, roughness, metallic, dielectricSpecular, transmission, clearCoat, clearCoatRoughness, N, L, V) * radiance;
}

float3 EvaluateLocalLight(xiiGpuLightData lightData, float3 worldPos, float3 N, float3 V,
                          float3 albedo, float roughness, float metallic, float dielectricSpecular)
{
  return EvaluateLocalLightLayered(lightData, worldPos, N, V, albedo, roughness, metallic, dielectricSpecular, 0.0f, 0.0f, 0.1f);
}
