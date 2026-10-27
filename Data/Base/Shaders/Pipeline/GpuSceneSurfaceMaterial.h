/// Copyright (c) Theophilus Eriata. All Rights Reserved.

// XII Engine - common canonical surface loading and alpha coverage.

#pragma once

#include <Shaders/Materials/GpuSurfaceSampling.h>

StructuredBuffer<xiiGpuSurfaceMaterial> g_SurfaceMaterials;
Texture2D<float4>                       g_Textures[];

xiiGpuSurfaceMaterial xiiLoadSceneSurfaceMaterial(uint materialIndex, uint materialBaseIndex)
{
  uint materialCount, materialStride;
  g_SurfaceMaterials.GetDimensions(materialCount, materialStride);
  const bool materialIsValid = materialIndex != 0xFFFFFFFFu && materialBaseIndex < materialCount && materialIndex < materialCount - materialBaseIndex;

  xiiGpuSurfaceMaterial material       = (xiiGpuSurfaceMaterial)0;
  material.BaseColorOpacity            = 1.0f;
  material.EmissiveColorAndRoughness.w = 0.5f;
  material.SurfaceParameters           = float4(0.0f, 0.5f, 0.0f, 1.0f);
  material.LayerParameters             = float4(0.5f, 1.0f, 0.0f, 0.1f);
  material.TextureIndices0             = 0xFFFFFFFFu;
  material.TextureIndices1             = 0xFFFFFFFFu;
  material.SamplerIndices0             = 0xFFFFFFFFu;
  material.SamplerIndices1             = 0xFFFFFFFFu;
  material.Metadata.y                  = 0xF000u;
  if (materialIsValid)
    material = g_SurfaceMaterials[materialBaseIndex + materialIndex];

  return material;
}

float4 xiiSampleSceneBaseColor(xiiGpuSurfaceMaterial material, float2 texCoord)
{
  float4 color = max(material.BaseColorOpacity, 0.0f);
  if (material.TextureIndices0.x != 0xFFFFFFFFu)
    color *= max(xiiSampleSurfaceTexture(g_Textures[NonUniformResourceIndex(material.TextureIndices0.x)], material.SamplerIndices0.x, g_sAnisotropicWrap, texCoord), 0.0f);
  return color;
}

float3 xiiSampleSceneEmissive(xiiGpuSurfaceMaterial material, float2 texCoord)
{
  float3 emissive = max(material.EmissiveColorAndRoughness.rgb, 0);
  if (material.TextureIndices1.x != 0xFFFFFFFFu)
    emissive *= max(xiiSampleSurfaceTexture(g_Textures[NonUniformResourceIndex(material.TextureIndices1.x)], material.SamplerIndices1.x, g_sLinearWrap, texCoord).rgb, 0);
  return emissive;
}

void xiiClipSceneSurface(xiiGpuSurfaceMaterial material, float4 color, bool isFrontFace, bool pickTranslucent)
{
  if (!isFrontFace && (material.Metadata.y & (1u << 10u)) == 0u)
    clip(-1.0f);
  if (material.Metadata.z == 1u)
    clip(color.a - material.LayerParameters.x);
  else if (pickTranslucent && material.Metadata.z >= 2u)
    clip(color.a - 1e-5f);
}
