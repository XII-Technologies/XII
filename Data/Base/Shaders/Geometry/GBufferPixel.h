/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Pipeline/GBufferMaterial.h>
#include <Shaders/Pipeline/GpuSceneSurfaceShading.h>

struct GBufferOutput
{
  float4 Albedo : SV_Target0;
  float2 Normal : SV_Target1;
  float4 Material : SV_Target2;
  float4 Emissive : SV_Target3;
  float4 NormalRoughness : SV_Target4;
  float2 Velocity : SV_Target5;
  uint   ObjectId : SV_Target6;
};

GBufferOutput main(GBufferVertex input, bool isFrontFace : SV_IsFrontFace)
{
  xiiGpuSurfaceMaterial material = xiiLoadSceneSurfaceMaterial(input.MaterialIndex, MaterialBaseIndex);

  const xiiMaterialSurface surface   = xiiSampleSceneSurface(material, input, isFrontFace);
  const float4                baseColor = surface.BaseColor;
  const bool                  unlit     = material.Metadata.x == 6u || (material.Metadata.y & (1u << 12u)) == 0u;
  const float3                emissive  = surface.Emissive + (unlit ? baseColor.rgb : 0);
  const float                 roughness = surface.Roughness, metallic = surface.Metallic, occlusion = surface.Occlusion, transmission = surface.Transmission;
  const float3                worldNormal = surface.Normal;

  const float2 octNormal   = EncodeOctNormal(worldNormal);
  const float  featureByte = xiiPackGBufferMaterialFlags(material.Metadata.y, unlit);

  GBufferOutput output;
  output.Albedo   = float4(baseColor.rgb, occlusion);
  output.Normal   = octNormal;
  output.Material = float4(max(roughness, 0.01f), metallic, saturate(material.SurfaceParameters.y), featureByte);
  // Emissive alpha carries the scalar transmission factor. Emissive composition
  // consumes RGB only, leaving one full-precision channel for screen refraction.
  output.Emissive          = float4(emissive, transmission);
  output.NormalRoughness   = float4(octNormal, max(roughness, 0.01f), saturate(material.SurfaceParameters.y));
  float2 currentNDC        = input.Position.xy * InvRenderSize * 2.0f - 1.0f;
  currentNDC.y             = -currentNDC.y;
  const float2 previousNDC = input.PreviousClipPosition.xy / max(input.PreviousClipPosition.w, 1e-7f);
  output.Velocity          = (currentNDC - CurrentJitter) - (previousNDC - PreviousJitter);
  output.ObjectId          = input.ObjectId;
  return output;
}
