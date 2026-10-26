/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Pipeline/Passes/Refraction/ForwardRefractionConstants.h>

Texture2D<float>  g_RefractionDepth;
Texture2D<float4> g_RefractionBackground;
SamplerState      g_sRefractionLinearClamp;

bool xiiProjectRefraction(float3 world, out float2 uv)
{
  const float4 clipPosition = mul(g_ViewProjectionMatrix, float4(world, 1));
  uv                        = 0;
  if (!all(isfinite(clipPosition)) || clipPosition.w <= 1e-7f)
    return false;
  const float3 ndc = clipPosition.xyz / clipPosition.w;
  uv               = float2(ndc.x * 0.5f + 0.5f, 0.5f - ndc.y * 0.5f);
  return all(uv >= 0) && all(uv < 1) && ndc.z > 0 && ndc.z <= 1;
}

bool xiiRefractionDepthGap(float3 world, float surfaceDepth, out float2 uv, out float gap)
{
  gap = 0;
  if (!xiiProjectRefraction(world, uv))
    return false;
  uint width, height;
  g_RefractionDepth.GetDimensions(width, height);
  const uint2 pixel = min(uint2(uv * float2(width, height)), uint2(width - 1, height - 1));
  const float raw   = g_RefractionDepth.Load(int3(pixel, 0));
  if (!isfinite(raw) || raw <= 0 || raw > 1)
    return false;
  const float2 center        = (float2(pixel) + 0.5f) / float2(width, height);
  const float4 reconstructed = mul(g_InverseViewProjectionMatrix, float4(center.x * 2 - 1, 1 - center.y * 2, raw, 1));
  if (!all(isfinite(reconstructed)) || abs(reconstructed.w) < 1e-7f)
    return false;
  const float sceneDepth = dot(reconstructed.xyz / reconstructed.w - g_CameraPositionAndNearPlane.xyz, g_CameraForwardAndFarPlane.xyz);
  if (sceneDepth <= surfaceDepth + 1e-4f)
    return false; // Never refract a foreground occluder onto the transmitting surface.
  gap = dot(world - g_CameraPositionAndNearPlane.xyz, g_CameraForwardAndFarPlane.xyz) - sceneDepth;
  return isfinite(gap);
}

/// Parallel-plane dielectric slab. The emerging ray is parallel to the incident ray, with a Snell-law lateral shift.
/// Screen misses explicitly use the unshifted opaque background; this path has no off-screen radiance source.
float3 xiiForwardRefraction(float3 world, float3 normal, float3 view, float ior, float thickness, uint2 pixel, out float transmissionFresnel)
{
  uint width, height;
  g_RefractionBackground.GetDimensions(width, height);
  const float3 fallback = max(g_RefractionBackground.Load(int3(min(pixel, uint2(width - 1, height - 1)), 0)).rgb, 0);
  transmissionFresnel   = 0;
  if (!isfinite(ior) || ior < 1 || !isfinite(thickness) || thickness < 0)
    return fallback;

  const float3 incident = -view;
  normal                = dot(normal, incident) > 0 ? -normal : normal;
  const float cosine    = saturate(-dot(normal, incident));
  const float f0        = (ior - 1) / (ior + 1);
  transmissionFresnel   = f0 * f0 + (1 - f0 * f0) * pow(1 - cosine, 5);
  if (ior == 1)
  {
    transmissionFresnel = 0;
    return fallback;
  }
  if (thickness == 0)
    return fallback;
  const float3 inside       = refract(incident, normal, 1 / ior);
  const float  normalTravel = -dot(normal, inside);
  if (!all(isfinite(inside)) || normalTravel <= 1e-5f)
    return fallback;
  const float3 exitPoint    = world + inside * (thickness / normalTravel);
  const float  surfaceDepth = dot(world - g_CameraPositionAndNearPlane.xyz, g_CameraForwardAndFarPlane.xyz);
  float2       uv;
  float        initialGap;
  if (!xiiRefractionDepthGap(exitPoint, surfaceDepth, uv, initialGap) || initialGap >= 0)
    return fallback;

  const uint steps            = (uint)RefractionTrace.z;
  float      previousDistance = 0;
  [loop] for (uint step = 1; step <= steps; ++step)
  {
    const float fraction = float(step) / float(steps);
    const float distance = fraction * fraction * RefractionTrace.x;
    float       gap;
    if (!xiiRefractionDepthGap(exitPoint + incident * distance, surfaceDepth, uv, gap))
      return fallback;
    if (gap >= 0)
    {
      float nearDistance = previousDistance;
      float farDistance  = distance;
      [unroll] for (uint refine = 0; refine < 8; ++refine)
      {
        const float middle = (nearDistance + farDistance) * 0.5f;
        if (!xiiRefractionDepthGap(exitPoint + incident * middle, surfaceDepth, uv, gap))
          return fallback;
        if (gap >= 0)
          farDistance = middle;
        else
          nearDistance = middle;
      }
      if (!xiiRefractionDepthGap(exitPoint + incident * farDistance, surfaceDepth, uv, gap) || gap > RefractionTrace.y)
        return fallback;
      const float3 color = g_RefractionBackground.SampleLevel(g_sRefractionLinearClamp, uv, 0).rgb;
      return all(isfinite(color)) ? max(color, 0) : fallback;
    }
    previousDistance = distance;
  }
  return fallback;
}
