/// Copyright (c) Theophilus Eriata. All Rights Reserved.

// Shared shadow evaluation for deferred and Forward+ surface lighting.
// The including shader provides the cascade, virtual-shadow, cloud-shadow,
// directional atlas, local atlas, and local descriptor resources.
#pragma once

uint SelectDirectionalShadowCascade(float linearDepth)
{
  const uint cascadeCount = min(ActiveCascadeCount, 4u);
  [unroll] for (uint cascadeIndex = 0u; cascadeIndex < 4u; ++cascadeIndex)
  {
    if (cascadeIndex < cascadeCount && linearDepth <= CascadeSplitDepths[cascadeIndex])
      return cascadeIndex;
  }
  return cascadeCount;
}

float2 VogelDiskSample(uint sampleIndex, uint sampleCount, float rotation)
{
  const float goldenAngle = 2.39996323f;
  const float radius = sqrt((float(sampleIndex) + 0.5f) / max(float(sampleCount), 1.0f));
  const float angle = float(sampleIndex) * goldenAngle + rotation;
  return float2(cos(angle), sin(angle)) * radius;
}

float StableShadowRotation(float2 shadowUV, uint atlasWidth)
{
  const float2 texel = floor(shadowUV * float(atlasWidth));
  return frac(52.9829189f * frac(dot(texel, float2(0.06711056f, 0.00583715f)))) * 6.28318531f;
}

float SampleDirectionalShadowCascade(uint cascadeIndex, float3 worldPos, float3 N, float3 L)
{
  const float splitFar = max(CascadeSplitDepths[cascadeIndex], g_CameraPositionAndNearPlane.w);
  const float NoL = saturate(dot(N, L));
  const float normalBias = splitFar * lerp(0.00012f, 0.00045f, 1.0f - NoL);
  const float4 shadowClip = mul(CascadeViewProjection[cascadeIndex], float4(worldPos + N * normalBias, 1.0f));
  if (shadowClip.w <= 0.0f)
    return 1.0f;

  const float3 shadowNdc = shadowClip.xyz / shadowClip.w;
  const float2 shadowUV = float2(shadowNdc.x * 0.5f + 0.5f, -shadowNdc.y * 0.5f + 0.5f);
  if (any(shadowUV <= 0.0f) || any(shadowUV >= 1.0f) || shadowNdc.z < 0.0f || shadowNdc.z > 1.0f)
    return 1.0f;

  uint atlasWidth;
  uint atlasHeight;
  uint atlasLayers;
  g_ShadowAtlas.GetDimensions(atlasWidth, atlasHeight, atlasLayers);
  const float2 texelSize = rcp(float2(atlasWidth, atlasHeight));
  const float receiverDepth = shadowNdc.z + lerp(0.00008f, 0.00065f, 1.0f - NoL);
  float virtualVisibility;
  if (TrySampleVirtualDirectionalShadow(cascadeIndex, shadowUV, receiverDepth, virtualVisibility))
    return virtualVisibility;

  const float sourceRadius = max(g_DirectionalShadowSourceRadius, 0.0f);
  if (sourceRadius <= 1e-6f)
  {
    float visibility = 0.0f;
    [unroll] for (int y = -1; y <= 1; ++y)
    {
      [unroll] for (int x = -1; x <= 1; ++x)
      {
        const float mapDepth = g_ShadowAtlas.SampleLevel(PointClampSampler, float3(shadowUV + float2(x, y) * texelSize, cascadeIndex), 0.0f);
        visibility += receiverDepth >= mapDepth ? 1.0f : 0.0f;
      }
    }
    return visibility * (1.0f / 9.0f);
  }

  const float worldUnitsPerTexel = max((2.0f * CascadeWorldRadii[cascadeIndex]) / float(atlasWidth), 1e-5f);
  const float maxPenumbra = max(g_DirectionalShadowMaxPenumbra, 1.0f);
  const float blockerSearchRadius = clamp(sourceRadius * splitFar / worldUnitsPerTexel, 1.0f, maxPenumbra);
  const uint blockerSampleCount = clamp(g_DirectionalShadowBlockerSamples, 1u, 32u);
  const uint filterSampleCount = clamp(g_DirectionalShadowFilterSamples, 1u, 32u);
  const float rotation = StableShadowRotation(shadowUV, atlasWidth);

  float blockerDepthSum = 0.0f;
  uint blockerCount = 0u;
  [loop] for (uint sampleIndex = 0u; sampleIndex < blockerSampleCount; ++sampleIndex)
  {
    const float2 offset = VogelDiskSample(sampleIndex, blockerSampleCount, rotation) * blockerSearchRadius * texelSize;
    const float mapDepth = g_ShadowAtlas.SampleLevel(PointClampSampler, float3(shadowUV + offset, cascadeIndex), 0.0f);
    if (mapDepth > receiverDepth)
    {
      blockerDepthSum += mapDepth;
      ++blockerCount;
    }
  }
  if (blockerCount == 0u)
    return 1.0f;

  const float averageBlockerDepth = blockerDepthSum / float(blockerCount);
  const float normalizedSeparation = max(averageBlockerDepth - receiverDepth, 0.0f);
  const float penumbraRadius = clamp(sourceRadius * normalizedSeparation * splitFar / worldUnitsPerTexel, 1.0f, maxPenumbra);
  float visibility = 0.0f;
  [loop] for (uint sampleIndex = 0u; sampleIndex < filterSampleCount; ++sampleIndex)
  {
    const float2 offset = VogelDiskSample(sampleIndex, filterSampleCount, rotation) * penumbraRadius * texelSize;
    const float mapDepth = g_ShadowAtlas.SampleLevel(PointClampSampler, float3(shadowUV + offset, cascadeIndex), 0.0f);
    visibility += receiverDepth >= mapDepth ? 1.0f : 0.0f;
  }
  return visibility / float(filterSampleCount);
}

