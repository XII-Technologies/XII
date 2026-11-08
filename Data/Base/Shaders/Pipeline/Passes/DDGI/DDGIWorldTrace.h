/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Materials/GpuSurfaceMaterial.h>
#include <Shaders/Materials/GpuSurfaceSampling.h>
#include <Shaders/Pipeline/Passes/DDGI/DDGIWorldTraceConstants.h>
#include <Shaders/Visibility/GpuSceneCommon.h>

StructuredBuffer<uint>                  g_ProbeCandidates;
StructuredBuffer<GpuSceneInstance>      g_ProbeInstances;
StructuredBuffer<GpuGeometryRecord>     g_ProbeGeometry;
StructuredBuffer<xiiGpuSurfaceMaterial> g_ProbeMaterials;
ByteAddressBuffer                       g_Buffers[];
Texture2D<float4>                       g_Textures[];
#include <Shaders/Geometry/VirtualGeometry.h>
SamplerState                            g_ProbeSampler;

#if defined(XII_DDGI_MESHLET_BOUNDS)
StructuredBuffer<GpuMeshlet> g_Meshlets;
#  include <Shaders/Visibility/MeshletRange.h>
#endif


struct ProbeHit
{
  float  Distance;
  float3 Emissive;
  bool   Found;
#if defined(XII_DDGI_ANALYTIC_LIGHTING)
  float3 Normal;
  float3 Albedo;
  float4 Material;
  float  Occlusion;
  bool   ReceivesShadows;
#endif
};

bool IntersectsProbeBounds(float3 origin, float3 direction, float maximum, float3 center, float3 extents)
{
  // Treat parallel rays conservatively. The expanded box also covers flat meshes.
  extents += 0.001f;
  const float3 signDirection = lerp(-1.0f, 1.0f, step(0.0f, direction));
  const float3 inverse       = signDirection / max(abs(direction), 1e-8f);
  const float3 a             = (center - extents - origin) * inverse;
  const float3 b             = (center + extents - origin) * inverse;
  const float3 near          = min(a, b);
  const float3 far           = max(a, b);
  return max(max(near.x, near.y), max(near.z, 0.0f)) <= min(min(far.x, far.y), min(far.z, maximum));
}

uint LoadProbeIndex(ByteAddressBuffer indices, uint index, bool shortIndex)
{
  if (!shortIndex)
    return indices.Load(index * 4u);
  return (indices.Load((index & ~1u) * 2u) >> ((index & 1u) * 16u)) & 0xffffu;
}

bool IntersectProbeTriangle(float3 origin, float3 direction, float3 a, float3 b, float3 c, float maximum, out float3 barycentrics, out float distance, out bool front)
{
  const float3 edge1         = b - a;
  const float3 edge2         = c - a;
  const float3 perpendicular = cross(direction, edge2);
  const float  determinant   = dot(edge1, perpendicular);
  barycentrics               = 0;
  distance                   = maximum;
  front                      = determinant > 0.0f;
  if (abs(determinant) < 1e-8f)
    return false;
  const float3 offset = origin - a;
  const float3 other  = cross(offset, edge1);
  const float2 uv     = float2(dot(offset, perpendicular), dot(direction, other)) / determinant;
  distance            = dot(edge2, other) / determinant;
  barycentrics        = float3(1.0f - uv.x - uv.y, uv);
  return all(barycentrics >= 0.0f) && distance > 0.015f && distance < maximum;
}

float4 ProbeTexture(uint textureIndex, uint samplerIndex, float2 uv)
{
  return textureIndex == 0xffffffffu ? 1.0f : xiiSampleSurfaceTextureLevel(textureIndex, samplerIndex, g_ProbeSampler, uv, 0);
}

