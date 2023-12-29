#pragma once

#if SHADING_QUALITY != SHADING_QUALITY_SIMPLIFIED
#error "Functions in LightDataSimplified.h are only for SIMPLIFIED shading quality. Include LightData.h instead."
#endif

#include <Shaders/Common/GlobalConstants.h>
#include <Shaders/Common/LightDataSimplified.h>
#include <Shaders/Common/AmbientCubeBasis.h>
#include <Shaders/Common/BRDF.h>

#if XII_RENDERER_ENABLED
TextureCubeArray ReflectionSpecularTexture;
Texture2D SkyIrradianceTexture;
#endif

///////////////////////////////////////////////////////////////////////////////////

float GetFogAmount(float3 worldPosition)
{
  return 1.0f;
}

float DepthFade(float3 screenPosition, float fadeDistance)
{
  return 1.0f;
}

AccumulatedLight CalculateLightingSimplified(xiiMaterialData matData)
{
  float3 viewVector = normalize(GetCameraPosition() - matData.worldPosition);

  AccumulatedLight totalLight = InitializeLight(0.0f, 0.0f);

  float occlusion = matData.occlusion;

  // sky light in ambient cube basis
  #if XII_RENDERER_ENABLED
  float3 skyLight = EvaluateAmbientCube(SkyIrradianceTexture, SkyIrradianceIndex, matData.worldNormal).rgb;
  totalLight.diffuseLight += matData.diffuseColor * skyLight * occlusion;
  #else
  totalLight.diffuseLight += matData.diffuseColor * occlusion;
  #endif

  return totalLight;
}
