/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <Shaders/Streaming/GpuResidencyFeedback.h>

// Visible clusters reference their physical page directly: flag, authored LOD
// (3 bits), descriptor (12 bits), and metadata offset in words (16 bits).
// Page/descriptor retirement protects this identity until every using frame completes.
bool xiiLoadCluster(uint key, out GpuMeshlet cluster)
{
  cluster = (GpuMeshlet)0;
  if ((key & 0x80000000u) == 0u)
  {
    uint capacity, stride;
    g_Meshlets.GetDimensions(capacity, stride);
    if (key >= capacity) return false;
    cluster = g_Meshlets[key];
    return true;
  }
  uint descriptor = NonUniformResourceIndex((key >> 16u) & 0xfffu);
  uint offset = (key & 0xffffu) * 4u;
  uint bytes;
  g_Buffers[descriptor].GetDimensions(bytes);
  if (offset > bytes || 96u > bytes - offset) return false;
  cluster.Header = g_Buffers[descriptor].Load4(offset);
  uint4 packed = g_Buffers[descriptor].Load4(offset + 16u);
  cluster.PackedPrimitiveVertexCount = packed.x;
  cluster.PackedLodSection = (packed.y & 0xffff0000u) | ((key >> 28u) & 7u);
  cluster.Padding = packed.zw;
  cluster.Bounds = asfloat(g_Buffers[descriptor].Load4(offset + 32u));
  cluster.Cone = asfloat(g_Buffers[descriptor].Load4(offset + 48u));
  cluster.Page = g_Buffers[descriptor].Load4(offset + 64u);
  cluster.Hierarchy = asfloat(g_Buffers[descriptor].Load4(offset + 80u));
  return true;
}

uint xiiGeometryClusterKey(uint4 mapping, uint localCluster, uint lodIndex)
{
  uint words = (mapping.z >> 16u) + (mapping.z & 0xffffu) + localCluster * 24u;
  return 0x80000000u | (lodIndex << 28u) | (mapping.x << 16u) | words;
}

uint4 xiiGeometryPageMapping(GpuGeometryLod lod, uint page)
{
  if (lod.VertexBufferIndex == 0xFFFFFFFFu)
    return uint4(0xFFFFFFFFu, 0u, 0u, 0u);
  uint bytes;
  g_Buffers[NonUniformResourceIndex(lod.VertexBufferIndex)].GetDimensions(bytes);
  if (bytes < 272u || page >= g_Buffers[NonUniformResourceIndex(lod.VertexBufferIndex)].Load(0u) || page >= (bytes - 272u) / 48u)
    return uint4(0xFFFFFFFFu, 0u, 0u, 0u);
  return g_Buffers[NonUniformResourceIndex(lod.VertexBufferIndex)].Load4(272u + page * 48u);
}

bool xiiResolveClusterLod(inout GpuGeometryLod lod, inout GpuMeshlet cluster)
{
  if (lod.Padding == 0u)
    return true;
  uint4 mapping = xiiGeometryPageMapping(lod, cluster.Page.x);
  if (mapping.x == 0xFFFFFFFFu)
    return false;
  uint baseWords = mapping.z >> 16u;
  lod.VertexBufferIndex          = mapping.x;
  lod.IndexBufferIndex           = baseWords * 4u; // Resolved virtual LODs store a vertex byte offset here.
  lod.MeshletTopologyBufferIndex = mapping.x;
  lod.MeshletPrimitiveByteOffset = 0u;
  lod.VertexCount                = cluster.Page.w;
  cluster.Header.y               = baseWords + cluster.Page.y / 4u;
  cluster.Header.z               = baseWords * 4u + cluster.Page.z;
  return true;
}

uint xiiGeometryVertexOffset(GpuGeometryLod lod)
{
  return lod.Padding != 0u ? lod.IndexBufferIndex : 0u;
}


struct xiiGeometryPageInfo
{
  uint4  Mapping;
  float4 Bounds;
  float4 Detail; // error, source first-cluster bits, first child plus one, child count
};
xiiGeometryPageInfo xiiLoadGeometryPage(GpuGeometryLod lod, uint page)
{
  xiiGeometryPageInfo result = (xiiGeometryPageInfo)0;
  result.Mapping             = xiiGeometryPageMapping(lod, page);
  if (result.Mapping.w != 0u)
  {
    result.Bounds = asfloat(g_Buffers[NonUniformResourceIndex(lod.VertexBufferIndex)].Load4(288u + page * 48u));
    result.Detail = asfloat(g_Buffers[NonUniformResourceIndex(lod.VertexBufferIndex)].Load4(304u + page * 48u));
  }
  return result;
}

// A root row contains the page, cumulative cluster count, primitive start,
// and prefix maximum primitive end. This bounds work to a draw's section.
bool xiiGeometryRootDirectory(GpuGeometryLod lod, out uint table, out uint offset, out uint roots)
{
  table = NonUniformResourceIndex(lod.VertexBufferIndex);
  offset = 0u;
  roots = 0u;
  if (lod.VertexBufferIndex == 0xffffffffu) return false;
  uint bytes;
  g_Buffers[table].GetDimensions(bytes);
  if (bytes < 272u) return false;
  uint pages = g_Buffers[table].Load(0u);
  if (pages > (bytes - 272u) / 48u) return false;
  offset = 272u + pages * 48u;
  roots = min(g_Buffers[table].Load(4u), (bytes - offset) / 16u);
  return true;
}

