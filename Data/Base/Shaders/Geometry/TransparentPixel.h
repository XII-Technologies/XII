/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Pipeline/GpuSceneSurfaceShading.h>
#include <Shaders/Pipeline/Passes/Lighting/EnvironmentLightConstants.h>
#include <Shaders/Pipeline/Passes/Lighting/LightClusterConstants.h>
#include <Shaders/Pipeline/Passes/Transparency/TransparentRasterConstants.h>
#include <Shaders/Pipeline/SurfaceLocalLight.h>
#if XII_TRANSPARENT_SHADOWS
#  include <Shaders/Pipeline/Passes/Shadows/DirectionalShadowSampling.h>
#endif
#if XII_TRANSPARENT_ATMOSPHERE
#  include <Shaders/Pipeline/Passes/Atmosphere/AtmosphereSampling.h>
#endif

StructuredBuffer<xiiGpuLightData> g_Lights;
StructuredBuffer<uint4>           g_Counts;
StructuredBuffer<uint>            g_Candidates;
StructuredBuffer<uint2>           g_LightGrid;
StructuredBuffer<uint>            g_LightIndices;
StructuredBuffer<uint>            g_Overflow;

float3 xiiTransparentLocalLight(uint index, float3 world, float3 normal, float3 view, xiiSceneSurfaceSample surface, xiiGpuSurfaceMaterial material)
{
  uint count, stride;
  g_Lights.GetDimensions(count, stride);
  if (index >= min(count, g_Counts[0].x))
    return 0;
  const xiiGpuLightData light = g_Lights[index];
  const uint            type  = GetLightType(light);
  if (type == XII_LIGHT_TYPE_DIRECTIONAL || type > XII_LIGHT_TYPE_TUBE || light.ColorAndIntensity.w <= 0)
    return 0;
  return EvaluateLocalLightLayered(light, world, normal, view, surface.BaseColor.rgb, max(surface.Roughness, 0.01f), surface.Metallic, saturate(material.SurfaceParameters.y), surface.Transmission, saturate(material.LayerParameters.z), saturate(material.LayerParameters.w));
}

float3 xiiTransparentClusterLights(uint2 pixel, float3 world, float3 normal, float3 view, xiiSceneSurfaceSample surface, xiiGpuSurfaceMaterial material)
{
  float3 result = 0;
  if (g_Overflow[0] != 0)
  {
    uint count, stride;
    g_Candidates.GetDimensions(count, stride);
    [loop] for (uint i = 0; i < count; ++i) result += xiiTransparentLocalLight(g_Candidates[i], world, normal, view, surface, material);
  }
  else
  {
    const float depth = dot(world - CameraPosition.xyz, CameraForward.xyz);
    if (depth >= DepthRange.x && depth <= DepthRange.y)
    {
      const float t       = DepthRange.w > 0 ? log2(max(depth, DepthRange.x) / DepthRange.x) / DepthRange.z : (depth - DepthRange.x) / (DepthRange.y - DepthRange.x);
      const uint  slice   = min((uint)(saturate(t) * ClusterCounts.z), ClusterCounts.z - 1);
      const uint2 cell    = min(pixel / Viewport.z, ClusterCounts.xy - 1);
      const uint  cluster = (slice * ClusterCounts.y + cell.y) * ClusterCounts.x + cell.x;
      uint        count, stride;
      g_LightGrid.GetDimensions(count, stride);
      if (cluster < count)
      {
        const uint2 range = g_LightGrid[cluster];
        uint        capacity;
        g_LightIndices.GetDimensions(capacity, stride);
        [loop] for (uint i = 0; i < min(range.y, Viewport.w); ++i) if (range.x < capacity && i < capacity - range.x) result += xiiTransparentLocalLight(g_LightIndices[range.x + i], world, normal, view, surface, material);
      }
    }
  }
  return result;
}

float3 xiiTransparentAtmosphere(float3 existing, float3 world, bool orthographic)
{
#if XII_TRANSPARENT_ATMOSPHERE
  const float3 camera    = g_CameraPositionAndNearPlane.xyz;
  const float3 direction = orthographic ? normalize(g_CameraForwardAndFarPlane.xyz) : normalize(world - camera);
  const float3 rayOrigin = orthographic ? world - direction * dot(world - camera, direction) : camera;
  const float3 up        = normalize(PlanetUpAndGroundAltitudeMeters.xyz);
  const float  height    = PlanetAtmosphereRadiiScaleHeights.x + max((dot(rayOrigin, up) - PlanetUpAndGroundAltitudeMeters.w) * 0.001f, 0);
  const float3 origin    = up * height;
  const float  distance  = min(max(dot(world - rayOrigin, direction), 0) * 0.001f, DistanceToSphereExit(origin, direction, PlanetAtmosphereRadiiScaleHeights.y));
  const float3 sun       = normalize(-EnvironmentDirectionAndIlluminance.xyz);
  float3       transmittance, scattering;
  IntegrateAtmosphereSegment(origin, direction, distance, sun, dot(direction, sun), EnvironmentColorAndAngularRadius.rgb * max(EnvironmentDirectionAndIlluminance.w, 0), clamp(SampleCounts.w, 1u, 64u), transmittance, scattering);
  return existing * transmittance + scattering;
#else
  return existing;
#endif
}