float EvaluateDirectionalShadow(float3 worldPos, float3 N, float3 L, float linearDepth)
{
  const uint cascadeCount = min(ActiveCascadeCount, 4u);
  if (cascadeCount == 0u)
    return 1.0f;

  const uint cascadeIndex = SelectDirectionalShadowCascade(linearDepth);
  if (cascadeIndex >= cascadeCount)
    return 1.0f;

  float visibility = SampleDirectionalShadowCascade(cascadeIndex, worldPos, N, L);
  if (cascadeIndex + 1u < cascadeCount)
  {
    const float splitNear = cascadeIndex == 0u ? g_CameraPositionAndNearPlane.w : CascadeSplitDepths[cascadeIndex - 1u];
    const float splitFar = CascadeSplitDepths[cascadeIndex];
    const float blendStart = lerp(splitNear, splitFar, 0.9f);
    const float blendWeight = saturate((linearDepth - blendStart) / max(splitFar - blendStart, 1e-4f));
    if (blendWeight > 0.0f)
      visibility = lerp(visibility, SampleDirectionalShadowCascade(cascadeIndex + 1u, worldPos, N, L), blendWeight);
  }
  return visibility;
}

uint SelectLocalShadowFace(xiiLocalShadowAtlasData shadowData, float3 worldPos)
{
  if (shadowData.Metadata.x <= 1u)
    return 0u;

  const float3 direction = worldPos - shadowData.LightPositionAndInvRange.xyz;
  const float3 magnitude = abs(direction);
  if (magnitude.x >= magnitude.y && magnitude.x >= magnitude.z)
    return direction.x >= 0.0f ? 0u : 1u;
  if (magnitude.y >= magnitude.z)
    return direction.y >= 0.0f ? 2u : 3u;
  return direction.z >= 0.0f ? 4u : 5u;
}

