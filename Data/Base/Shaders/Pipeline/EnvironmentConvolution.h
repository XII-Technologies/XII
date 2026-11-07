/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/StorageImageFormat.h>
#include <Shaders/Pipeline/ReflectionFilteredSpecularConstants.h>

TextureCube<float4> InputCubemap;
SamplerState        LinearClampSampler;
XII_STORAGE_IMAGE_FORMAT("rgba32f")
RWTexture2DArray<float4> ReflectionOutput;

float3 EnvironmentFaceDirection(uint2 pixel, uint face)
{
  const float2 uv = (float2(pixel) + 0.5f) * (2.0f / FaceExtent) - 1.0f;
  if (face == 0u)
    return normalize(float3(1, -uv.y, -uv.x));
  if (face == 1u)
    return normalize(float3(-1, -uv.y, uv.x));
  if (face == 2u)
    return normalize(float3(uv.x, 1, uv.y));
  if (face == 3u)
    return normalize(float3(uv.x, -1, -uv.y));
  if (face == 4u)
    return normalize(float3(uv.x, -uv.y, 1));
  return normalize(float3(-uv.x, -uv.y, -1));
}

float2 EnvironmentHammersley(uint index)
{
  return float2((float(index) + 0.5f) / float(SampleCount), float(reversebits(index)) * 2.3283064365386963e-10f);
}

float3 EnvironmentTangentToCube(float3 sampleDirection, float3 normal)
{
  const float3 up      = abs(normal.z) < 0.999f ? float3(0, 0, 1) : float3(1, 0, 0);
  const float3 tangent = normalize(cross(up, normal));
  return tangent * sampleDirection.x + cross(normal, tangent) * sampleDirection.y + normal * sampleDirection.z;
}

[numthreads(8, 8, 1)] void main(uint3 tid : SV_DispatchThreadID) {
  if (any(tid.xy >= FaceExtent) || tid.z >= 6u)
    return;
  const float3 normal   = EnvironmentFaceDirection(tid.xy, tid.z);
  float3       radiance = 0;
  float        weight   = 0;
  uint         sourceWidth, sourceHeight, sourceMips;
  InputCubemap.GetDimensions(0, sourceWidth, sourceHeight, sourceMips);
  const float texelSolidAngle = 12.5663706144f / (6.0f * sourceWidth * sourceWidth);
#if !XII_ENVIRONMENT_DIFFUSE
  if (MipLevel == 0u)
  {
    radiance              = max(InputCubemap.SampleLevel(LinearClampSampler, normal, 0).rgb, 0);
    ReflectionOutput[tid] = float4(radiance, 1);
    return;
  }
  const float roughness = float(MipLevel) / float(max(MipCount - 1u, 1u));
  const float alpha     = roughness * roughness;
  const float alpha2    = alpha * alpha;
#endif
  [loop] for (uint sampleIndex = 0u; sampleIndex < SampleCount; ++sampleIndex)
  {
    const float2 xi  = EnvironmentHammersley(sampleIndex);
    const float  phi = 6.2831853072f * xi.y;
#if XII_ENVIRONMENT_DIFFUSE
    // Cosine sampling estimates E/pi directly, with unit weight per sample.
    const float  radius       = sqrt(xi.x);
    const float3 direction    = EnvironmentTangentToCube(float3(radius * cos(phi), radius * sin(phi), sqrt(1.0f - xi.x)), normal);
    const float  pdf          = max(dot(normal, direction), 0) / 3.1415926536f;
    const float  sampleWeight = 1.0f;
#else
    const float  cosTheta     = sqrt(saturate((1.0f - xi.x) / ((1.0f - xi.x) + alpha2 * xi.x)));
    const float  sinTheta     = sqrt(max(1.0f - cosTheta * cosTheta, 0));
    const float3 halfVector   = EnvironmentTangentToCube(float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta), normal);
    const float3 direction    = reflect(-normal, halfVector);
    const float  sampleWeight = max(dot(normal, direction), 0);
    const float  denominator  = cosTheta * cosTheta * (alpha2 - 1.0f) + 1.0f;
    const float  pdf          = alpha2 / max(12.5663706144f * denominator * denominator, 1e-12f);
#endif
    if (sampleWeight > 0)
    {
      const float  sampleSolidAngle = 1.0f / max(float(SampleCount) * pdf, 1e-8f);
      const float  lod              = clamp(0.5f * log2(sampleSolidAngle / texelSolidAngle), 0, float(max(sourceMips, 1u) - 1u));
      const float3 sampleRadiance   = max(InputCubemap.SampleLevel(LinearClampSampler, direction, lod).rgb, 0);
      // A weighted running mean preserves constant HDR fields and avoids
      // overflowing an intermediate radiance sum for bright environments.
      const float nextWeight = weight + sampleWeight;
      radiance += (sampleRadiance - radiance) * (sampleWeight / nextWeight);
      weight = nextWeight;
    }
  }
  ReflectionOutput[tid] = float4(all(isfinite(radiance)) ? radiance : 0, 1);
}
