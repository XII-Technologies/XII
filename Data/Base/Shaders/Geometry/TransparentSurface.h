/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Pipeline/GpuSceneSurfaceShading.h>
#include <Shaders/Pipeline/Passes/Lighting/EnvironmentLightConstants.h>
#include <Shaders/Pipeline/Passes/Lighting/LightClusterConstants.h>
#include <Shaders/Pipeline/Passes/Transparency/TransparentRasterConstants.h>
#include <Shaders/Pipeline/SurfaceLocalLight.h>
#if FORWARD_SPOT_SHADOWS
#  include <Shaders/Pipeline/Passes/Shadows/SpotShadowSampling.h>
#endif
#if FORWARD_REFRACTION
#  include <Shaders/Pipeline/Passes/Refraction/ForwardRefraction.h>
#endif
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

float3 xiiTransparentLocalLight(uint index, float3 world, float3 normal, float3 view, xiiMaterialSurface surface, xiiGpuSurfaceMaterial material, bool receives)
{
  uint count, stride;
  g_Lights.GetDimensions(count, stride);
  if (index >= min(count, g_Counts[0].x))
    return 0;
  const xiiGpuLightData light = g_Lights[index];
  const uint            type  = GetLightType(light);
  if (type == XII_LIGHT_TYPE_DIRECTIONAL || type > XII_LIGHT_TYPE_TUBE || light.ColorAndIntensity.w <= 0)
    return 0;
  float3 result = EvaluateLocalLightLayered(light, world, normal, view, surface.BaseColor.rgb, max(surface.Roughness, 0.01f), surface.Metallic, saturate(surface.Reflectance), surface.Transmission, saturate(material.LayerParameters.z), saturate(material.LayerParameters.w));
#if FORWARD_SPOT_SHADOWS
  if (receives)
    result *= xiiSampleSpotShadow(index, world, normal, g_CameraPositionAndNearPlane.xyz);
#endif
  return result;
}

float3 xiiTransparentClusterLights(uint2 pixel, float3 world, float3 normal, float3 view, xiiMaterialSurface surface, xiiGpuSurfaceMaterial material, bool receives)
{
  float3 result = 0;
  if (g_Overflow[0] != 0)
  {
    uint count, stride;
    g_Candidates.GetDimensions(count, stride);
    [loop] for (uint i = 0; i < count; ++i) result += xiiTransparentLocalLight(g_Candidates[i], world, normal, view, surface, material, receives);
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
        [loop] for (uint i = 0; i < min(range.y, Viewport.w); ++i) if (range.x < capacity && i < capacity - range.x) result += xiiTransparentLocalLight(g_LightIndices[range.x + i], world, normal, view, surface, material, receives);
      }
    }
  }
  return result;
}

void xiiTransparentAtmosphereTerms(float3 world, bool orthographic, out float3 transmittance, out float3 scattering)
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
  IntegrateAtmosphereSegment(origin, direction, distance, sun, dot(direction, sun), EnvironmentColorAndAngularRadius.rgb * max(EnvironmentDirectionAndIlluminance.w, 0), clamp(SampleCounts.w, 1u, 64u), transmittance, scattering);

#else
  transmittance = 1;
  scattering    = 0;
#endif
}

float3 xiiTransparentAtmosphere(float3 existing, float3 world, bool orthographic)
{
  float3 transmittance, scattering;
  xiiTransparentAtmosphereTerms(world, orthographic, transmittance, scattering);
  return existing * transmittance + scattering;
}

#include <Shaders/Pipeline/Passes/Transparency/TransparentLayerOutput.h>
TransparentOutput xiiShadeTransparentSurface(GBufferVertex input, xiiMaterialSurface surface)
{
  const xiiGpuSurfaceMaterial material = xiiLoadSceneSurfaceMaterial(input.MaterialIndex, MaterialBaseIndex);
  // Geometry routing uses extracted material flags. Guard malformed/stale opaque packets too.
  if (material.Metadata.z < 2u && material.Metadata.w < 2u)
    clip(-1.0f);
  const float opacity = saturate(surface.BaseColor.a);
  clip(opacity - 1.0e-5f);
  if ((material.Metadata.y & (1u << 6u)) == 0u)
    surface.Transmission = 0;
  const bool   orthographic = dot(g_ViewProjectionMatrix[3].xyz, g_ViewProjectionMatrix[3].xyz) < 1.0e-8f;
  const float3 view         = orthographic ? -normalize(g_CameraForwardAndFarPlane.xyz) : normalize(g_CameraPositionAndNearPlane.xyz - input.WorldPosition);
  const float  viewDepth    = dot(input.WorldPosition - g_CameraPositionAndNearPlane.xyz, g_CameraForwardAndFarPlane.xyz);
  float3       lighting     = surface.Emissive;
  const bool   unlit        = surface.Unlit;
  if (unlit)
  {
#if FORWARD_REFRACTION
    lighting += surface.BaseColor.rgb * (1 - surface.Transmission);
#else
    lighting += surface.BaseColor.rgb;
#endif
  }
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
      lighting += BRDFLayered(surface.BaseColor.rgb, max(surface.Roughness, 0.01f), surface.Metallic, saturate(surface.Reflectance), surface.Transmission, saturate(material.LayerParameters.z), saturate(material.LayerParameters.w), surface.Normal, normalize(-EnvironmentDirectionAndIlluminance.xyz), view) * EnvironmentColorAndAngularRadius.rgb * EnvironmentDirectionAndIlluminance.w * visibility;
    }
    bool receives = true;
#if FORWARD_SPOT_SHADOWS
    receives = (input.SurfaceFlags & (1u << 3u)) != 0u;
#endif
    lighting += xiiTransparentClusterLights(uint2(input.Position.xy), input.WorldPosition, surface.Normal, view, surface, material, receives);
  }
#if FORWARD_REFRACTION
  float3 atmosphericTransmission, atmosphericScattering;
  xiiTransparentAtmosphereTerms(input.WorldPosition, orthographic, atmosphericTransmission, atmosphericScattering);
  lighting = max(lighting * TransparentWeightsAndScale.z, 0) * atmosphericTransmission + atmosphericScattering;
  if (surface.Transmission > 0)
  {
    float        fresnel;
    const float3 transmitted = xiiForwardRefraction(input.WorldPosition, surface.Normal, view, material.AdvancedParameters.y, material.AdvancedParameters.x, uint2(input.Position.xy), fresnel);
    // Background HDR already includes the camera's atmospheric segment. Do not attenuate it twice.
    lighting += max(transmitted - atmosphericScattering, 0) * surface.BaseColor.rgb * surface.Transmission * (1 - fresnel) * TransparentWeightsAndScale.z;
  }
#else
  lighting = xiiTransparentAtmosphere(max(lighting * TransparentWeightsAndScale.z, 0), input.WorldPosition, orthographic);
#endif
  if (!all(isfinite(lighting)))
    clip(-1.0f);
  return xiiTransparentLayer(lighting, opacity, viewDepth, material.Metadata.w, TransparentWeightsAndScale.xy);
}