struct TransparentOutput
{
  float4 Accumulation : SV_Target0;
  float4 Revealage : SV_Target1;
  float4 Additive : SV_Target2;
  float4 Modulation : SV_Target3;
  float4 HistoryTrust : SV_Target4;
};
TransparentOutput main(GBufferVertex input, bool isFrontFace : SV_IsFrontFace)
{
  const xiiGpuSurfaceMaterial material = xiiLoadSceneSurfaceMaterial(input.MaterialIndex, MaterialBaseIndex);
  // Geometry routing uses extracted material flags. Guard malformed/stale opaque packets too.
  if (material.Metadata.z < 2u && material.Metadata.w < 2u)
    clip(-1.0f);
  xiiSceneSurfaceSample surface = xiiSampleSceneSurface(material, input, isFrontFace);
  const float           opacity = saturate(surface.BaseColor.a);
  clip(opacity - 1.0e-5f);
  if ((material.Metadata.y & (1u << 6u)) == 0u)
    surface.Transmission = 0;
  const bool   orthographic = dot(g_ViewProjectionMatrix[3].xyz, g_ViewProjectionMatrix[3].xyz) < 1.0e-8f;
  const float3 view         = orthographic ? -normalize(g_CameraForwardAndFarPlane.xyz) : normalize(g_CameraPositionAndNearPlane.xyz - input.WorldPosition);
  const float  viewDepth    = dot(input.WorldPosition - g_CameraPositionAndNearPlane.xyz, g_CameraForwardAndFarPlane.xyz);
  float3       lighting     = surface.Emissive;
  if (material.Metadata.x == 6u || (material.Metadata.y & (1u << 12u)) == 0u)
    lighting += surface.BaseColor.rgb;
  else
  {
    lighting += max(EnvironmentAmbientRadiance.rgb, 0) * surface.BaseColor.rgb * (1 - surface.Metallic) * (1 - surface.Transmission) * surface.Occlusion / 3.14159265f;
    if (EnvironmentDirectionAndIlluminance.w > 0)
    {
      float visibility = 1;
#if XII_TRANSPARENT_SHADOWS
      if ((input.SurfaceFlags & (1u << 3u)) != 0u)
        visibility = xiiSampleDirectionalShadow(input.WorldPosition, surface.Normal, viewDepth, (uint)TransparentShadowBias.w, TransparentShadowBias.xyz);
#endif
      lighting += BRDFLayered(surface.BaseColor.rgb, max(surface.Roughness, 0.01f), surface.Metallic, saturate(material.SurfaceParameters.y), surface.Transmission, saturate(material.LayerParameters.z), saturate(material.LayerParameters.w), surface.Normal, normalize(-EnvironmentDirectionAndIlluminance.xyz), view) * EnvironmentColorAndAngularRadius.rgb * EnvironmentDirectionAndIlluminance.w * visibility;
    }
    lighting += xiiTransparentClusterLights(uint2(input.Position.xy), input.WorldPosition, surface.Normal, view, surface, material);
  }
  lighting = xiiTransparentAtmosphere(max(lighting * TransparentWeightsAndScale.z, 0), input.WorldPosition, orthographic);
  if (!all(isfinite(lighting)))
    clip(-1.0f);
  TransparentOutput output = (TransparentOutput)0;
  output.Modulation        = 1;
  output.HistoryTrust      = float4(0, 0, 0, opacity);
  if (material.Metadata.w == 3u)
    output.Additive = float4(lighting * opacity, 0);
  else if (material.Metadata.w == 4u)
    output.Modulation = float4(lerp(1, lighting, opacity), 1);
  else
  {
    const float nearRatio = max(viewDepth, 0) / TransparentWeightsAndScale.x;
    const float farRatio  = max(viewDepth, 0) / 200.0f;
    const float weight    = opacity * clamp(10.0f / (1.0e-5f + nearRatio * nearRatio + pow(farRatio, 6)), 0.01f, TransparentWeightsAndScale.y);
    output.Accumulation   = float4(lighting * opacity * weight, opacity * weight);
    output.Revealage      = float4(0, 0, 0, opacity);
  }
  return output;
}
