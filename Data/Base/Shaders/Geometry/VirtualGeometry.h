/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <Shaders/Streaming/GpuResidencyFeedback.h>

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
  lod.VertexBufferIndex          = mapping.x;
  lod.MeshletTopologyBufferIndex = mapping.x;
  lod.MeshletPrimitiveByteOffset = 0u;
  lod.VertexCount                = cluster.Page.w;
  cluster.Header.y               = cluster.Page.y / 4u;
  cluster.Header.z               = cluster.Page.z;
  return true;
}


struct xiiGeometryPageInfo
{
  uint4  Mapping;
  float4 Bounds;
  float4 Detail; // error, parent plus one, first child plus one, child count
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
