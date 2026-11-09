/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include "GpuSceneCommon.h"
StructuredBuffer<GpuSceneInstance>  g_SceneInstances;
StructuredBuffer<GpuVisibilityView> g_VisibilityView;
StructuredBuffer<GpuGeometryRecord> g_Geometry;
StructuredBuffer<GpuMeshlet>        g_Meshlets;
StructuredBuffer<uint>              g_VisibleInstances;
StructuredBuffer<uint>              g_VisibleInstanceCount;
StructuredBuffer<uint4>             g_CurrentLodHistory;
StructuredBuffer<uint>              g_MeshletWorkArguments;
RWStructuredBuffer<uint2>           g_VisibleMeshlets;
RWStructuredBuffer<uint>            g_VisibleMeshletCount;
RWStructuredBuffer<uint>            g_VisibleMeshletOverflow;
ByteAddressBuffer                   g_Buffers[];
#include "MeshletRange.h"
#include <Shaders/Geometry/VirtualGeometry.h>
groupshared uint s_FirstMeshlet;
groupshared uint s_RangeMeshletCount;
#if defined(XII_CULL_DIAGNOSTICS)
RWStructuredBuffer<uint4> g_DiagnosticMeshlets;
RWStructuredBuffer<uint>  g_DiagnosticMeshletCount;
void                      RecordDecision(uint instance, uint meshlet, uint outcome)
{
  uint capacity, stride;
  g_DiagnosticMeshlets.GetDimensions(capacity, stride);
  for (;;)
  {
    const uint count = g_DiagnosticMeshletCount[0];
    if (count >= capacity)
    {
      InterlockedOr(g_VisibleMeshletOverflow[0], 2u);
      return;
    }
    uint observed;
    InterlockedCompareExchange(g_DiagnosticMeshletCount[0], count, count + 1u, observed);
    if (observed == count)
    {
      g_DiagnosticMeshlets[count] = uint4(instance, meshlet, outcome, 0u);
      return;
    }
  }
}
#else
void RecordDecision(uint instance, uint meshlet, uint outcome) {}
#endif
groupshared uint s_VisibleCount;
groupshared uint s_Destination;
groupshared uint s_Granted;
void             ReserveMeshletBatch(uint requested)
{
  uint capacity, stride;
  g_VisibleMeshlets.GetDimensions(capacity, stride);
  s_Destination = 0u;
  s_Granted     = 0u;
  if (requested == 0u)
    return;
  for (;;)
  {
    uint count   = g_VisibleMeshletCount[0];
    uint granted = min(requested, capacity - min(count, capacity));
    if (granted == 0u)
    {
      InterlockedOr(g_VisibleMeshletOverflow[0], 1u);
      return;
    }
    uint observed;
    InterlockedCompareExchange(g_VisibleMeshletCount[0], count, count + granted, observed);
    if (observed == count)
    {
      s_Destination = count;
      s_Granted     = granted;
      if (granted < requested)
        InterlockedOr(g_VisibleMeshletOverflow[0], 1u);
      return;
    }
  }
}
bool CullMeshlet(uint index, uint meshletIndex, GpuSceneInstance instance, GpuVisibilityView view, uint lodIndex)
{
  GpuMeshlet meshlet = g_Meshlets[meshletIndex];
  if ((meshlet.PackedLodSection & 0xFFFFu) != lodIndex)
    return false;
  uint primitives = meshlet.PackedPrimitiveVertexCount & 0xFFFFu;
  if (primitives == 0u || (meshlet.PackedPrimitiveVertexCount >> 16u) == 0u)
    return false;
  if (instance.PrimitiveCount != 0u)
  {
    // Difference comparisons preserve half-open ranges without overflowing uint ends.
    if (meshlet.Header.x >= instance.FirstPrimitive)
    {
      if (meshlet.Header.x - instance.FirstPrimitive >= instance.PrimitiveCount)
        return false;
    }
    else if (instance.FirstPrimitive - meshlet.Header.x >= (meshlet.Padding.y != 0u ? meshlet.Padding.y : primitives))
      return false;
  }
  float3 c0          = float3(instance.GlobalTransform[0][0], instance.GlobalTransform[1][0], instance.GlobalTransform[2][0]);
  float3 c1          = float3(instance.GlobalTransform[0][1], instance.GlobalTransform[1][1], instance.GlobalTransform[2][1]);
  float3 c2          = float3(instance.GlobalTransform[0][2], instance.GlobalTransform[1][2], instance.GlobalTransform[2][2]);
  float3 diagonal    = float3(dot(c0, c0), dot(c1, c1), dot(c2, c2));
  float3 offDiagonal = abs(float3(dot(c0, c1), dot(c0, c2), dot(c1, c2)));
  // The largest Gram-matrix row sum bounds the squared stretch, including shear.
  float3 stretch      = diagonal + float3(offDiagonal.x + offDiagonal.y, offDiagonal.x + offDiagonal.z, offDiagonal.y + offDiagonal.z);
  float  maximumScale = sqrt(max(stretch.x, max(stretch.y, stretch.z)));
  bool   posed        = instance.SkinningCount != 0u;
  float3 center       = posed ? instance.BoundsCenterRadius.xyz : mul(instance.GlobalTransform, float4(meshlet.Bounds.xyz, 1.0f)).xyz;
  float  radius       = posed ? instance.BoundsCenterRadius.w : meshlet.Bounds.w * maximumScale;
  if (!SphereInsideFrustum(center, radius, view))
  {
    RecordDecision(index, meshletIndex, 1u);
    return false;
  }
  float tolerance    = max(max(diagonal.x, max(diagonal.y, diagonal.z)), 0.00000001f) * 0.001f;
  bool  uniformScale = max(diagonal.x, max(diagonal.y, diagonal.z)) - min(diagonal.x, min(diagonal.y, diagonal.z)) <= tolerance &&
    max(offDiagonal.x, max(offDiagonal.y, offDiagonal.z)) <= tolerance;
  if (view.LodParameters.w == 0.0f && !posed && (instance.Flags & (1u << 9u)) == 0u && meshlet.Cone.w >= 0.0f && uniformScale && dot(c0, cross(c1, c2)) > 0.0f)
  {
    float3 axis     = normalize(mul((float3x3)instance.GlobalTransform, meshlet.Cone.xyz));
    float3 toCamera = view.CameraPosition.xyz - center;
    // Include the bounding sphere radius for conservative rejection near a surface.
    if (dot(axis, toCamera) <= -sqrt(saturate(1.0f - meshlet.Cone.w * meshlet.Cone.w)) * length(toCamera) - radius)
    {
      RecordDecision(index, meshletIndex, 2u);
      return false;
    }
  }
  RecordDecision(index, meshletIndex, 0u);
  return true;
}

