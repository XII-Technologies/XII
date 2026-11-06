/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/StorageImageFormat.h>
#include <Shaders/Pipeline/Passes/Reflections/SSRTemporalConstants.h>
#include <Shaders/Pipeline/Passes/ScreenSpaceGeometry.h>

Texture2D<float4> g_CurrentReflection;
Texture2D<float4> g_PreviousReflection;
Texture2D<float>  g_SceneDepth;
Texture2D<float2> g_GBufferNormal;
Texture2D<float4> g_GBufferMaterial;
Texture2D<uint>   g_GBufferObjectId;
Texture2D<float2> g_Velocity;
Texture2D<uint2>  g_PreviousSurface;
Texture2D<uint2>  g_PreviousIdentitySignature;
XII_STORAGE_IMAGE_FORMAT(XII_SSR_TEMPORAL_FORMAT)
RWTexture2D<float4> g_ResolvedReflection;
XII_STORAGE_IMAGE_FORMAT("rg32ui")
RWTexture2D<uint2> g_CurrentSurface;
XII_STORAGE_IMAGE_FORMAT("rg32ui")
RWTexture2D<uint2> g_CurrentIdentitySignature;

uint PackSSRMaterialSignature(float4 material)
{
  const uint4 value = (uint4)round(saturate(material) * 255.0f);
  return value.x | (value.y << 8u) | (value.z << 16u) | (value.w << 24u);
}

[numthreads(8, 8, 1)] void main(uint3 tid : SV_DispatchThreadID) {
  uint2 dimensions;
  g_ResolvedReflection.GetDimensions(dimensions.x, dimensions.y);
  if (any(tid.xy >= dimensions))
    return;
  const uint2  pixel                = tid.xy;
  const float  depth                = g_SceneDepth.Load(int3(pixel, 0));
  const float2 encodedNormal        = g_GBufferNormal.Load(int3(pixel, 0));
  const uint2  identity             = uint2(PackSSRMaterialSignature(g_GBufferMaterial.Load(int3(pixel, 0))), g_GBufferObjectId.Load(int3(pixel, 0)));
  const uint2  packedNormal         = (uint2)round(saturate(encodedNormal) * 65535.0f);
  g_CurrentSurface[pixel]           = uint2(packedNormal.x | (packedNormal.y << 16u), asuint(depth));
  g_CurrentIdentitySignature[pixel] = identity;
  float4 current                    = g_CurrentReflection.Load(int3(pixel, 0));
  current                           = all(isfinite(current)) ? float4(clamp(current.rgb, 0, 65504.0f), saturate(current.a)) : 0;
  if (current.a <= 1e-4f)
    current = 0;
  // A current miss must use the scene's fallback, never leave a ghost of an
  // emitter or receiver that is no longer represented by the current trace.
  [branch] if (HistoryValid == 0u || !isfinite(depth) || !all(isfinite(encodedNormal)) || depth <= 0 || identity.y == 0u || current.a <= 1e-4f)
  {
    g_ResolvedReflection[pixel] = current;
    return;
  }
  const float2 uv         = (float2(pixel) + 0.5f) / float2(dimensions);
  const float2 motion     = g_Velocity.Load(int3(pixel, 0));
  const float2 previousUV = uv + (JitterDelta - motion) * float2(0.5f, -0.5f);
  if (!all(isfinite(previousUV)) || any(previousUV < 0) || any(previousUV >= 1))
  {
    g_ResolvedReflection[pixel] = current;
    return;
  }
  const float4 world        = mul(InverseViewProjection, float4(uv * float2(2, -2) + float2(-1, 1), depth, 1));
  const float4 previousClip = mul(PreviousViewProjection, world);
  if (!all(isfinite(previousClip)) || previousClip.w <= 0)
  {
    g_ResolvedReflection[pixel] = current;
    return;
  }
  const float  expectedDepth   = previousClip.z / previousClip.w;
  const uint2  previousPixel   = min((uint2)(previousUV * float2(dimensions)), dimensions - 1u);
  const uint2  packedSurface   = g_PreviousSurface.Load(int3(previousPixel, 0));
  const float3 previousSurface = float3(float2(packedSurface.x & 65535u, packedSurface.x >> 16u) / 65535.0f, asfloat(packedSurface.y));
  const float3 normal          = xiiScreenNormal(encodedNormal);
  const float  tolerance       = max(max(abs(expectedDepth), abs(previousSurface.z)) * DepthThreshold, 1e-5f);
  if (!all(isfinite(previousSurface)) || expectedDepth <= 0 || expectedDepth > 1 || previousSurface.z <= 0 || abs(previousSurface.z - expectedDepth) > tolerance ||
      dot(xiiScreenNormal(previousSurface.xy), normal) < NormalThreshold || any(g_PreviousIdentitySignature.Load(int3(previousPixel, 0)) != identity))
  {
    g_ResolvedReflection[pixel] = current;
    return;
  }
  float4 history = g_PreviousReflection.Load(int3(previousPixel, 0));
  if (!all(isfinite(history)) || history.a <= 1e-4f)
  {
    g_ResolvedReflection[pixel] = current;
    return;
  }
  float3 minimum = current.rgb, maximum = current.rgb, sum = 0, squaredSum = 0;
  uint   count = 0;
  [unroll] for (int y = -1; y <= 1; ++y)
    [unroll] for (int x = -1; x <= 1; ++x)
  {
    const int2   samplePixel = clamp(int2(pixel) + int2(x, y), 0, int2(dimensions) - 1);
    const float4 sampleValue = g_CurrentReflection.Load(int3(samplePixel, 0));
    if (sampleValue.a <= 1e-4f || !all(isfinite(sampleValue)) || g_GBufferObjectId.Load(int3(samplePixel, 0)) != identity.y ||
        PackSSRMaterialSignature(g_GBufferMaterial.Load(int3(samplePixel, 0))) != identity.x ||
        dot(xiiScreenNormal(g_GBufferNormal.Load(int3(samplePixel, 0))), normal) < NormalThreshold)
      continue;
    const float3 value = clamp(sampleValue.rgb, 0, 65504.0f);
    minimum            = min(minimum, value);
    maximum            = max(maximum, value);
    sum += value;
    squaredSum += value * value;
    ++count;
  }
  const float3 mean      = sum / max(float(count), 1);
  const float3 deviation = sqrt(max(squaredSum / max(float(count), 1) - mean * mean, 0));
  history.rgb            = clamp(history.rgb, max(minimum, mean - 1.5f * deviation), min(maximum, mean + 1.5f * deviation));
  const float  oldWeight = saturate(history.a) * HistoryWeight * ConfidenceDecay;
  const float3 radiance  = (current.rgb * current.a + history.rgb * oldWeight) / max(current.a + oldWeight, 1e-5f);
  // Coverage is the current trace's confidence. Reuse filters radiance without
  // repeatedly increasing its contribution to the scene's specular energy.
  g_ResolvedReflection[pixel] = float4(radiance, current.a);
}
