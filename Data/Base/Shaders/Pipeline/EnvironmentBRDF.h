/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
// The LUT already integrates Schlick Fresnel: use F0*A+B without applying a
// second view-dependent Fresnel factor to the integrated response.
float3 xiiEnvironmentSpecularResponse(float3 albedo, float metallic, float specular, float2 integratedBRDF)
{
  const float3 f0 = lerp((0.08f * saturate(specular)).xxx, max(albedo, 0), saturate(metallic));
  return f0 * integratedBRDF.x + integratedBRDF.y;
}