float EvaluateLocalShadow(uint lightIndex, xiiGpuLightData lightData, float3 worldPos, float3 N)
{
  if (lightData.ShadowData.x <= 0.5f)
    return 1.0f;

  const xiiLocalShadowAtlasData shadowData = g_LocalShadowAtlasDescs[lightIndex];
  if (shadowData.Metadata.z == 0u || shadowData.Metadata.x == 0u)
    return 1.0f;

  const uint faceIndex = SelectLocalShadowFace(shadowData, worldPos);
  const float3 toLight = shadowData.LightPositionAndInvRange.xyz - worldPos;
  const float distanceToLight = length(toLight);
  const float3 L = distanceToLight > 1e-4f ? toLight / distanceToLight : -normalize(lightData.DirectionAndType.xyz);
  const float NoL = saturate(dot(N, L));
  const float lightRange = rcp(max(shadowData.LightPositionAndInvRange.w, 1e-6f));
  const float worldUnitsPerTexel = lightRange / max(float(shadowData.Metadata.y), 1.0f);
  const float normalBias = worldUnitsPerTexel * lerp(0.35f, 1.5f, 1.0f - NoL);
  const float4 shadowClip = mul(shadowData.ViewProjection[faceIndex], float4(worldPos + N * normalBias, 1.0f));
  if (shadowClip.w <= 0.0f)
    return 1.0f;

  const float3 shadowNdc = shadowClip.xyz / shadowClip.w;
  const float2 localUV = float2(shadowNdc.x * 0.5f + 0.5f, -shadowNdc.y * 0.5f + 0.5f);
  if (any(localUV <= 0.0f) || any(localUV >= 1.0f) || shadowNdc.z < 0.0f || shadowNdc.z > 1.0f)
    return 1.0f;

  uint atlasWidth;
  uint atlasHeight;
  g_LocalShadowAtlas.GetDimensions(atlasWidth, atlasHeight);
  const float2 atlasTexel = rcp(float2(atlasWidth, atlasHeight));
  const float4 scaleBias = shadowData.AtlasScaleBias[faceIndex];
  const float2 atlasUV = localUV * scaleBias.xy + scaleBias.zw;
  const float2 tileMinimum = scaleBias.zw + atlasTexel * 1.5f;
  const float2 tileMaximum = scaleBias.zw + scaleBias.xy - atlasTexel * 1.5f;
  const float receiverDepth = shadowNdc.z + lerp(0.00008f, 0.0008f, 1.0f - NoL);
  const float rotation = StableShadowRotation(atlasUV, atlasWidth);

  const float sourceRadius = max(lightData.AttenuationAndSize.y, 0.0f);
  if (sourceRadius <= 1e-5f)
  {
    float visibility = 0.0f;
    [unroll] for (uint sampleIndex = 0u; sampleIndex < 9u; ++sampleIndex)
    {
      const float2 offset = VogelDiskSample(sampleIndex, 9u, rotation) * 1.5f * atlasTexel;
      const float mapDepth = g_LocalShadowAtlas.SampleLevel(PointClampSampler, clamp(atlasUV + offset, tileMinimum, tileMaximum), 0.0f);
      visibility += receiverDepth >= mapDepth ? 1.0f : 0.0f;
    }
    return lerp(1.0f, visibility / 9.0f, GetLocalShadowFadeStrength(lightData));
  }

  const float projectedSourceRadius = sourceRadius / max(distanceToLight, sourceRadius);
  const float blockerSearchRadius = clamp(projectedSourceRadius * float(shadowData.Metadata.y), 1.0f, 24.0f);
  const uint blockerSampleCount = clamp(g_DirectionalShadowBlockerSamples, 1u, 32u);
  float blockerDepthSum = 0.0f;
  uint blockerCount = 0u;
  [loop] for (uint sampleIndex = 0u; sampleIndex < blockerSampleCount; ++sampleIndex)
  {
    const float2 offset = VogelDiskSample(sampleIndex, blockerSampleCount, rotation) * blockerSearchRadius * atlasTexel;
    const float mapDepth = g_LocalShadowAtlas.SampleLevel(PointClampSampler, clamp(atlasUV + offset, tileMinimum, tileMaximum), 0.0f);
    if (mapDepth > receiverDepth)
    {
      blockerDepthSum += mapDepth;
      ++blockerCount;
    }
  }
  if (blockerCount == 0u)
    return 1.0f;

  const float averageBlockerDepth = blockerDepthSum / float(blockerCount);
  const float blockerSeparation = max(averageBlockerDepth - receiverDepth, 0.0f);
  const float penumbraRadius = clamp(blockerSearchRadius * blockerSeparation / max(averageBlockerDepth, 1e-4f), 1.0f, 32.0f);
  const uint filterSampleCount = clamp(g_DirectionalShadowFilterSamples, 1u, 32u);
  float visibility = 0.0f;
  [loop] for (uint sampleIndex = 0u; sampleIndex < filterSampleCount; ++sampleIndex)
  {
    const float2 offset = VogelDiskSample(sampleIndex, filterSampleCount, rotation) * penumbraRadius * atlasTexel;
    const float mapDepth = g_LocalShadowAtlas.SampleLevel(PointClampSampler, clamp(atlasUV + offset, tileMinimum, tileMaximum), 0.0f);
    visibility += receiverDepth >= mapDepth ? 1.0f : 0.0f;
  }
  return lerp(1.0f, visibility / float(filterSampleCount), GetLocalShadowFadeStrength(lightData));
}
