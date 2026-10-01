/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Pipeline/Passes/Atmosphere/CloudShadowConstants.h>

float CloudShadowHash(float2 p)
{
  return frac(sin(dot(p, float2(127.1f, 311.7f))) * 43758.5453123f);
}

float CloudShadowNoise(float2 p)
{
  const float2 cell           = floor(p);
  const float2 fraction       = frac(p);
  const float2 smoothFraction = fraction * fraction * (3.0f - 2.0f * fraction);
  const float  a              = CloudShadowHash(cell);
  const float  b              = CloudShadowHash(cell + float2(1.0f, 0.0f));
  const float  c              = CloudShadowHash(cell + float2(0.0f, 1.0f));
  const float  d              = CloudShadowHash(cell + float2(1.0f, 1.0f));
  return lerp(lerp(a, b, smoothFraction.x), lerp(c, d, smoothFraction.x), smoothFraction.y);
}

float CloudShadowFBM(float2 p)
{
  float value     = 0.0f;
  float amplitude = 0.5f;
  [unroll] for (uint octave = 0u; octave < 4u; ++octave)
  {
    value += CloudShadowNoise(p) * amplitude;
    p = mul(float2x2(0.8f, -0.6f, 0.6f, 0.8f), p * 2.03f) + 17.0f;
    amplitude *= 0.5f;
  }
  return value;
}

/// Beer-Lambert transmission through the procedural cloud layer along a ray
/// from the shaded point toward the sun.
float EvaluateCloudShadow(float3 worldPosition, float3 directionToSun)
{
  if (WindStrengthAndEnabled.w < 0.5f || WindStrengthAndEnabled.z <= 0.0f || LayerNormalAndOpticalDepth.w <= 0.0f)
    return 1.0f;

  const float3 layerNormal         = normalize(LayerNormalAndOpticalDepth.xyz);
  const float  rayPlaneDenominator = dot(directionToSun, layerNormal);
  if (abs(rayPlaneDenominator) < 1e-4f)
    return 1.0f;

  const float distanceToLayer = dot(LayerOriginAndInvScale.xyz - worldPosition, layerNormal) / rayPlaneDenominator;
  if (distanceToLayer <= 0.0f)
    return 1.0f;

  const float3 layerPosition    = worldPosition + directionToSun * distanceToLayer;
  const float2 worldCoordinates = float2(dot(layerPosition, ProjectionAxisUAndDetail.xyz), dot(layerPosition, ProjectionAxisVAndCoverage.xyz));
  const float2 windOffset       = WindStrengthAndEnabled.xy * g_WorldTime;
  const float2 baseUV           = (worldCoordinates - windOffset) * LayerOriginAndInvScale.w;

  const float baseShape         = CloudShadowFBM(baseUV);
  const float detailShape       = CloudShadowFBM(baseUV * ProjectionAxisUAndDetail.w + 41.0f);
  const float cloudShape        = saturate(baseShape * 0.8f + detailShape * 0.2f);
  const float coverage          = saturate(ProjectionAxisVAndCoverage.w);
  const float density           = saturate((cloudShape - (1.0f - coverage)) / max(coverage, 0.02f));
  const float slantOpticalDepth = LayerNormalAndOpticalDepth.w * density / max(abs(rayPlaneDenominator), 0.1f);
  const float transmittance     = exp(-slantOpticalDepth);
  return lerp(1.0f, transmittance, saturate(WindStrengthAndEnabled.z));
}
