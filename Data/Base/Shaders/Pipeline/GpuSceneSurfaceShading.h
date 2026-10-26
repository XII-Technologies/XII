/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Pipeline/GpuSceneSurfaceMaterial.h>
#include <Shaders/Pipeline/GpuSceneSurfaceVertex.h>

/// Canonical texture and normal evaluation shared by deferred and transparent rasterization.
struct xiiSceneSurfaceSample
{
  float4 BaseColor;
  float3 Emissive;
  float  Roughness;
  float  Metallic;
  float  Occlusion;
  float  Transmission;
  float3 Normal;
};
xiiSceneSurfaceSample xiiSampleSceneSurface(xiiGpuSurfaceMaterial material, GBufferVertex input, bool isFrontFace)
{
  float4 baseColor = xiiSampleSceneBaseColor(material, input.TexCoord);
  xiiClipSceneSurface(material, baseColor, isFrontFace, false);
  float3 emissive     = xiiSampleSceneEmissive(material, input.TexCoord);
  float  roughness    = saturate(material.EmissiveColorAndRoughness.w);
  float  metallic     = saturate(material.SurfaceParameters.x);
  float  occlusion    = 1.0f;
  float  transmission = saturate(material.SurfaceParameters.z);

  if (material.TextureIndices0.z != 0xFFFFFFFFu)
  {
    const float4 metallicRoughness = xiiSampleSurfaceTexture(g_Textures[NonUniformResourceIndex(material.TextureIndices0.z)], material.SamplerIndices0.z, g_sLinearWrap, input.TexCoord);
    roughness *= metallicRoughness.g;
    metallic *= metallicRoughness.b;
  }
  if (material.TextureIndices0.w != 0xFFFFFFFFu)
  {
    const float sampledOcclusion = xiiSampleSurfaceTexture(g_Textures[NonUniformResourceIndex(material.TextureIndices0.w)], material.SamplerIndices0.w, g_sLinearWrap, input.TexCoord).r;
    occlusion                    = lerp(1.0f, sampledOcclusion, saturate(material.SurfaceParameters.w));
  }
  if (material.TextureIndices1.w != 0xFFFFFFFFu)
    transmission *= saturate(xiiSampleSurfaceTexture(g_Textures[NonUniformResourceIndex(material.TextureIndices1.w)], material.SamplerIndices1.w, g_sLinearWrap, input.TexCoord).r);

  float3 worldNormal = normalize(input.WorldNormal) * (isFrontFace ? 1.0f : -1.0f);
  if (material.TextureIndices0.y != 0xFFFFFFFFu)
  {
    float3 tangentNormal = xiiSampleSurfaceTexture(g_Textures[NonUniformResourceIndex(material.TextureIndices0.y)], material.SamplerIndices0.y, g_sAnisotropicWrap, input.TexCoord).xyz * 2.0f - 1.0f;
    tangentNormal.xy *= material.LayerParameters.y;
    tangentNormal.z             = sqrt(saturate(1.0f - dot(tangentNormal.xy, tangentNormal.xy)));
    const float3 worldTangent   = normalize(input.WorldTangent);
    const float3 worldBitangent = cross(worldNormal, worldTangent) * (input.TangentSign < 0.0f ? -1.0f : 1.0f);
    worldNormal                 = normalize(worldTangent * tangentNormal.x + worldBitangent * tangentNormal.y + worldNormal * tangentNormal.z);
  }


  xiiSceneSurfaceSample result;
  result.BaseColor    = baseColor;
  result.Emissive     = emissive;
  result.Roughness    = roughness;
  result.Metallic     = metallic;
  result.Occlusion    = occlusion;
  result.Transmission = transmission;
  result.Normal       = worldNormal;
  return result;
}
