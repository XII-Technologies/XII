/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Pipeline/Passes/RayTracing/RayTracingTextureSampling.h>

// The hit stage declares the canonical material, geometry and bindless tables.
struct xiiRayTracingSurface
{
  float3 BaseColor, Emission, Normal, GeometricNormal;
  float  Roughness, Metallic, Transmission, ClearCoat, ClearCoatRoughness, Occlusion;
};

xiiRayTracingSurface xiiLoadRayTracingSurface(uint instanceIndex, uint primitiveIndex, float2 hitBarycentrics, float coneWidth, float coneSpread)
{
  const xiiRayTracingMaterialData material          = g_RayTracingMaterials[instanceIndex];
  const xiiRayTracingGeometryData geometry          = g_RayTracingGeometry[instanceIndex];
  const uint                      vertexBufferIndex = NonUniformResourceIndex(geometry.BufferIndices.x);
  uint3                           indices           = (geometry.VertexAttributes.w + primitiveIndex) * 3U + uint3(0U, 1U, 2U);
  if (geometry.BufferIndices.z != 0U)
  {
    const uint indexBufferIndex = NonUniformResourceIndex(geometry.BufferIndices.y);
    indices.x                   = xiiLoadRayTracingIndex(g_RayTracingBuffers[indexBufferIndex], indices.x, geometry.BufferIndices.z);
    indices.y                   = xiiLoadRayTracingIndex(g_RayTracingBuffers[indexBufferIndex], indices.y, geometry.BufferIndices.z);
    indices.z                   = xiiLoadRayTracingIndex(g_RayTracingBuffers[indexBufferIndex], indices.z, geometry.BufferIndices.z);
  }

  const float3 position0    = xiiLoadRayTracingPosition(g_RayTracingBuffers[vertexBufferIndex], geometry, indices.x);
  const float3 position1    = xiiLoadRayTracingPosition(g_RayTracingBuffers[vertexBufferIndex], geometry, indices.y);
  const float3 position2    = xiiLoadRayTracingPosition(g_RayTracingBuffers[vertexBufferIndex], geometry, indices.z);
  const float3 barycentrics = float3(1.0f - hitBarycentrics.x - hitBarycentrics.y, hitBarycentrics.x, hitBarycentrics.y);
  float3       objectNormal = normalize(cross(position1 - position0, position2 - position0));
  if (geometry.VertexLayout.z != 0xFFFFFFFFU)
  {
    objectNormal = normalize(
      xiiLoadRayTracingNormal(g_RayTracingBuffers[vertexBufferIndex], geometry, indices.x) * barycentrics.x +
      xiiLoadRayTracingNormal(g_RayTracingBuffers[vertexBufferIndex], geometry, indices.y) * barycentrics.y +
      xiiLoadRayTracingNormal(g_RayTracingBuffers[vertexBufferIndex], geometry, indices.z) * barycentrics.z);
  }
  float3 worldNormal = normalize(mul(objectNormal, (float3x3)WorldToObject3x4()));

  const bool hasTexCoord = geometry.VertexLayout.w != 0xFFFFFFFFU && geometry.VertexAttributes.z != 0U;
  float2     texCoord0   = 0.0f;
  float2     texCoord1   = 0.0f;
  float2     texCoord2   = 0.0f;
  float2     texCoord    = 0.0f;
  if (hasTexCoord)
  {
    texCoord0 = xiiLoadRayTracingTexCoord(g_RayTracingBuffers[vertexBufferIndex], geometry, indices.x);
    texCoord1 = xiiLoadRayTracingTexCoord(g_RayTracingBuffers[vertexBufferIndex], geometry, indices.y);
    texCoord2 = xiiLoadRayTracingTexCoord(g_RayTracingBuffers[vertexBufferIndex], geometry, indices.z);
    texCoord  = texCoord0 * barycentrics.x + texCoord1 * barycentrics.y + texCoord2 * barycentrics.z;
  }

  const float3 worldEdge0        = mul((float3x3)ObjectToWorld3x4(), position1 - position0);
  const float3 worldEdge1        = mul((float3x3)ObjectToWorld3x4(), position2 - position0);
  const float  worldTriangleArea = length(cross(worldEdge0, worldEdge1));
  const float2 uvEdge0           = texCoord1 - texCoord0;
  const float2 uvEdge1           = texCoord2 - texCoord0;
  const float  uvTriangleArea    = hasTexCoord ? abs(uvEdge0.x * uvEdge1.y - uvEdge0.y * uvEdge1.x) : 0.0f;
  const float  rayConeWidth      = max(coneWidth + RayTCurrent() * coneSpread, 1e-6f);

  if (hasTexCoord && geometry.VertexAttributes.x != 0xFFFFFFFFU && geometry.VertexAttributes.y != 0U && material.TextureIndices0.y != 0xFFFFFFFFU)
  {
    const float4 objectTangent =
      xiiLoadRayTracingTangent(g_RayTracingBuffers[vertexBufferIndex], geometry, indices.x) * barycentrics.x +
      xiiLoadRayTracingTangent(g_RayTracingBuffers[vertexBufferIndex], geometry, indices.y) * barycentrics.y +
      xiiLoadRayTracingTangent(g_RayTracingBuffers[vertexBufferIndex], geometry, indices.z) * barycentrics.z;
    float3 worldTangent         = normalize(mul((float3x3)ObjectToWorld3x4(), objectTangent.xyz));
    worldTangent                = normalize(worldTangent - worldNormal * dot(worldTangent, worldNormal));
    const float3 worldBitangent = cross(worldNormal, worldTangent) * (objectTangent.w < 0.0f ? -1.0f : 1.0f);
    const float3 tangentNormal  = xiiDecodeRayTracingNormal(
      xiiSampleRayTracingMaterialTexture(material.TextureIndices0.y, material.SamplerIndices0.y, texCoord, worldTriangleArea, uvTriangleArea, rayConeWidth),
      material.LayerParameters.y);
    worldNormal = normalize(worldTangent * tangentNormal.x + worldBitangent * tangentNormal.y + worldNormal * tangentNormal.z);
  }
  if (dot(worldNormal, WorldRayDirection()) > 0.0f)
    worldNormal = -worldNormal;

  float3 baseColor          = max(material.BaseColorOpacity.rgb, 0.0f);
  float3 emissive           = max(material.EmissiveColorAndRoughness.rgb, 0.0f);
  float  roughness          = saturate(material.EmissiveColorAndRoughness.w);
  float  metallic           = saturate(material.SurfaceParameters.x);
  float  transmission       = saturate(material.SurfaceParameters.z);
  float  clearCoat          = saturate(material.LayerParameters.z);
  float  clearCoatRoughness = saturate(material.LayerParameters.w);
  float  occlusion          = 1.0f;
  if (hasTexCoord && material.TextureIndices0.x != 0xFFFFFFFFU)
    baseColor *= max(xiiSampleRayTracingMaterialTexture(material.TextureIndices0.x, material.SamplerIndices0.x, texCoord, worldTriangleArea, uvTriangleArea, rayConeWidth).rgb, 0.0f);
  if (hasTexCoord && material.TextureIndices0.z != 0xFFFFFFFFU)
  {
    const float4 metallicRoughness = xiiSampleRayTracingMaterialTexture(material.TextureIndices0.z, material.SamplerIndices0.z, texCoord, worldTriangleArea, uvTriangleArea, rayConeWidth);
    roughness *= metallicRoughness.g;
    metallic *= metallicRoughness.b;
  }
  if (hasTexCoord && material.TextureIndices0.w != 0xFFFFFFFFU)
  {
    const float sampledOcclusion = xiiSampleRayTracingMaterialTexture(material.TextureIndices0.w, material.SamplerIndices0.w, texCoord, worldTriangleArea, uvTriangleArea, rayConeWidth).r;
    occlusion                    = lerp(1.0f, sampledOcclusion, saturate(material.SurfaceParameters.w));
  }
  if (hasTexCoord && material.TextureIndices1.x != 0xFFFFFFFFU)
    emissive *= max(xiiSampleRayTracingMaterialTexture(material.TextureIndices1.x, material.SamplerIndices1.x, texCoord, worldTriangleArea, uvTriangleArea, rayConeWidth).rgb, 0.0f);
  if (hasTexCoord && material.TextureIndices1.z != 0xFFFFFFFFU)
  {
    const float2 coatSample = xiiSampleRayTracingMaterialTexture(material.TextureIndices1.z, material.SamplerIndices1.z, texCoord, worldTriangleArea, uvTriangleArea, rayConeWidth).rg;
    clearCoat *= coatSample.r;
    clearCoatRoughness *= coatSample.g;
  }
  if (hasTexCoord && material.TextureIndices1.w != 0xFFFFFFFFU)
    transmission *= saturate(xiiSampleRayTracingMaterialTexture(material.TextureIndices1.w, material.SamplerIndices1.w, texCoord, worldTriangleArea, uvTriangleArea, rayConeWidth).r);

  roughness          = max(roughness, 0.01f);
  clearCoatRoughness = max(clearCoatRoughness, 0.01f);
  xiiRayTracingSurface result;
  result.BaseColor          = baseColor;
  result.Emission           = emissive;
  result.Normal             = worldNormal;
  result.GeometricNormal    = worldTriangleArea > 1e-12f ? cross(worldEdge0, worldEdge1) / worldTriangleArea : 0;
  result.Roughness          = roughness;
  result.Metallic           = metallic;
  result.Transmission       = transmission;
  result.ClearCoat          = clearCoat;
  result.ClearCoatRoughness = clearCoatRoughness;
  result.Occlusion          = occlusion;
  return result;
}