void TraceProbePrimitive(float3 origin, float3 direction, float4x4 transform, uint vertexBufferIndex, uint vertexCount,
                         xiiGpuSurfaceMaterial material, uint3 vertices, uint instanceFlags, bool shadowQuery, inout ProbeHit hit)
{
  if (any(vertices >= vertexCount))
    return;
  const uint   vertexBuffer = NonUniformResourceIndex(vertexBufferIndex);
  const uint3  offsets      = vertices * ProbeSceneLayout.z;
  const float3 a            = mul(transform, float4(asfloat(g_Buffers[vertexBuffer].Load3(offsets.x)), 1.0f)).xyz;
  const float3 b            = mul(transform, float4(asfloat(g_Buffers[vertexBuffer].Load3(offsets.y)), 1.0f)).xyz;
  const float3 c            = mul(transform, float4(asfloat(g_Buffers[vertexBuffer].Load3(offsets.z)), 1.0f)).xyz;
  float3       barycentrics;
  float        distance;
  bool         front;
  if (!IntersectProbeTriangle(origin, direction, a, b, c, hit.Distance, barycentrics, distance, front))
    return;
  const uint3  uvOffsets = offsets + ProbeSceneLayout.w;
  const float2 uv        = asfloat(g_Buffers[vertexBuffer].Load2(uvOffsets.x)) * barycentrics.x +
    asfloat(g_Buffers[vertexBuffer].Load2(uvOffsets.y)) * barycentrics.y +
    asfloat(g_Buffers[vertexBuffer].Load2(uvOffsets.z)) * barycentrics.z;
  if (material.Metadata.z == 1u &&
      material.BaseColorOpacity.a * ProbeTexture(material.TextureIndices0.x, material.SamplerIndices0.x, uv).a < material.LayerParameters.x)
    return;
  hit.Distance = distance;
  hit.Found    = true;
  if (shadowQuery)
    return;
  hit.Emissive      = 0;
  const bool facing = front || (material.Metadata.y & (1u << 10u)) != 0u;
  if (facing)
    hit.Emissive = material.EmissiveColorAndRoughness.rgb * ProbeTexture(material.TextureIndices1.x, material.SamplerIndices1.x, uv).rgb;
#if defined(XII_DDGI_ANALYTIC_LIGHTING)
  hit.Normal = 0;
  if (facing && material.Metadata.x != 6u && (material.Metadata.y & (1u << 12u)) != 0u)
  {
    hit.Normal                     = normalize(cross(b - a, c - a)) * (front ? 1.0f : -1.0f);
    hit.Albedo                     = saturate(material.BaseColorOpacity.rgb * ProbeTexture(material.TextureIndices0.x, material.SamplerIndices0.x, uv).rgb);
    const float4 metallicRoughness = ProbeTexture(material.TextureIndices0.z, material.SamplerIndices0.z, uv);
    const float  transmission      = material.SurfaceParameters.z * ProbeTexture(material.TextureIndices1.w, material.SamplerIndices1.w, uv).r;
    hit.Material                   = saturate(float4(material.EmissiveColorAndRoughness.w * metallicRoughness.g, material.SurfaceParameters.x * metallicRoughness.b, material.SurfaceParameters.y, transmission));
    hit.Occlusion                  = lerp(1.0f, saturate(ProbeTexture(material.TextureIndices0.w, material.SamplerIndices0.w, uv).r), saturate(material.SurfaceParameters.w));
    hit.ReceivesShadows            = (instanceFlags & (1u << 3u)) != 0u;
  }
#endif
}

#if defined(XII_DDGI_MESHLET_BOUNDS)
uint ProbeMeshletByte(ByteAddressBuffer stream, uint offset)
{
  return (stream.Load(offset & ~3u) >> ((offset & 3u) * 8u)) & 0xffu;
}

