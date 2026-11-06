/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

// Included after the canonical g_Meshlets declaration. Ordered endpoints allow
// section candidates to skip unrelated meshlets. Unordered authored ranges retain
// the complete scan and the existing exact per-meshlet overlap test.
void GetCandidateMeshletRange(GpuSceneInstance instance, GpuGeometryLod lod, out uint first, out uint count)
{
  uint capacity, stride;
  g_Meshlets.GetDimensions(capacity, stride);
  first = 0u;
  count = lod.MeshletMetadataOffset < capacity ? min(lod.MeshletCount, capacity - lod.MeshletMetadataOffset) : 0u;
  if (instance.PrimitiveCount == 0u || lod.MeshletRangesOrdered == 0u || count == 0u)
    return;
  uint low = 0u, high = count;
  while (low < high)
  {
    uint       middle  = low + (high - low) / 2u;
    GpuMeshlet meshlet = g_Meshlets[lod.MeshletMetadataOffset + middle];
    uint       end     = meshlet.Header.x + (meshlet.PackedPrimitiveVertexCount & 0xFFFFu);
    if (end <= instance.FirstPrimitive)
      low = middle + 1u;
    else
      high = middle;
  }
  first = low;
  high  = count;
  while (low < high)
  {
    uint middle = low + (high - low) / 2u;
    uint start  = g_Meshlets[lod.MeshletMetadataOffset + middle].Header.x;
    if (start < instance.FirstPrimitive || start - instance.FirstPrimitive < instance.PrimitiveCount)
      low = middle + 1u;
    else
      high = middle;
  }
  count = low - first;
}
