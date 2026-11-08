/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/StorageImageFormat.h>
// Motion-aware temporal accumulation shared by stochastic AO and near-field GI.
// History is clipped to a current-frame edge-aware neighbourhood before blending,
// which bounds ghost energy while retaining a high effective sample count.

#include <Shaders/Pipeline/Passes/Denoising/TemporalDenoiseConstants.h>
#include <Shaders/Pipeline/PipelineCommon.xiiShader>

#if defined(XII_DENOISE_LOCAL_LIGHT)
#  include <Shaders/Pipeline/Passes/Lighting/LocalLightReservoirConstants.h>
Texture2D<uint> g_SampleDomain : register(t16);
#endif

Texture2D<float4> g_CurrentSignal : register(t4);
Texture2D<float4> g_PreviousSignal : register(t5);
Texture2D<float>  g_SceneDepth : register(t6);
Texture2D<float2> g_GBufferNormal : register(t7);
Texture2D<float2> g_Velocity : register(t8);
Texture2D<float4> g_PreviousSurface : register(t9);
XII_STORAGE_IMAGE_FORMAT(XII_DENOISE_FORMAT)
RWTexture2D<float4> g_DenoisedOutput : register(u10);
#if defined(XII_DENOISE_CAPTURE_SURFACE)
#  if !defined(XII_DENOISE_SURFACE_FORMAT)
#    define XII_DENOISE_SURFACE_FORMAT "rgba16f"
#  endif
XII_STORAGE_IMAGE_FORMAT(XII_DENOISE_SURFACE_FORMAT)
RWTexture2D<float4> g_CurrentSurface : register(u11);
#endif

#if defined(XII_DENOISE_MATERIAL_VALIDATION)
Texture2D<uint>   g_ObjectId : register(t12);
Texture2D<float4> g_GBufferMaterial : register(t13);
uint2             SignalIdentity(uint2 pixel)
{
  const uint4 material = (uint4)round(saturate(g_GBufferMaterial.Load(int3(pixel, 0))) * 255.0f);
  return uint2(g_ObjectId.Load(int3(pixel, 0)), material.x | (material.y << 8u) | (material.z << 16u) | (material.w << 24u));
}
#endif
#if defined(XII_DENOISE_CAPTURE_IDENTITY)
#  if defined(XII_DENOISE_LOCAL_LIGHT)
Texture2D<uint4> g_PreviousIdentity : register(t14);
XII_STORAGE_IMAGE_FORMAT("rgba32ui")
RWTexture2D<uint4> g_CurrentIdentity : register(u15);
#  else
Texture2D<uint2> g_PreviousIdentity : register(t14);
XII_STORAGE_IMAGE_FORMAT("rg32ui")
RWTexture2D<uint2> g_CurrentIdentity : register(u15);
#  endif
#endif

float4 LoadSignal(int2 pixel, uint2 dimensions)
{
  const int2   clampedPixel = clamp(pixel, int2(0, 0), int2(dimensions) - 1);
  const float4 signal       = g_CurrentSignal.Load(int3(clampedPixel, 0));
  return SignalMode == 1U ? signal : signal.xxxx;
}

