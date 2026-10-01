/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Pipeline/Passes/ShadowCascade/ShadowCascadeConstants.h>

Texture2DArray<float>                     g_VolumetricDirectionalShadowAtlas;
Texture2D<float>                          g_VolumetricLocalShadowAtlas;
StructuredBuffer<xiiLocalShadowAtlasData> g_VolumetricLocalShadowData;

uint SelectVolumetricCascade(float linearDepth)
{
  const uint cascadeCount = min(ActiveCascadeCount, 4u);
  [unroll] for (uint cascadeIndex = 0u; cascadeIndex < 4u; ++cascadeIndex)
  {
    if (cascadeIndex < cascadeCount && linearDepth <= CascadeSplitDepths[cascadeIndex])
      return cascadeIndex;
  }
  return cascadeCount;
}

/// Compact temporally-stable PCF for participating media. Volumes have no
/// receiver normal, so the bias follows the light ray instead of a surface.
float EvaluateVolumetricDirectionalShadow(float3 worldPosition, float3 directionToLight, float linearDepth)
{
  const uint cascadeCount = min(ActiveCascadeCount, 4u);
  const uint cascadeIndex = SelectVolumetricCascade(linearDepth);
  if (cascadeIndex >= cascadeCount)
    return 1.0f;

  const float  rayBias    = max(CascadeSplitDepths[cascadeIndex], 1.0f) * 0.00015f;
  const float4 shadowClip = mul(CascadeViewProjection[cascadeIndex], float4(worldPosition + directionToLight * rayBias, 1.0f));
  if (shadowClip.w <= 0.0f)
    return 1.0f;

  const float3 shadowNdc = shadowClip.xyz / shadowClip.w;
  const float2 shadowUV  = float2(shadowNdc.x * 0.5f + 0.5f, -shadowNdc.y * 0.5f + 0.5f);
  if (any(shadowUV <= 0.0f) || any(shadowUV >= 1.0f) || shadowNdc.z < 0.0f || shadowNdc.z > 1.0f)
    return 1.0f;

  uint atlasWidth;
  uint atlasHeight;
  uint atlasLayers;
  g_VolumetricDirectionalShadowAtlas.GetDimensions(atlasWidth, atlasHeight, atlasLayers);
  const float2 texelSize     = rcp(float2(atlasWidth, atlasHeight));
  const float  receiverDepth = shadowNdc.z + 0.00035f;
  float        visibility    = 0.0f;
  [unroll] for (uint sampleIndex = 0u; sampleIndex < 4u; ++sampleIndex)
  {
    const float2 offset   = float2(sampleIndex & 1u, sampleIndex >> 1u) * 2.0f - 1.0f;
    const float  mapDepth = g_VolumetricDirectionalShadowAtlas.SampleLevel(PointClampSampler, float3(shadowUV + offset * texelSize, cascadeIndex), 0.0f);
    visibility += receiverDepth >= mapDepth ? 1.0f : 0.0f;
  }
  return visibility * 0.25f;
}

uint SelectVolumetricLocalShadowFace(xiiLocalShadowAtlasData shadowData, float3 worldPosition)
{
  if (shadowData.Metadata.x <= 1u)
    return 0u;

  const float3 direction = worldPosition - shadowData.LightPositionAndInvRange.xyz;
  const float3 magnitude = abs(direction);
  if (magnitude.x >= magnitude.y && magnitude.x >= magnitude.z)
    return direction.x >= 0.0f ? 0u : 1u;
  if (magnitude.y >= magnitude.z)
    return direction.y >= 0.0f ? 2u : 3u;
  return direction.z >= 0.0f ? 4u : 5u;
}

float EvaluateVolumetricLocalShadow(uint lightIndex, xiiGpuLightData lightData, float3 worldPosition)
{
  if (lightData.ShadowData.x <= 0.5f)
    return 1.0f;

  const xiiLocalShadowAtlasData shadowData = g_VolumetricLocalShadowData[lightIndex];
  if (shadowData.Metadata.z == 0u || shadowData.Metadata.x == 0u)
    return 1.0f;

  const uint   faceIndex  = SelectVolumetricLocalShadowFace(shadowData, worldPosition);
  const float4 shadowClip = mul(shadowData.ViewProjection[faceIndex], float4(worldPosition, 1.0f));
  if (shadowClip.w <= 0.0f)
    return 1.0f;

  const float3 shadowNdc = shadowClip.xyz / shadowClip.w;
  const float2 localUV   = float2(shadowNdc.x * 0.5f + 0.5f, -shadowNdc.y * 0.5f + 0.5f);
  if (any(localUV <= 0.0f) || any(localUV >= 1.0f) || shadowNdc.z < 0.0f || shadowNdc.z > 1.0f)
    return 1.0f;

  uint atlasWidth;
  uint atlasHeight;
  g_VolumetricLocalShadowAtlas.GetDimensions(atlasWidth, atlasHeight);
  const float2 atlasTexel    = rcp(float2(atlasWidth, atlasHeight));
  const float4 scaleBias     = shadowData.AtlasScaleBias[faceIndex];
  const float2 atlasUV       = localUV * scaleBias.xy + scaleBias.zw;
  const float2 tileMinimum   = scaleBias.zw + atlasTexel * 1.5f;
  const float2 tileMaximum   = scaleBias.zw + scaleBias.xy - atlasTexel * 1.5f;
  const float  receiverDepth = shadowNdc.z + 0.0004f;

  float visibility = 0.0f;
  [unroll] for (uint sampleIndex = 0u; sampleIndex < 4u; ++sampleIndex)
  {
    const float2 offset   = (float2(sampleIndex & 1u, sampleIndex >> 1u) * 2.0f - 1.0f) * atlasTexel;
    const float  mapDepth = g_VolumetricLocalShadowAtlas.SampleLevel(PointClampSampler, clamp(atlasUV + offset, tileMinimum, tileMaximum), 0.0f);
    visibility += receiverDepth >= mapDepth ? 1.0f : 0.0f;
  }
  return lerp(1.0f, visibility * 0.25f, saturate(lightData.ShadowData.y));
}