void xiiGeometryRootRange(uint table, uint offset, uint roots, GpuSceneInstance instance, out uint first, out uint end)
{
  first = 0u;
  end = roots;
  if (instance.PrimitiveCount == 0u) return;
  uint low = 0u, high = roots;
  while (low < high)
  {
    uint middle = low + (high - low) / 2u;
    if (g_Buffers[NonUniformResourceIndex(table)].Load(offset + middle * 16u + 12u) <= instance.FirstPrimitive) low = middle + 1u;
    else high = middle;
  }
  first = low;
  high = roots;
  while (low < high)
  {
    uint middle = low + (high - low) / 2u;
    uint start = g_Buffers[NonUniformResourceIndex(table)].Load(offset + middle * 16u + 8u);
    if (start < instance.FirstPrimitive || start - instance.FirstPrimitive < instance.PrimitiveCount) low = middle + 1u;
    else high = middle;
  }
  end = low;
}

void xiiGeometryRootClusterRange(GpuGeometryLod lod, GpuSceneInstance instance, out uint first, out uint count)
{
  first = 0u;
  count = 0u;
  uint table, offset, roots;
  if (!xiiGeometryRootDirectory(lod, table, offset, roots)) return;
  uint begin, end;
  xiiGeometryRootRange(table, offset, roots, instance, begin, end);
  if (begin == end) return;
  first = begin == 0u ? 0u : g_Buffers[NonUniformResourceIndex(table)].Load(offset + (begin - 1u) * 16u + 4u);
  count = g_Buffers[NonUniformResourceIndex(table)].Load(offset + (end - 1u) * 16u + 4u) - first;
}
bool xiiLoadGeometryRootCluster(GpuGeometryLod lod, uint lodIndex, uint ordinal, out GpuMeshlet cluster)
{
  cluster = (GpuMeshlet)0;
  uint table, offset, roots;
  if (!xiiGeometryRootDirectory(lod, table, offset, roots)) return false;
  table = NonUniformResourceIndex(table);
  uint low = 0u, high = roots;
  while (low < high)
  {
    uint middle = low + (high - low) / 2u;
    if (g_Buffers[table].Load(offset + middle * 16u + 4u) <= ordinal) low = middle + 1u;
    else high = middle;
  }
  if (low >= roots) return false;
  uint previous = low == 0u ? 0u : g_Buffers[table].Load(offset + (low - 1u) * 16u + 4u);
  xiiGeometryPageInfo page = xiiLoadGeometryPage(lod, g_Buffers[table].Load(offset + low * 16u));
  uint first = asuint(page.Detail.y);
  if (page.Mapping.x == 0xffffffffu || first < lod.IndexBufferIndex ||
      first - lod.IndexBufferIndex >= lod.MeshletCount || ordinal - previous >= page.Mapping.w) return false;
  return xiiLoadCluster(xiiGeometryClusterKey(page.Mapping, ordinal - previous, lodIndex), cluster);
}
float xiiGeometryMaximumScale(GpuSceneInstance instance)
{
  float3 a        = float3(instance.GlobalTransform[0][0], instance.GlobalTransform[1][0], instance.GlobalTransform[2][0]);
  float3 b        = float3(instance.GlobalTransform[0][1], instance.GlobalTransform[1][1], instance.GlobalTransform[2][1]);
  float3 c        = float3(instance.GlobalTransform[0][2], instance.GlobalTransform[1][2], instance.GlobalTransform[2][2]);
  float3 diagonal = float3(dot(a, a), dot(b, b), dot(c, c));
  float3 off      = abs(float3(dot(a, b), dot(a, c), dot(b, c)));
  float3 bound    = diagonal + float3(off.x + off.y, off.x + off.z, off.y + off.z);
  return sqrt(max(bound.x, max(bound.y, bound.z)));
}
bool xiiGeometryPageVisible(xiiGeometryPageInfo page, GpuSceneInstance instance, GpuVisibilityView view)
{
  float3 center = mul(instance.GlobalTransform, float4(page.Bounds.xyz, 1.0f)).xyz;
  return SphereInsideFrustum(center, page.Bounds.w * xiiGeometryMaximumScale(instance), view);
}
bool xiiGeometryPageRefines(GpuGeometryLod lod, xiiGeometryPageInfo page, GpuSceneInstance instance, GpuVisibilityView view)
{
  uint children = min((uint)page.Detail.w, 8u);
  if (children == 0u)
    return false;
  float3 center     = mul(instance.GlobalTransform, float4(page.Bounds.xyz, 1.0f)).xyz;
  float  scale      = xiiGeometryMaximumScale(instance);
  float  distance   = length(view.ViewProjectionMatrix[3].xyz) < 0.0001f ? 1.0f :
                                                                           max(0.001f, abs(mul(view.ViewProjectionMatrix, float4(center, 1.0f)).w) - page.Bounds.w * scale);
  float  error      = page.Detail.x * scale * view.GeometryPixelScale *
    exp2(clamp(instance.LodBias, -16.0f, 16.0f)) / distance;
  if (error <= 1.0f)
    return false;
  uint feedback = g_Buffers[NonUniformResourceIndex(lod.VertexBufferIndex)].Load(16u + (lod.Padding >> 1u) * 4u);
  bool complete = true;
  for (uint child = 0u; child < children; ++child)
  {
    xiiGeometryPageInfo childPage = xiiLoadGeometryPage(lod, (uint)page.Detail.z - 1u + child);
    if (xiiGeometryPageVisible(childPage, instance, view))
    {
      xiiRecordPageDemand(feedback, childPage.Mapping.y);
      complete = complete && childPage.Mapping.x != 0xffffffffu;
    }
  }
  return complete;
}
