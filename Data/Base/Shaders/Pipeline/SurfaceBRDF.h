/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
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

float3 BRDFLayered(float3 albedo, float roughness, float metallic, float dielectricSpecular,
                   float transmission, float clearCoat, float clearCoatRoughness, float3 N, float3 L, float3 V)
{
  const float3 f0        = lerp((0.08f * saturate(dielectricSpecular)).xxx, albedo, metallic);
  const float3 H         = normalize(V + L);
  const float  NoH       = saturate(dot(N, H));
  const float  NoV       = max(dot(N, V), 1e-4f);
  const float  NoL       = saturate(dot(N, L));
  const float  VoH       = saturate(dot(V, H));
  const float3 F         = F_Schlick(f0, VoH);
  const float  D         = D_GGX(NoH, roughness);
  const float  G         = G_SmithGGX(NoV, NoL, roughness);
  const float3 spec      = (D * G * F) / max(4.0f * NoV * NoL, 1e-7f);
  const float3 diff      = (1.0f - F) * (1.0f - metallic) * (1.0f - saturate(transmission)) * albedo / 3.14159265f;
  const float3 baseLayer = (diff + spec) * NoL;

  // A dielectric coat sits above the base layer. Its Fresnel both contributes a
  // second GGX lobe and removes the same energy from the underlying material.
  const float coatWeight = saturate(clearCoat);
  if (coatWeight <= 0.0f)
    return baseLayer;

  const float  coatRoughness = clamp(clearCoatRoughness, 0.02f, 1.0f);
  const float3 coatF         = F_Schlick(0.04f.xxx, VoH);
  const float  coatD         = D_GGX(NoH, coatRoughness);
  const float  coatG         = G_SmithGGX(NoV, NoL, coatRoughness);
  const float3 coatSpecular  = (coatD * coatG * coatF) / max(4.0f * NoV * NoL, 1e-7f) * NoL;
  return baseLayer * (1.0f - coatF * coatWeight) + coatSpecular * coatWeight;
}

float3 BRDF(float3 albedo, float roughness, float metallic, float dielectricSpecular, float3 N, float3 L, float3 V)
{
  return BRDFLayered(albedo, roughness, metallic, dielectricSpecular, 0.0f, 0.0f, 0.1f, N, L, V);
}

