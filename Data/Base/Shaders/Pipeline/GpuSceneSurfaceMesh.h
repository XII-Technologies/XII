/// Copyright (c) Theophilus Eriata. All Rights Reserved.

// XII Engine - shared canonical GPU-scene surface mesh rasterization.

#pragma once

StructuredBuffer<GpuSceneInstance>  g_SceneInstances;
StructuredBuffer<GpuGeometryRecord> g_Geometry;
StructuredBuffer<GpuMeshlet>        g_Meshlets;
StructuredBuffer<uint2>             g_VisibleMeshlets;
StructuredBuffer<uint>              g_VisibleMeshletCount;
ByteAddressBuffer                   g_Buffers[];

#include <Shaders/Pipeline/GpuSceneSurfaceData.h>

[outputtopology("triangle")]
  [numthreads(64, 1, 1)] void main(uint                       threadIndex : SV_GroupThreadID,
                                   uint3                      groupIndex : SV_GroupID,
                                   out vertices GBufferVertex vertices[64],
                                   out indices uint3          triangles[126]) {
    uint visibleCountCapacity, visibleCountStride;
    uint visibleCapacity, visibleStride;
    uint sceneCapacity, sceneStride;
    uint geometryCapacity, geometryStride;
    uint meshletCapacity, meshletStride;
    g_VisibleMeshletCount.GetDimensions(visibleCountCapacity, visibleCountStride);
    g_VisibleMeshlets.GetDimensions(visibleCapacity, visibleStride);
    g_SceneInstances.GetDimensions(sceneCapacity, sceneStride);
    g_Geometry.GetDimensions(geometryCapacity, geometryStride);
    g_Meshlets.GetDimensions(meshletCapacity, meshletStride);

    const uint visibleMeshletIndex = groupIndex.x +
      groupIndex.y * MeshDispatchGroupCountX +
      groupIndex.z * MeshDispatchGroupCountX * MeshDispatchGroupCountY;
    if (visibleCountCapacity == 0u || visibleMeshletIndex >= min(g_VisibleMeshletCount[0], visibleCapacity))
    {
      SetMeshOutputCounts(0u, 0u);
      return;
    }

    const uint2 visible = g_VisibleMeshlets[visibleMeshletIndex];
    if (visible.x >= sceneCapacity || visible.y >= meshletCapacity)
    {
      SetMeshOutputCounts(0u, 0u);
      return;
    }

    const GpuSceneInstance instance = g_SceneInstances[visible.x];
    if (GeometryBaseIndex >= geometryCapacity || instance.GeometryIndex >= geometryCapacity - GeometryBaseIndex)
    {
      SetMeshOutputCounts(0u, 0u);
      return;
    }

    const GpuGeometryRecord geometry = g_Geometry[GeometryBaseIndex + instance.GeometryIndex];
    const GpuMeshlet        meshlet  = g_Meshlets[visible.y];
    const uint              lodIndex = meshlet.PackedLodSection & 0xFFFFu;
    if (lodIndex >= min(geometry.LodCount, 8u) || (instance.DrawLod != 0xFFFFFFFFu && instance.DrawLod != lodIndex))
    {
      SetMeshOutputCounts(0u, 0u);
      return;
    }

    const GpuGeometryLod lod            = geometry.Lods[lodIndex];
    const uint           primitiveCount = meshlet.PackedPrimitiveVertexCount & 0xFFFFu;
    const uint           vertexCount    = meshlet.PackedPrimitiveVertexCount >> 16u;
    if (instance.PrimitiveCount != 0u)
    {
      const uint selectedEnd = instance.FirstPrimitive + instance.PrimitiveCount;
      const uint meshletEnd  = meshlet.Header.x + primitiveCount;
      if (meshlet.Header.x >= selectedEnd || meshletEnd <= instance.FirstPrimitive)
      {
        SetMeshOutputCounts(0u, 0u);
        return;
      }
    }
    if (vertexCount > 64u || primitiveCount > 126u || (primitiveCount > 0u && vertexCount == 0u) ||
        lod.VertexBufferIndex == 0xFFFFFFFFu || lod.MeshletRemapBufferIndex == 0xFFFFFFFFu || lod.MeshletPrimitiveBufferIndex == 0xFFFFFFFFu)
    {
      SetMeshOutputCounts(0u, 0u);
      return;
    }

    uint vertexBufferBytes;
    uint remapBufferBytes;
    uint primitiveBufferBytes;
    g_Buffers[lod.VertexBufferIndex].GetDimensions(vertexBufferBytes);
    g_Buffers[lod.MeshletRemapBufferIndex].GetDimensions(remapBufferBytes);
    g_Buffers[lod.MeshletPrimitiveBufferIndex].GetDimensions(primitiveBufferBytes);
    const uint lastAttributeEnd = max(max(12u, NormalOffset + 12u), max(TangentOffset + 16u, TexCoordOffset + 8u));
    if (VertexStride < lastAttributeEnd || meshlet.Header.y > remapBufferBytes / 4u || vertexCount > remapBufferBytes / 4u - meshlet.Header.y ||
        meshlet.Header.z > primitiveBufferBytes || primitiveCount * 3u > primitiveBufferBytes - meshlet.Header.z)
    {
      SetMeshOutputCounts(0u, 0u);
      return;
    }

    SetMeshOutputCounts(vertexCount, primitiveCount);

    if (threadIndex < vertexCount)
    {
      const uint sourceVertex = g_Buffers[lod.MeshletRemapBufferIndex].Load((meshlet.Header.y + threadIndex) * 4u);
      vertices[threadIndex]   = xiiLoadSceneSurfaceVertex(instance, lod, sourceVertex);
    }

    // A meshlet can contain more primitives than the 64 lanes in this workgroup.
    for (uint primitiveIndex = threadIndex; primitiveIndex < primitiveCount; primitiveIndex += 64u)
    {
      const uint  sourcePrimitive  = meshlet.Header.x + primitiveIndex;
      const uint  primitiveOffset  = meshlet.Header.z + primitiveIndex * 3u;
      const uint3 primitiveIndices = uint3(
        LoadByte(g_Buffers[lod.MeshletPrimitiveBufferIndex], primitiveOffset + 0u),
        LoadByte(g_Buffers[lod.MeshletPrimitiveBufferIndex], primitiveOffset + 1u),
        LoadByte(g_Buffers[lod.MeshletPrimitiveBufferIndex], primitiveOffset + 2u));
      const bool primitiveSelected = instance.PrimitiveCount == 0u ||
        (sourcePrimitive >= instance.FirstPrimitive && sourcePrimitive - instance.FirstPrimitive < instance.PrimitiveCount);
      triangles[primitiveIndex] = primitiveSelected && all(primitiveIndices < vertexCount) ? primitiveIndices : uint3(0u, 0u, 0u);
    }
  }