groupshared uint s_PageStack[64];
groupshared uint s_PageStackSize;
groupshared uint s_SelectedPage;
groupshared uint s_SelectedFirst;
groupshared uint s_SelectedCount;

void CullVirtualHierarchy(uint workGroup, uint workGroups, uint lane, uint index, GpuSceneInstance instance, GpuVisibilityView view, uint lodIndex, GpuGeometryLod lod)
{
  uint table = NonUniformResourceIndex(lod.VertexBufferIndex);
  uint bytes;
  g_Buffers[table].GetDimensions(bytes);
  if (bytes < 272u)
    return;
  uint pageCount = g_Buffers[table].Load(0u);
  if (pageCount > (bytes - 272u) / 48u)
    return;
  uint rootsOffset = 272u + pageCount * 48u;
  uint rootCount   = min(g_Buffers[table].Load(4u), (bytes - rootsOffset) / 4u);
  for (uint root = workGroup; root < rootCount; root += workGroups)
  {
    if (lane == 0u)
    {
      s_PageStackSize = 1u;
      s_PageStack[0]  = g_Buffers[table].Load(rootsOffset + root * 4u);
    }
    GroupMemoryBarrierWithGroupSync();
    while (s_PageStackSize != 0u)
    {
      if (lane == 0u)
      {
        s_SelectedPage                = 0xffffffffu;
        uint                pageIndex = s_PageStack[--s_PageStackSize];
        xiiGeometryPageInfo page      = xiiLoadGeometryPage(lod, pageIndex);
        if (page.Mapping.x != 0xffffffffu && page.Mapping.z >= lod.IndexBufferIndex &&
            page.Mapping.z - lod.IndexBufferIndex < lod.MeshletCount && xiiGeometryPageVisible(page, instance, view))
        {
          uint children = min((uint)page.Detail.w, 8u);
          if (xiiGeometryPageRefines(lod, page, instance, view) && s_PageStackSize + children <= 64u)
          {
            for (uint child = 0u; child < children; ++child)
              s_PageStack[s_PageStackSize++] = (uint)page.Detail.z - 1u + child;
          }
          else
          {
            s_SelectedPage  = pageIndex;
            s_SelectedFirst = lod.MeshletMetadataOffset + page.Mapping.z - lod.IndexBufferIndex;
            s_SelectedCount = min(page.Mapping.w, lod.MeshletCount - (page.Mapping.z - lod.IndexBufferIndex));
            uint feedback   = g_Buffers[table].Load(16u + (lod.Padding >> 1u) * 4u);
            xiiRecordPageDemand(feedback, page.Mapping.y);
          }
        }
      }
      GroupMemoryBarrierWithGroupSync();
      if (s_SelectedPage != 0xffffffffu)
      {
        for (uint base = 0u; base < s_SelectedCount; base += 64u)
        {
          if (lane == 0u)
            s_VisibleCount = 0u;
          GroupMemoryBarrierWithGroupSync();
          uint meshlet = s_SelectedFirst + base + lane;
          uint rank    = 0xffffffffu;
          if (base + lane < s_SelectedCount && CullMeshlet(index, meshlet, instance, view, lodIndex))
            InterlockedAdd(s_VisibleCount, 1u, rank);
          GroupMemoryBarrierWithGroupSync();
          if (lane == 0u)
            ReserveMeshletBatch(s_VisibleCount);
          GroupMemoryBarrierWithGroupSync();
          if (rank < s_Granted)
            g_VisibleMeshlets[s_Destination + rank] = uint2(index, meshlet);
          GroupMemoryBarrierWithGroupSync();
        }
      }
      GroupMemoryBarrierWithGroupSync();
    }
  }
}

