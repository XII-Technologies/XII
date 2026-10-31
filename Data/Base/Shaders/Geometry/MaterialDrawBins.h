/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
StructuredBuffer<uint2>   g_VisibleMeshlets;
StructuredBuffer<uint>    g_VisibleMeshletCount;
StructuredBuffer<uint>    g_InstanceBins;
RWStructuredBuffer<uint>  g_BinCounts;
RWStructuredBuffer<uint4> g_BinDraws;
RWStructuredBuffer<uint2> g_BinnedMeshlets;

uint xiiCandidateBin(uint index)
{
  uint capacity, stride;
  g_VisibleMeshlets.GetDimensions(capacity, stride);
  if (index >= min(g_VisibleMeshletCount[0], capacity))
    return 0xFFFFFFFFu;
  const uint instance = g_VisibleMeshlets[index].x;
  g_InstanceBins.GetDimensions(capacity, stride);
  if (instance >= capacity)
    return 0xFFFFFFFFu;
  const uint bin = g_InstanceBins[instance];
  g_BinCounts.GetDimensions(capacity, stride);
  return bin < capacity ? bin : 0xFFFFFFFFu;
}

#if XII_MATERIAL_BIN_PHASE == 0
[numthreads(64, 1, 1)] void main(uint3 tid : SV_DispatchThreadID) {
  const uint bin = xiiCandidateBin(tid.x);
  if (bin != 0xFFFFFFFFu)
    InterlockedAdd(g_BinCounts[bin], 1u);
}
#elif XII_MATERIAL_BIN_PHASE == 1
[numthreads(1, 1, 1)] void main() {
  uint count, stride;
  g_BinCounts.GetDimensions(count, stride);
  uint offset = 0;
  for (uint bin = 0; bin < count; ++bin)
  {
    const uint size = g_BinCounts[bin];
    g_BinDraws[bin] = uint4(378u, size, 0u, offset);
    offset += size;
    g_BinCounts[bin] = 0u;
  }
}
#else
[numthreads(64, 1, 1)] void main(uint3 tid : SV_DispatchThreadID) {
  const uint bin = xiiCandidateBin(tid.x);
  if (bin == 0xFFFFFFFFu)
    return;
  uint index;
  InterlockedAdd(g_BinCounts[bin], 1u, index);
  g_BinnedMeshlets[g_BinDraws[bin].w + index] = g_VisibleMeshlets[tid.x];
}
#endif
