/// Copyright (c) Theophilus Eriata. All Rights Reserved.

// Shared physically based surface-light evaluation for deferred and Forward+ shading.
// The including shader must include PipelineCommon.xiiShader first so camera and
// clustered-light constants, LinearizeDepth(), and the common samplers are available.
#pragma once

#include <Shaders/Pipeline/LightingData.h>
#include <Shaders/Pipeline/PhotometricLightSampling.h>

float D_GGX(float NoH, float roughness)
{
  const float a  = roughness * roughness;
  const float a2 = a * a;
  const float d  = max(NoH * NoH * (a2 - 1.0f) + 1.0f, 1e-7f);
  return a2 / (3.14159265f * d * d);
}

float G_SmithGGX(float NoV, float NoL, float roughness)
{
  const float r  = roughness + 1.0f;
  const float k  = (r * r) * 0.125f;
  const float gv = NoV / (NoV * (1.0f - k) + k);
  const float gl = NoL / (NoL * (1.0f - k) + k);
  return gv * gl;
}

float3 F_Schlick(float3 f0, float VoH)
{
  return f0 + (1.0f - f0) * pow(saturate(1.0f - VoH), 5.0f);
}

float3 BRDF(float3 albedo, float roughness, float metallic, float dielectricSpecular, float3 N, float3 L, float3 V)
{
  const float3 f0   = lerp((0.08f * saturate(dielectricSpecular)).xxx, albedo, metallic);
  const float3 H    = normalize(V + L);
  const float NoH   = saturate(dot(N, H));
  const float NoV   = max(dot(N, V), 1e-4f);
  const float NoL   = saturate(dot(N, L));
  const float VoH   = saturate(dot(V, H));
  const float3 F    = F_Schlick(f0, VoH);
  const float D     = D_GGX(NoH, roughness);
  const float G     = G_SmithGGX(NoV, NoL, roughness);
  const float3 spec = (D * G * F) / max(4.0f * NoV * NoL, 1e-7f);
  const float3 diff = (1.0f - F) * (1.0f - metallic) * albedo / 3.14159265f;
  return (diff + spec) * NoL;
}

// FiniteAreaLight.h intentionally consumes the canonical BRDF and photometric
// profile functions above rather than carrying a renderer-specific copy.
#include <Shaders/Pipeline/FiniteAreaLight.h>

float3 EvaluateLocalLight(xiiGpuLightData lightData, float3 worldPos, float3 N, float3 V,
                          float3 albedo, float roughness, float metallic, float dielectricSpecular)
{
  const uint lightType = GetLightType(lightData);

  if (lightType >= XII_LIGHT_TYPE_RECTANGLE)
    return EvaluateFiniteAreaLight(lightData, worldPos, N, V, albedo, roughness, metallic, dielectricSpecular);

  const float3 representativePosition = GetRepresentativeLightPosition(lightData, worldPos);
  const float3 toLight                 = representativePosition - worldPos;
  const float distanceToLight          = length(toLight);
  const float3 L                       = distanceToLight > 1e-4f ? toLight / distanceToLight : normalize(-lightData.DirectionAndType.xyz);

  float attenuation = EvaluateFiniteEmitterAttenuation(distanceToLight, lightData.AttenuationAndSize.x, lightData.AttenuationAndSize.y);
  if (lightType == XII_LIGHT_TYPE_SPOT)
    attenuation *= EvaluateSpotCone(lightData, L);

  attenuation *= SampleIESProfile(lightData, L);
  attenuation *= GetPhotometricEmitterScale(lightData, L);

  const float3 radiance = lightData.ColorAndIntensity.rgb * lightData.ColorAndIntensity.w * attenuation;
  return BRDF(albedo, roughness, metallic, dielectricSpecular, N, L, V) * radiance;
}

uint ComputeClusterIndex(float2 uv, float depth)
{
  if (g_ClusterCountX == 0u || g_ClusterCountY == 0u || g_ClusterCountZ == 0u)
    return 0u;

  const float linearDepth = LinearizeDepth(depth);
  const float zSlice      = max(0.0f, log2(max(linearDepth, g_ClusterNearPlane) / max(g_ClusterNearPlane, 1e-4f)) /
                                         max(g_ClusterLogFarOverNear, 1e-4f) * (float)g_ClusterCountZ);

  const uint cX = min((uint)(uv.x * (float)g_ClusterCountX), g_ClusterCountX - 1u);
  const uint cY = min((uint)(uv.y * (float)g_ClusterCountY), g_ClusterCountY - 1u);
  const uint cZ = min((uint)zSlice, g_ClusterCountZ - 1u);
  return (cZ * g_ClusterCountY + cY) * g_ClusterCountX + cX;
}