[numthreads(64, 1, 1)] void main(uint3 tid : SV_DispatchThreadID, uint3 group : SV_GroupID, uint lane : SV_GroupIndex) {
  uint capacity, stride, sceneCount, geometryCount, meshletCount, historyCount, viewCount;
  g_VisibleInstances.GetDimensions(capacity, stride);
  g_SceneInstances.GetDimensions(sceneCount, stride);
  g_Geometry.GetDimensions(geometryCount, stride);
  g_Meshlets.GetDimensions(meshletCount, stride);
  g_CurrentLodHistory.GetDimensions(historyCount, stride);
  g_VisibilityView.GetDimensions(viewCount, stride);
  uint candidate = group.y + group.z * 65535u;
  if (viewCount == 0u || candidate >= min(g_VisibleInstanceCount[0], capacity))
    return;
  uint index = g_VisibleInstances[candidate];
  if (index >= sceneCount || index >= historyCount)
    return;
  GpuSceneInstance  instance = g_SceneInstances[index];
  GpuVisibilityView view     = g_VisibilityView[0];
  if (view.GeometryBaseIndex >= geometryCount || instance.GeometryIndex >= geometryCount - view.GeometryBaseIndex)
    return;
  GpuGeometryRecord geometry = g_Geometry[view.GeometryBaseIndex + instance.GeometryIndex];
  // Section candidates share one geometry-level LOD decision and representative ID.
  // Validate the same representative that InstanceCull used when publishing history.
  uint selectionIndex = index;
  if (instance.LodSelectionIndex < min(view.InstanceCount, sceneCount) &&
      g_SceneInstances[instance.LodSelectionIndex].GeometryIndex == instance.GeometryIndex)
    selectionIndex = instance.LodSelectionIndex;
  uint4 history = g_CurrentLodHistory[index];
  if (history.x >= min(geometry.LodCount, 8u) || history.y != geometry.Generation || history.z != g_SceneInstances[selectionIndex].ObjectId ||
      (geometry.ResidentLodMask & (1u << history.x)) == 0u || (instance.DrawLod != 0xFFFFFFFFu && instance.DrawLod != history.x))
    return;
  GpuGeometryLod lod = geometry.Lods[history.x];
  if (lod.Padding != 0u)
  {
    CullVirtualHierarchy(group.x, max(g_MeshletWorkArguments[0], 1u), lane, index, instance, view, history.x, lod);
    return;
  }
  if (lane == 0u)
    GetCandidateMeshletRange(instance, lod, s_FirstMeshlet, s_RangeMeshletCount);
  GroupMemoryBarrierWithGroupSync();
  const uint workStride = max(g_MeshletWorkArguments[0], 1u) * 64u;
  // Uniform tile iterations keep group barriers safe even for partial ranges.
  for (uint base = group.x * 64u; base < s_RangeMeshletCount; base += workStride)
  {
    if (lane == 0u)
      s_VisibleCount = 0u;
    GroupMemoryBarrierWithGroupSync();
    uint meshletIndex = lod.MeshletMetadataOffset + s_FirstMeshlet + base + lane;
    uint rank         = 0xFFFFFFFFu;
    if (base + lane < s_RangeMeshletCount)
      if (CullMeshlet(index, meshletIndex, instance, view, history.x))
        InterlockedAdd(s_VisibleCount, 1u, rank);
    GroupMemoryBarrierWithGroupSync();
    if (lane == 0u)
      ReserveMeshletBatch(s_VisibleCount);
    GroupMemoryBarrierWithGroupSync();
    if (rank < s_Granted)
      g_VisibleMeshlets[s_Destination + rank] = uint2(index, meshletIndex);
    GroupMemoryBarrierWithGroupSync();
  }
}
