/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
float3 FresnelSchlickRoughness(float NoV, float3 f0, float roughness)
{
  const float3 grazing = max(1.0f - roughness, f0);
  return f0 + (grazing - f0) * pow(saturate(1.0f - NoV), 5.0f);
}

// The LUT already integrates Schlick Fresnel: use F0*A+B without applying a
// second view-dependent Fresnel factor to the integrated response.
float3 xiiEnvironmentSpecularResponse(float3 albedo, float metallic, float specular, float2 integratedBRDF)
{
  const float3 f0 = lerp((0.08f * saturate(specular)).xxx, max(albedo, 0), saturate(metallic));
  return f0 * integratedBRDF.x + integratedBRDF.y;
}
