/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once

uint4 xiiGeometryPageMapping(GpuGeometryLod lod, uint page)
{
  if (lod.VertexBufferIndex == 0xFFFFFFFFu)
    return uint4(0xFFFFFFFFu, 0u, 0u, 0u);
  uint bytes;
  g_Buffers[NonUniformResourceIndex(lod.VertexBufferIndex)].GetDimensions(bytes);
  if (bytes < 16u || page >= g_Buffers[NonUniformResourceIndex(lod.VertexBufferIndex)].Load(0u) || page >= (bytes - 16u) / 48u)
    return uint4(0xFFFFFFFFu, 0u, 0u, 0u);
  return g_Buffers[NonUniformResourceIndex(lod.VertexBufferIndex)].Load4(16u + page * 48u);
}

bool xiiResolveClusterLod(inout GpuGeometryLod lod, inout GpuMeshlet cluster)
{
  if (lod.Padding == 0u)
    return true;
  uint4 mapping = xiiGeometryPageMapping(lod, cluster.Page.x);
  if (mapping.x == 0xFFFFFFFFu)
    return false;
  lod.VertexBufferIndex = mapping.x;
  lod.MeshletTopologyBufferIndex = mapping.x;
  lod.MeshletPrimitiveByteOffset = 0u;
  lod.VertexCount = cluster.Page.w;
  cluster.Header.y = cluster.Page.y / 4u;
  cluster.Header.z = cluster.Page.z;
  return true;
}

bool xiiClusterWantsDetail(GpuMeshlet cluster, GpuSceneInstance instance, GpuVisibilityView view)
{
  float3 center = mul(instance.GlobalTransform, float4(cluster.Bounds.xyz, 1.0f)).xyz;
  float scale = max(length(instance.GlobalTransform[0].xyz), max(length(instance.GlobalTransform[1].xyz), length(instance.GlobalTransform[2].xyz)));
  float distance = max(0.001f, length(center - view.CameraPosition.xyz) - cluster.Bounds.w * scale);
  return cluster.Hierarchy.x * scale * view.ViewportAndHiZ.y * exp2(clamp(instance.LodBias, -16.0f, 16.0f)) / distance > 1.0f;
}

bool xiiSelectCluster(GpuGeometryLod lod, GpuMeshlet cluster, GpuSceneInstance instance, GpuVisibilityView view)
{
  if (lod.Padding == 0u)
    return true;
  uint detailPage = (uint)cluster.Hierarchy.y - 1u;
  bool refined = xiiClusterWantsDetail(cluster, instance, view) && xiiGeometryPageMapping(lod, detailPage).x != 0xFFFFFFFFu;
  return cluster.Hierarchy.z != 0.0f ? !refined : refined;
}
