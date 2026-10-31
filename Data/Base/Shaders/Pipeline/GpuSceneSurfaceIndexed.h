/// Copyright (c) Theophilus Eriata. All Rights Reserved.

// XII Engine - canonical surface vertices pulled from resident meshlet indices.
#pragma once

StructuredBuffer<GpuSceneInstance>  g_SceneInstances;
StructuredBuffer<GpuGeometryRecord> g_Geometry;
StructuredBuffer<GpuMeshlet>        g_Meshlets;
#if defined(XII_SCENE_CULL_DIAGNOSTICS)
StructuredBuffer<uint4> g_VisibleMeshlets;
#else
StructuredBuffer<uint2> g_VisibleMeshlets;
#endif
ByteAddressBuffer g_Buffers[];

#include <Shaders/Pipeline/GpuSceneSurfaceData.h>

GBufferVertex xiiLoadIndexedSceneSurfaceVertex(uint vertexIndex, uint visibleIndex)
{
  uint sceneCapacity, visibleCapacity, meshletCapacity, geometryCapacity, stride;
  g_SceneInstances.GetDimensions(sceneCapacity, stride);
  g_VisibleMeshlets.GetDimensions(visibleCapacity, stride);
  g_Meshlets.GetDimensions(meshletCapacity, stride);
  g_Geometry.GetDimensions(geometryCapacity, stride);
  if (visibleIndex >= visibleCapacity)
    return xiiInvalidSceneSurfaceVertex();
  const uint2 visible       = g_VisibleMeshlets[visibleIndex].xy;
  const uint  instanceIndex = visible.x;
  if (instanceIndex >= sceneCapacity || visible.y >= meshletCapacity)
    return xiiInvalidSceneSurfaceVertex();
  const GpuSceneInstance instance = g_SceneInstances[instanceIndex];
  if (GeometryBaseIndex >= geometryCapacity || instance.GeometryIndex >= geometryCapacity - GeometryBaseIndex)
    return xiiInvalidSceneSurfaceVertex();
  const GpuGeometryRecord geometry = g_Geometry[GeometryBaseIndex + instance.GeometryIndex];
  const uint              lodIndex = g_Meshlets[visible.y].PackedLodSection & 0xFFFFu;
  if (lodIndex >= min(geometry.LodCount, 8u))
    return xiiInvalidSceneSurfaceVertex();
  const GpuGeometryLod lod            = geometry.Lods[lodIndex];
  const GpuMeshlet     meshlet        = g_Meshlets[visible.y];
  const uint           primitiveCount = meshlet.PackedPrimitiveVertexCount & 0xFFFFu;
  const uint           vertexCount    = meshlet.PackedPrimitiveVertexCount >> 16u;
  const uint           firstIndex     = vertexIndex - vertexIndex % 3u;
  uint                 primitiveBufferBytes, remapBufferBytes;
  g_Buffers[lod.MeshletPrimitiveBufferIndex].GetDimensions(primitiveBufferBytes);
  g_Buffers[lod.MeshletRemapBufferIndex].GetDimensions(remapBufferBytes);
  if (primitiveCount > 126u || vertexCount == 0u || vertexCount > 64u || firstIndex / 3u >= primitiveCount ||
      meshlet.Header.y > remapBufferBytes / 4u || vertexCount > remapBufferBytes / 4u - meshlet.Header.y ||
      meshlet.Header.z > primitiveBufferBytes || primitiveCount * 3u > primitiveBufferBytes - meshlet.Header.z)
    return xiiInvalidSceneSurfaceVertex();
  // Use the canonical meshlet indices: meshoptimizer may reorder source-buffer triangles.
  // This keeps indexed and mesh-shader rasterization consistent with meshlet culling bounds.
  uint3 indices;
  [unroll] for (uint i = 0u; i < 3u; ++i)
  {
    const uint localIndex = LoadByte(g_Buffers[lod.MeshletPrimitiveBufferIndex], meshlet.Header.z + firstIndex + i);
    if (localIndex >= vertexCount)
      return xiiInvalidSceneSurfaceVertex();
    indices[i] = g_Buffers[lod.MeshletRemapBufferIndex].Load((meshlet.Header.y + localIndex) * 4u);
  }
  // Reject the whole triangle consistently, including malformed source indices.
  uint vertexBufferBytes;
  g_Buffers[lod.VertexBufferIndex].GetDimensions(vertexBufferBytes);
  const uint attributeEnd = max(max(12u, NormalOffset + 12u), max(TangentOffset + 16u, TexCoordOffset + 8u));
  if (VertexStride < attributeEnd || vertexBufferBytes < attributeEnd || any(indices >= lod.VertexCount) ||
      any(indices > (vertexBufferBytes - attributeEnd) / VertexStride))
    return xiiInvalidSceneSurfaceVertex();
  const uint sourcePrimitive = meshlet.Header.x + firstIndex / 3u;
  if ((instance.DrawLod != 0xFFFFFFFFu && instance.DrawLod != lodIndex) ||
      (instance.PrimitiveCount != 0u && (sourcePrimitive < instance.FirstPrimitive || sourcePrimitive - instance.FirstPrimitive >= instance.PrimitiveCount)))
    return xiiInvalidSceneSurfaceVertex();
  GBufferVertex output = xiiLoadSceneSurfaceVertex(instance, lod, indices[vertexIndex % 3u]);
#if defined(XII_SCENE_MATERIAL_ATTRIBUTES)
  output.InstanceIndex = instanceIndex;
  output.Barycentric   = vertexIndex % 3u == 0u ? float3(1, 0, 0) : vertexIndex % 3u == 1u ? float3(0, 1, 0) :
                                                                                             float3(0, 0, 1);
#endif
#if defined(XII_SCENE_DIAGNOSTICS)
  output.DiagnosticIdentity.yz = uint2(lodIndex, visible.y);
  output.DiagnosticIdentity.w  = sourcePrimitive + 1u;
#  if defined(XII_SCENE_CULL_DIAGNOSTICS)
  output.DiagnosticDecision = g_VisibleMeshlets[visibleIndex].z;
#  endif
  output.DiagnosticBarycentrics = vertexIndex % 3u == 0u ? float2(1, 0) : vertexIndex % 3u == 1u ? float2(0, 1) :
                                                                                                   float2(0, 0);
#endif
  return output;
}

#if !defined(XII_SCENE_CUSTOM_VERTEX_ENTRY)
GBufferVertex main(uint vertexIndex : SV_VertexID, uint visibleIndex : SV_InstanceID)
{
  return xiiLoadIndexedSceneSurfaceVertex(vertexIndex, visibleIndex);
}
#endif
