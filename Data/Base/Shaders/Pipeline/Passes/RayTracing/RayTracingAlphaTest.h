/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

// Shared cutout-material evaluation for ray-tracing any-hit shaders. The including stage must
// declare g_RayTracingMaterials, g_RayTracingGeometry, g_RayTracingBuffers and
// g_RayTracingTextures before including this file.
bool xiiShouldIgnoreRayTracingHit(uint instanceIndex, uint primitiveIndex, float2 hitBarycentrics)
{
  const xiiRayTracingMaterialData material = g_RayTracingMaterials[instanceIndex];
  if (material.Rendering.x != 1U) // xiiMaterialAlphaMode::Mask
    return false;

  float alpha = saturate(material.BaseColorOpacity.w);
  const xiiRayTracingGeometryData geometry = g_RayTracingGeometry[instanceIndex];
  if (material.TextureIndices0.x != 0xFFFFFFFFU && geometry.VertexLayout.w != 0xFFFFFFFFU && geometry.VertexAttributes.z != 0U)
  {
    const uint vertexBufferIndex = NonUniformResourceIndex(geometry.BufferIndices.x);
    uint3 indices = primitiveIndex * 3U + uint3(0U, 1U, 2U);
    if (geometry.BufferIndices.z != 0U)
    {
      const uint indexBufferIndex = NonUniformResourceIndex(geometry.BufferIndices.y);
      indices.x = xiiLoadRayTracingIndex(g_RayTracingBuffers[indexBufferIndex], indices.x, geometry.BufferIndices.z);
      indices.y = xiiLoadRayTracingIndex(g_RayTracingBuffers[indexBufferIndex], indices.y, geometry.BufferIndices.z);
      indices.z = xiiLoadRayTracingIndex(g_RayTracingBuffers[indexBufferIndex], indices.z, geometry.BufferIndices.z);
    }

    const float3 barycentrics = float3(1.0f - hitBarycentrics.x - hitBarycentrics.y, hitBarycentrics.x, hitBarycentrics.y);
    const float2 texCoord =
      xiiLoadRayTracingTexCoord(g_RayTracingBuffers[vertexBufferIndex], geometry, indices.x) * barycentrics.x +
      xiiLoadRayTracingTexCoord(g_RayTracingBuffers[vertexBufferIndex], geometry, indices.y) * barycentrics.y +
      xiiLoadRayTracingTexCoord(g_RayTracingBuffers[vertexBufferIndex], geometry, indices.z) * barycentrics.z;
    alpha *= g_RayTracingTextures[NonUniformResourceIndex(material.TextureIndices0.x)].SampleLevel(g_sAnisotropicWrap, texCoord, 0.0f).a;
  }

  return alpha < saturate(material.LayerParameters.x);
}