// Validate that a single meshlet section covers the complete draw before using
// its reordered primitives. A partial span cannot be mixed with raw indices.
bool TraceProbeMeshlets(float3 origin, float3 direction, GpuSceneInstance instance, GpuGeometryLod lod, uint lodIndex,
                        xiiGpuSurfaceMaterial material, uint begin, uint end, inout uint remainingMeshlets,
                        inout uint remainingTriangles, bool shadowQuery, inout ProbeHit hit, out bool complete)
{
  complete = false;
  if (lod.Padding != 0u)
  {
    uint first, count;
    GetCandidateMeshletRange(instance, lod, first, count);
    if (count > remainingMeshlets) return false;
    remainingMeshlets -= count;
    [loop] for (uint n = 0u; n < count; ++n)
    {
      GpuMeshlet cluster = g_Meshlets[lod.MeshletMetadataOffset + first + n];
      if (cluster.Hierarchy.z == 0.0f) continue;
      GpuGeometryLod resident = lod;
      if (!xiiResolveClusterLod(resident, cluster)) return false;
      uint triangles = cluster.PackedPrimitiveVertexCount & 0xffffu;
      if (triangles > remainingTriangles) return false;
      remainingTriangles -= triangles;
      [loop] for (uint p = 0u; p < triangles; ++p)
      {
        uint offset = cluster.Header.z + p * 3u;
        uint3 local = uint3(ProbeMeshletByte(g_Buffers[resident.MeshletTopologyBufferIndex], offset),
          ProbeMeshletByte(g_Buffers[resident.MeshletTopologyBufferIndex], offset + 1u),
          ProbeMeshletByte(g_Buffers[resident.MeshletTopologyBufferIndex], offset + 2u));
        uint3 indices = uint3(g_Buffers[resident.MeshletTopologyBufferIndex].Load((cluster.Header.y + local.x) * 4u),
          g_Buffers[resident.MeshletTopologyBufferIndex].Load((cluster.Header.y + local.y) * 4u),
          g_Buffers[resident.MeshletTopologyBufferIndex].Load((cluster.Header.y + local.z) * 4u));
        TraceProbePrimitive(origin, direction, instance.GlobalTransform, resident.VertexBufferIndex, resident.VertexCount, material, indices, instance.Flags, shadowQuery, hit);
        if (shadowQuery && hit.Found) { complete = true; return true; }
      }
    }
    complete = true;
    return true;
  }
  if (lod.MeshletRangesOrdered == 0u || lod.MeshletTopologyBufferIndex == 0xffffffffu)
    return false;
  uint first, count;
  GetCandidateMeshletRange(instance, lod, first, count);
  if (count == 0u || count > remainingMeshlets)
    return false;
  remainingMeshlets -= count;
  first += lod.MeshletMetadataOffset;

  const uint topologyBuffer = NonUniformResourceIndex(lod.MeshletTopologyBufferIndex);
  const uint remapBytes     = lod.MeshletPrimitiveByteOffset;
  uint       topologyBytes;
  g_Buffers[topologyBuffer].GetDimensions(topologyBytes);
  if (remapBytes > topologyBytes)
    return false;
  const uint primitiveBytes = topologyBytes - remapBytes;
  const uint section        = g_Meshlets[first].PackedLodSection >> 16u;
  uint       cursor         = begin;
  [loop] for (uint i = first; i < first + count; ++i)
  {
    const GpuMeshlet meshlet   = g_Meshlets[i];
    const uint       triangles = meshlet.PackedPrimitiveVertexCount & 0xffffu;
    const uint       vertices  = meshlet.PackedPrimitiveVertexCount >> 16u;
    if (meshlet.Header.x != cursor || triangles == 0u || triangles > end - cursor ||
        (meshlet.PackedLodSection & 0xffffu) != lodIndex || (meshlet.PackedLodSection >> 16u) != section ||
        meshlet.Header.y >= remapBytes / 4u || vertices == 0u || vertices > remapBytes / 4u - meshlet.Header.y ||
        meshlet.Header.z >= primitiveBytes || triangles * 3u > primitiveBytes - meshlet.Header.z)
      return false;
    cursor += triangles;
  }
  if (cursor != end)
    return false;

  // The AABB of a transformed sphere uses the lengths of the linear transform rows.
  const float3 sphereScale = float3(length(instance.GlobalTransform[0].xyz),
                                    length(instance.GlobalTransform[1].xyz),
                                    length(instance.GlobalTransform[2].xyz));
  [loop] for (uint i = first; i < first + count; ++i)
  {
    const GpuMeshlet meshlet = g_Meshlets[i];
    const float3     center  = mul(instance.GlobalTransform, float4(meshlet.Bounds.xyz, 1.0f)).xyz;
    const float3     extents = sphereScale * meshlet.Bounds.w;
    if (meshlet.Bounds.w >= 0.0f && all(isfinite(center)) && all(isfinite(extents)) &&
        !IntersectsProbeBounds(origin, direction, hit.Distance, center, extents))
      continue;

    const uint triangles = meshlet.PackedPrimitiveVertexCount & 0xffffu;
    if (triangles > remainingTriangles)
      return true;
    remainingTriangles -= triangles;
    const uint vertices = meshlet.PackedPrimitiveVertexCount >> 16u;
    [loop] for (uint primitive = 0u; primitive < triangles; ++primitive)
    {
      const uint  offset = meshlet.Header.z + primitive * 3u;
      const uint3 local  = uint3(ProbeMeshletByte(g_Buffers[topologyBuffer], remapBytes + offset),
                                 ProbeMeshletByte(g_Buffers[topologyBuffer], remapBytes + offset + 1u),
                                 ProbeMeshletByte(g_Buffers[topologyBuffer], remapBytes + offset + 2u));
      if (any(local >= vertices))
        return true;
      const uint3 remap  = (meshlet.Header.y + local) * 4u;
      const uint3 source = uint3(g_Buffers[topologyBuffer].Load(remap.x), g_Buffers[topologyBuffer].Load(remap.y), g_Buffers[topologyBuffer].Load(remap.z));
      TraceProbePrimitive(origin, direction, instance.GlobalTransform, lod.VertexBufferIndex, lod.VertexCount, material, source, instance.Flags, shadowQuery, hit);
      if (shadowQuery && hit.Found)
      {
        complete = true;
        return true;
      }
    }
  }
  complete = true;
  return true;
}
#endif

