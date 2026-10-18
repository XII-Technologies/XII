/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
float3 FresnelSchlickRoughness(float NoV, float3 f0, float roughness)
{
  const float3 grazing=max(1.0f-roughness,f0);
  return f0+(grazing-f0)*pow(saturate(1.0f-NoV),5.0f);
}