float SurfaceViewDepth(uint2 pixel, uint2 dimensions, float depth)
{
  const float2 uv = (float2(pixel) + 0.5f) / float2(dimensions);
  return dot(ReconstructWorldPos(uv, depth) - g_CameraPositionAndNearPlane.xyz, g_CameraForwardAndFarPlane.xyz);
}

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID) {
  uint2 dimensions;
  g_DenoisedOutput.GetDimensions(dimensions.x, dimensions.y);
  if (any(dispatchThreadId.xy >= dimensions))
    return;

  const uint2 pixel   = dispatchThreadId.xy;
  const float depth   = g_SceneDepth.Load(int3(pixel, 0));
  float4      current = LoadSignal(int2(pixel), dimensions);
  if (SignalMode == 1u)
    current = all(isfinite(current)) ? max(current, 0.0f) : 0.0f;
#if defined(XII_DENOISE_LOCAL_LIGHT)
  const uint domain = g_SampleDomain.Load(int3(pixel, 0));
#endif
#if defined(XII_DENOISE_MATERIAL_VALIDATION)
  const uint2 identity = SignalIdentity(pixel);
#endif
#if defined(XII_DENOISE_CAPTURE_IDENTITY)
#  if defined(XII_DENOISE_LOCAL_LIGHT)
  g_CurrentIdentity[pixel] = uint4(identity, domain, 0u);
#  else
  g_CurrentIdentity[pixel] = identity;
#  endif
#endif
#if defined(XII_DENOISE_CAPTURE_SURFACE)
  // Keep the reprojection surface in this pass instance's own history. Capturing
  // it here avoids a dependency on a lighting algorithm's unrelated history.
  g_CurrentSurface[pixel] = depth > 0.0f ? float4(g_GBufferNormal.Load(int3(pixel, 0)), SurfaceViewDepth(pixel, dimensions, depth), 1.0f) : 0.0f.xxxx;
#endif
  if (depth <= 0.0f)
  {
    g_DenoisedOutput[pixel] = SignalMode == 0U ? 1.0f.xxxx : current;
    return;
  }
#if defined(XII_DENOISE_LOCAL_LIGHT)
  if (domain == 0u)
  {
    g_DenoisedOutput[pixel] = current;
    return;
  }
#endif

  const float2 encodedNormal = g_GBufferNormal.Load(int3(pixel, 0));
  const float3 normal        = DecodeOctNormal(encodedNormal);
  const float  linearDepth   = SurfaceViewDepth(pixel, dimensions, depth);

  float4 localMinimum   = current;
  float4 localMaximum   = current;
  float4 spatialSum     = current;
  float  spatialSamples = 1.0f;

#if defined(XII_DENOISE_LOCAL_LIGHT)
  const int radius = 2;
#else
  const int radius = 1;
#endif
  [unroll] for (int y = -radius; y <= radius; ++y)
  {
    [unroll] for (int x = -radius; x <= radius; ++x)
    {
      if (x == 0 && y == 0)
        continue;

      const int2  samplePixel = clamp(int2(pixel) + int2(x, y), int2(0, 0), int2(dimensions) - 1);
      const float sampleDepth = g_SceneDepth.Load(int3(samplePixel, 0));
      if (sampleDepth <= 0.0f)
        continue;

      const float  sampleLinearDepth = SurfaceViewDepth(samplePixel, dimensions, sampleDepth);
      const float3 sampleNormal      = DecodeOctNormal(g_GBufferNormal.Load(int3(samplePixel, 0)));
      if (abs(sampleLinearDepth - linearDepth) > max(linearDepth * DepthThreshold, 0.01f) || dot(sampleNormal, normal) < NormalThreshold)
        continue;

#if defined(XII_DENOISE_MATERIAL_VALIDATION)
      if (any(SignalIdentity(uint2(samplePixel)) != identity))
        continue;
#endif
#if defined(XII_DENOISE_LOCAL_LIGHT)
      if (g_SampleDomain.Load(int3(samplePixel, 0)) != domain)
        continue;
#endif
      const float4 sampleSignal = LoadSignal(samplePixel, dimensions);
      if (!all(isfinite(sampleSignal)))
        continue;
      localMinimum = min(localMinimum, sampleSignal);
      localMaximum = max(localMaximum, sampleSignal);
      spatialSum += sampleSignal;
      spatialSamples += 1.0f;
    }
  }

  current = lerp(current, spatialSum / spatialSamples, saturate(SpatialWeight));
#if defined(XII_DENOISE_SPATIAL_ONLY)
  g_DenoisedOutput[pixel] = current;
#else
#  if defined(XII_DENOISE_LOCAL_LIGHT)
  // Moving shadow casters can change visibility while receiver and light IDs stay fixed.
  if ((domain & XII_LOCAL_LIGHT_SHADOW_DOMAIN) != 0u)
  {
    g_DenoisedOutput[pixel] = current;
    return;
  }
#  endif
  if (HistoryValid == 0U)
  {
    g_DenoisedOutput[pixel] = current;
    return;
  }

  const float2 uv       = (float2(pixel) + 0.5f) / float2(dimensions);
  const float2 velocity = g_Velocity.Load(int3(pixel, 0));
#  if defined(XII_DENOISE_REPROJECT_DEPTH)
  // Raster motion excludes projection jitter; raw AO and surface histories
  // retain the previous raster grid rather than TAA's stable color grid.
  const float2 previousUV = uv + (JitterDelta - velocity) * float2(0.5f, -0.5f);
#  else
  const float2 previousUV = uv - velocity * float2(0.5f, -0.5f);
#  endif
  if (!all(isfinite(previousUV)) || any(previousUV <= 0.0f) || any(previousUV >= 1.0f))
  {
    g_DenoisedOutput[pixel] = current;
    return;
  }

  const uint2 previousPixel = min((uint2)(previousUV * float2(dimensions)), dimensions - 1U);
#  if defined(XII_DENOISE_CAPTURE_IDENTITY)
#    if defined(XII_DENOISE_LOCAL_LIGHT)
  if (any(g_PreviousIdentity.Load(int3(previousPixel, 0)).xyz != uint3(identity, domain)))
#    else
  if (any(g_PreviousIdentity.Load(int3(previousPixel, 0)) != identity))
#    endif
  {
    g_DenoisedOutput[pixel] = current;
    return;
  }
#  endif
  const float4 previousSurface = g_PreviousSurface.Load(int3(previousPixel, 0));
  const float3 previousNormal  = DecodeOctNormal(previousSurface.xy);
#  if defined(XII_DENOISE_REPROJECT_DEPTH)
  const float expectedPreviousDepth = dot(ReconstructWorldPos(uv, depth) - PreviousCameraPosition.xyz, PreviousCameraForward.xyz);
#  else
  const float expectedPreviousDepth = linearDepth;
#  endif
  const bool surfaceMatches = previousSurface.w > 0.0f &&
    expectedPreviousDepth > 0 && abs(previousSurface.z - expectedPreviousDepth) <= max(expectedPreviousDepth * DepthThreshold, 0.01f) &&
    dot(previousNormal, normal) >= NormalThreshold;
  if (!surfaceMatches)
  {
    g_DenoisedOutput[pixel] = current;
    return;
  }

  float4 history = g_PreviousSignal.Load(int3(previousPixel, 0));
  if (!all(isfinite(history)))
    history = current;
  history                 = SignalMode == 1U ? history : history.xxxx;
  history                 = clamp(history, localMinimum, localMaximum);
  g_DenoisedOutput[pixel] = lerp(current, history, saturate(HistoryWeight));
#endif
}
