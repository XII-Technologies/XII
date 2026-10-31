/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#define XII_SCENE_WORLD_ATTRIBUTES
#define XII_SCENE_MATERIAL_ATTRIBUTES
#define XII_SCENE_SURFACE_FLAGS
#include <Shaders/Common/MaterialGraphUtilities.h>
#include <Shaders/Materials/NativeMaterialViewConstants.h>
#include <Shaders/Pipeline/GBufferMaterial.h>
#include <Shaders/Pipeline/GpuSceneSurfaceShading.h>
#include <Shaders/Pipeline/GpuSceneSurfaceVertex.h>
StructuredBuffer<GpuSceneInstance> g_SceneInstances;

/// Authored surface stages consume the canonical GPU scene vertex and write the
/// same seven targets as standard materials. Geometry and visibility stay native.
float3 xiiMaterialDecodeNormalTexture(float4 value)
{
  float3 normal = value.xyz * 2 - 1;
  normal.z      = sqrt(saturate(1 - dot(normal.xy, normal.xy)));
  return normalize(normal);
}

float3 xiiMaterialTangentToWorld(GBufferVertex input, float3 normal)
{
  const float3 n = normalize(input.WorldNormal);
  const float3 t = normalize(input.WorldTangent);
  return normalize(t * normal.x + cross(n, t) * input.TangentSign * normal.y + n * normal.z);
}

xiiMaterialSurface xiiDefaultMaterialSurface(GBufferVertex input, bool frontFace)
{
  const xiiGpuSurfaceMaterial material = xiiLoadSceneSurfaceMaterial(input.MaterialIndex, MaterialBaseIndex);
  if (!frontFace && (material.Metadata.y & (1u << 10u)) == 0u)
    clip(-1.0f);
  return xiiSampleSceneSurface(material, input, frontFace, false);
}

#if defined(RENDER_PASS) && (RENDER_PASS == RENDER_PASS_PICKING || RENDER_PASS == RENDER_PASS_PICKING_WIREFRAME)
struct xiiMaterialSurfaceOutput
{
  uint4 Surface : SV_Target0;
};
xiiMaterialSurfaceOutput xiiWriteMaterialSurface(GBufferVertex input, xiiMaterialSurface surface)
{
#  if defined(BLEND_MODE) && BLEND_MODE == BLEND_MODE_MASKED
  clip(surface.BaseColor.a - surface.AlphaCutoff);
#  elif defined(BLEND_MODE) && BLEND_MODE >= BLEND_MODE_TRANSPARENT
  clip(surface.BaseColor.a - 1e-5f);
#  endif
  const float2             normal = EncodeOctNormal(normalize(surface.Normal));
  xiiMaterialSurfaceOutput result;
  result.Surface = uint4(input.ObjectId, asuint(input.Position.z), asuint(normal.x), asuint(normal.y));
  return result;
}
#elif defined(BLEND_MODE) && BLEND_MODE >= BLEND_MODE_TRANSPARENT && (!defined(RENDER_PASS) || RENDER_PASS != RENDER_PASS_DEPTH_ONLY)
#  define XII_TRANSPARENT_SHADOWS    FORWARD_DIRECTIONAL_SHADOWS
#  define XII_TRANSPARENT_ATMOSPHERE FORWARD_ATMOSPHERE
#  include <Shaders/Geometry/TransparentSurface.h>
typedef TransparentOutput xiiMaterialSurfaceOutput;
xiiMaterialSurfaceOutput  xiiWriteMaterialSurface(GBufferVertex input, xiiMaterialSurface surface)
{
  return xiiShadeTransparentSurface(input, surface);
}
#else
struct xiiMaterialSurfaceOutput
{
#  if defined(RENDER_PASS) && RENDER_PASS == RENDER_PASS_DEPTH_ONLY
  float Depth : SV_Depth;
#  else
  float4 Albedo : SV_Target0;
  float2 Normal : SV_Target1;
  float4 Material : SV_Target2;
  float4 Emissive : SV_Target3;
  float4 NormalRoughness : SV_Target4;
  float2 Velocity : SV_Target5;
  uint   ObjectId : SV_Target6;
#  endif
};

xiiMaterialSurfaceOutput xiiWriteMaterialSurface(GBufferVertex input, xiiMaterialSurface surface)
{
#  if defined(BLEND_MODE) && BLEND_MODE == BLEND_MODE_MASKED
  clip(surface.BaseColor.a - surface.AlphaCutoff);
#  endif
  xiiMaterialSurfaceOutput result;
#  if defined(RENDER_PASS) && RENDER_PASS == RENDER_PASS_DEPTH_ONLY
  const xiiGpuSurfaceMaterial material = xiiLoadSceneSurfaceMaterial(input.MaterialIndex, MaterialBaseIndex);
  if ((material.Metadata.y & (1u << 13u)) == 0u || material.Metadata.z >= 2u)
    clip(-1.0f);
  result.Depth = input.Position.z;
#  else
  const xiiGpuSurfaceMaterial material = xiiLoadSceneSurfaceMaterial(input.MaterialIndex, MaterialBaseIndex);
  const bool                  unlit    = surface.Unlit;
  const float2                normal   = EncodeOctNormal(normalize(surface.Normal));
  result.Albedo                        = float4(surface.BaseColor.rgb, saturate(surface.Occlusion));
  result.Normal                        = normal;
  result.Material                      = float4(max(saturate(surface.Roughness), 0.01f), saturate(surface.Metallic), saturate(surface.Reflectance), xiiPackGBufferMaterialFlags(material.Metadata.y, unlit));
  result.Emissive                      = float4(surface.Emissive + (unlit ? surface.BaseColor.rgb : 0), saturate(surface.Transmission));
  result.NormalRoughness               = float4(normal, max(saturate(surface.Roughness), 0.01f), saturate(surface.Reflectance));
  float2 current                       = input.Position.xy * InvRenderSize * 2 - 1;
  current.y                            = -current.y;
  result.Velocity                      = (current - CurrentJitter) - (input.PreviousClipPosition.xy / max(input.PreviousClipPosition.w, 1e-7f) - PreviousJitter);
  result.ObjectId                      = input.ObjectId;
#  endif
  return result;
}

#endif
