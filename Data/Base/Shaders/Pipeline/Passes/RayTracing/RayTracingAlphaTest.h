/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

// Shared cutout-material evaluation for ray-tracing any-hit shaders. The including stage must
// declare g_RayTracingMaterials, g_RayTracingGeometry, g_RayTracingBuffers and
// g_RayTracingTextures before including this file.
#include <Shaders/Pipeline/Passes/RayTracing/RayTracingTextureSampling.h>

bool xiiShouldIgnoreRayTracingHitInternal(uint instanceIndex, uint primitiveIndex, float2 hitBarycentrics,
                                          float rayConeWidth, bool useRayCone)
{
  const xiiRayTracingMaterialData material = g_RayTracingMaterials[instanceIndex];
  if (material.Rendering.x != 1U) // xiiMaterialAlphaMode::Mask
    return false;

  float                           alpha    = saturate(material.BaseColorOpacity.w);
  const xiiRayTracingGeometryData geometry = g_RayTracingGeometry[instanceIndex];
  if (material.TextureIndices0.x != 0xFFFFFFFFU && geometry.VertexLayout.w != 0xFFFFFFFFU && geometry.VertexAttributes.z != 0U)
  {
    const uint vertexBufferIndex = NonUniformResourceIndex(geometry.BufferIndices.x);
    uint3      indices           = (geometry.VertexAttributes.w + primitiveIndex) * 3U + uint3(0U, 1U, 2U);
    if (geometry.BufferIndices.z != 0U)
    {
      const uint indexBufferIndex = NonUniformResourceIndex(geometry.BufferIndices.y);
      indices.x                   = xiiLoadRayTracingIndex(g_RayTracingBuffers[indexBufferIndex], indices.x, geometry.BufferIndices.z);
      indices.y                   = xiiLoadRayTracingIndex(g_RayTracingBuffers[indexBufferIndex], indices.y, geometry.BufferIndices.z);
      indices.z                   = xiiLoadRayTracingIndex(g_RayTracingBuffers[indexBufferIndex], indices.z, geometry.BufferIndices.z);
    }

    const float3 barycentrics = float3(1.0f - hitBarycentrics.x - hitBarycentrics.y, hitBarycentrics.x, hitBarycentrics.y);
    const float2 texCoord0    = xiiLoadRayTracingTexCoord(g_RayTracingBuffers[vertexBufferIndex], geometry, indices.x);
    const float2 texCoord1    = xiiLoadRayTracingTexCoord(g_RayTracingBuffers[vertexBufferIndex], geometry, indices.y);
    const float2 texCoord2    = xiiLoadRayTracingTexCoord(g_RayTracingBuffers[vertexBufferIndex], geometry, indices.z);
    const float2 texCoord     = texCoord0 * barycentrics.x + texCoord1 * barycentrics.y + texCoord2 * barycentrics.z;

    if (useRayCone)
    {
      const float3 position0         = xiiLoadRayTracingPosition(g_RayTracingBuffers[vertexBufferIndex], geometry, indices.x);
      const float3 position1         = xiiLoadRayTracingPosition(g_RayTracingBuffers[vertexBufferIndex], geometry, indices.y);
      const float3 position2         = xiiLoadRayTracingPosition(g_RayTracingBuffers[vertexBufferIndex], geometry, indices.z);
      const float3 worldEdge0        = mul((float3x3)ObjectToWorld3x4(), position1 - position0);
      const float3 worldEdge1        = mul((float3x3)ObjectToWorld3x4(), position2 - position0);
      const float  worldTriangleArea = length(cross(worldEdge0, worldEdge1));
      const float2 uvEdge0           = texCoord1 - texCoord0;
      const float2 uvEdge1           = texCoord2 - texCoord0;
      const float  uvTriangleArea    = abs(uvEdge0.x * uvEdge1.y - uvEdge0.y * uvEdge1.x);
      alpha *= xiiSampleRayTracingMaterialTexture(material.TextureIndices0.x, material.SamplerIndices0.x, texCoord, worldTriangleArea,
                                                  uvTriangleArea, max(rayConeWidth, 1e-6f))
                 .a;
    }
    else
    {
      alpha *= xiiSampleSurfaceTextureLevel(g_RayTracingTextures[NonUniformResourceIndex(material.TextureIndices0.x)], material.SamplerIndices0.x, g_sAnisotropicWrap, texCoord, 0.0f).a;
    }
  }

  return alpha < saturate(material.LayerParameters.x);
}

bool xiiShouldIgnoreRayTracingHit(uint instanceIndex, uint primitiveIndex, float2 hitBarycentrics)
{
  return xiiShouldIgnoreRayTracingHitInternal(instanceIndex, primitiveIndex, hitBarycentrics, 0.0f, false);
}

bool xiiShouldIgnoreRayTracingHitWithRayCone(uint instanceIndex, uint primitiveIndex, float2 hitBarycentrics,
                                             float rayConeWidth)
{
  return xiiShouldIgnoreRayTracingHitInternal(instanceIndex, primitiveIndex, hitBarycentrics, rayConeWidth, true);
}
