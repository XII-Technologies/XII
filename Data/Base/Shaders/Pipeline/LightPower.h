/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Pipeline/LightingData.h>

float GetLightPowerWeight(xiiGpuLightData lightData)
{
  const float luminance      = dot(max(lightData.ColorAndIntensity.rgb, 0.0f), float3(0.2126f, 0.7152f, 0.0722f));
  const float intensity      = max(lightData.ColorAndIntensity.w, 0.0f);
  const uint  lightType      = GetLightType(lightData);
  const float pi             = 3.14159265359f;
  float       emitterMeasure = 0.0f;

  if (lightType == XII_LIGHT_TYPE_POINT)
  {
    emitterMeasure = 4.0f * pi;
  }
  else if (lightType == XII_LIGHT_TYPE_SPOT)
  {
    emitterMeasure = 2.0f * pi * max(1.0f - lightData.SpotAnglesAndRectSize.y, 0.0f);
  }
  else if (lightType == XII_LIGHT_TYPE_RECTANGLE)
  {
    emitterMeasure = pi * max(lightData.SpotAnglesAndRectSize.z * lightData.SpotAnglesAndRectSize.w, 0.0f);
  }
  else if (lightType == XII_LIGHT_TYPE_DISC)
  {
    const float radius = max(lightData.AttenuationAndSize.y, 0.0f);
    emitterMeasure     = pi * pi * radius * radius;
  }
  else if (lightType == XII_LIGHT_TYPE_SPHERE)
  {
    const float radius = max(lightData.AttenuationAndSize.y, 0.0f);
    emitterMeasure     = 4.0f * pi * pi * radius * radius;
  }
  else if (lightType == XII_LIGHT_TYPE_TUBE)
  {
    const float radius = max(lightData.AttenuationAndSize.y, 0.0f);
    const float length = max(lightData.AttenuationAndSize.z, 0.0f);
    emitterMeasure     = pi * (2.0f * pi * radius * length + 4.0f * pi * radius * radius);
  }

  const float power = luminance * intensity * emitterMeasure;
  return isfinite(power) ? max(power, 0.0f) : 0.0f;
}