// Return false when the scene cannot be covered within the bounded software budget.
bool TraceProbeScene(float3 origin, float3 direction, float maximum, out ProbeHit hit, bool shadowQuery = false)
{
  hit          = (ProbeHit)0;
  hit.Distance = maximum;
  uint count, stride, geometryCount, materialCount;
  g_ProbeCandidates.GetDimensions(count, stride);
  g_ProbeGeometry.GetDimensions(geometryCount, stride);
  g_ProbeMaterials.GetDimensions(materialCount, stride);
  uint remaining = 2048u;
#if defined(XII_DDGI_MESHLET_BOUNDS)
  uint remainingMeshlets = 4096u;
#endif

  [loop] for (uint i = 0u; i < count; ++i)
  {
    const uint             candidate = g_ProbeCandidates[i];
    const GpuSceneInstance instance  = g_ProbeInstances[candidate & 0x7fffffffu];
    if (shadowQuery && (instance.Flags & (1u << 2u)) == 0u)
      continue;
    if (!IntersectsProbeBounds(origin, direction, hit.Distance, instance.BoundsCenterRadius.xyz, instance.BoundsExtents.xyz))
      continue;
    if ((candidate & 0x80000000u) != 0u)
      return false;
    const uint geometryIndex = ProbeSceneLayout.x + instance.GeometryIndex;
    const uint materialIndex = ProbeSceneLayout.y + instance.MaterialIndex;
    if (instance.GeometryIndex == 0xffffffffu || geometryIndex >= geometryCount || instance.MaterialIndex == 0xffffffffu || materialIndex >= materialCount)
      return false;
    const GpuGeometryRecord geometry = g_ProbeGeometry[geometryIndex];
    const uint              lodIndex = instance.ForcedLod != 0xffffffffu ? instance.ForcedLod : 0u;
    if (lodIndex >= min(geometry.LodCount, 8u) || (geometry.ResidentLodMask & (1u << lodIndex)) == 0u)
      return false;
    const GpuGeometryLod lod = geometry.Lods[lodIndex];
    if (lod.VertexBufferIndex == 0xffffffffu)
      return false;
    const xiiGpuSurfaceMaterial material = g_ProbeMaterials[materialIndex];
    if (material.Metadata.z >= 2u || (shadowQuery && (material.Metadata.y & (1u << 13u)) == 0u))
      continue;
    const bool indexed      = lod.IndexBufferIndex != 0xffffffffu;
    const uint available    = (indexed ? lod.IndexCount : lod.VertexCount) / 3u;
    const uint begin        = min(instance.FirstPrimitive, available);
    const uint triangles    = instance.PrimitiveCount == 0u ? available - begin : min(instance.PrimitiveCount, available - begin);
    const uint vertexBuffer = NonUniformResourceIndex(lod.VertexBufferIndex);
    const uint indexBuffer  = NonUniformResourceIndex(lod.IndexBufferIndex);
    uint       vertexBytes;
    g_Buffers[vertexBuffer].GetDimensions(vertexBytes);
    if (vertexBytes / ProbeSceneLayout.z != lod.VertexCount)
      return false;
    // Native index types are xiiGALValueType::UInt16 and UInt32.
    const bool shortIndex = lod.IndexType == 5u;
    if (indexed)
    {
      uint indexBytes;
      g_Buffers[indexBuffer].GetDimensions(indexBytes);
      if ((!shortIndex && lod.IndexType != 6u) || lod.IndexCount > indexBytes / (shortIndex ? 2u : 4u))
        return false;
    }
#if defined(XII_DDGI_MESHLET_BOUNDS)
    bool complete;
    if (TraceProbeMeshlets(origin, direction, instance, lod, lodIndex, material, begin, begin + triangles, remainingMeshlets, remaining, shadowQuery, hit, complete))
    {
      if (!complete)
        return false;
      if (shadowQuery && hit.Found)
        return true;
      continue;
    }
#endif
    if (triangles > remaining)
      return false;
    remaining -= triangles;
    [loop] for (uint primitive = begin; primitive < begin + triangles; ++primitive)
    {
      uint3 vertices = primitive * 3u + uint3(0u, 1u, 2u);
      if (indexed)
        vertices = uint3(LoadProbeIndex(g_Buffers[indexBuffer], vertices.x, shortIndex),
                         LoadProbeIndex(g_Buffers[indexBuffer], vertices.y, shortIndex),
                         LoadProbeIndex(g_Buffers[indexBuffer], vertices.z, shortIndex));
      TraceProbePrimitive(origin, direction, instance.GlobalTransform, lod.VertexBufferIndex, lod.VertexCount, material, vertices, instance.Flags, shadowQuery, hit);
      if (shadowQuery && hit.Found)
        return true;
    }
  }
  return true;
}
