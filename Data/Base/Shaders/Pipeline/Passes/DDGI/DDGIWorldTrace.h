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
SamplerState                            g_ProbeSampler;

struct ProbeHit
{
  float  Distance;
  float3 Emissive;
  bool   Found;
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
  return textureIndex == 0xffffffffu ? 1.0f : xiiSampleSurfaceTextureLevel(g_Textures[NonUniformResourceIndex(textureIndex)], samplerIndex, g_ProbeSampler, uv, 0);
}

// Return false when the scene cannot be covered within the bounded software budget.
bool TraceProbeScene(float3 origin, float3 direction, float maximum, out ProbeHit hit)
{
  hit.Distance = maximum;
  hit.Emissive = 0;
  hit.Found    = false;
  uint count, stride, geometryCount, materialCount;
  g_ProbeCandidates.GetDimensions(count, stride);
  g_ProbeGeometry.GetDimensions(geometryCount, stride);
  g_ProbeMaterials.GetDimensions(materialCount, stride);
  uint remaining = 2048u;
  [loop] for (uint i = 0u; i < count; ++i)
  {
    const uint             candidate = g_ProbeCandidates[i];
    const GpuSceneInstance instance  = g_ProbeInstances[candidate & 0x7fffffffu];
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
    if (material.Metadata.z >= 2u)
      continue;
    const bool indexed   = lod.IndexBufferIndex != 0xffffffffu;
    const uint available = (indexed ? lod.IndexCount : lod.VertexCount) / 3u;
    const uint begin     = min(instance.FirstPrimitive, available);
    const uint triangles = instance.PrimitiveCount == 0u ? available - begin : min(instance.PrimitiveCount, available - begin);
    if (triangles > remaining)
      return false;
    remaining -= triangles;
    const uint vertexBuffer = NonUniformResourceIndex(lod.VertexBufferIndex);
    const uint indexBuffer  = NonUniformResourceIndex(lod.IndexBufferIndex);
    // xiiGALValueType::UInt16. The other native index type is UInt32.
    const bool shortIndex = lod.IndexType == 5u;
    [loop] for (uint primitive = begin; primitive < begin + triangles; ++primitive)
    {
      uint3 vertices = primitive * 3u + uint3(0u, 1u, 2u);
      if (indexed)
        vertices = uint3(LoadProbeIndex(g_Buffers[indexBuffer], vertices.x, shortIndex),
                         LoadProbeIndex(g_Buffers[indexBuffer], vertices.y, shortIndex),
                         LoadProbeIndex(g_Buffers[indexBuffer], vertices.z, shortIndex));
      if (any(vertices >= lod.VertexCount))
        continue;
      const uint3  offsets = vertices * ProbeSceneLayout.z;
      const float3 a       = mul(instance.GlobalTransform, float4(asfloat(g_Buffers[vertexBuffer].Load3(offsets.x)), 1.0f)).xyz;
      const float3 b       = mul(instance.GlobalTransform, float4(asfloat(g_Buffers[vertexBuffer].Load3(offsets.y)), 1.0f)).xyz;
      const float3 c       = mul(instance.GlobalTransform, float4(asfloat(g_Buffers[vertexBuffer].Load3(offsets.z)), 1.0f)).xyz;
      float3       barycentrics;
      float        distance;
      bool         front;
      if (!IntersectProbeTriangle(origin, direction, a, b, c, hit.Distance, barycentrics, distance, front))
        continue;
      const uint3  uvOffsets = offsets + ProbeSceneLayout.w;
      const float2 uv        = asfloat(g_Buffers[vertexBuffer].Load2(uvOffsets.x)) * barycentrics.x +
        asfloat(g_Buffers[vertexBuffer].Load2(uvOffsets.y)) * barycentrics.y +
        asfloat(g_Buffers[vertexBuffer].Load2(uvOffsets.z)) * barycentrics.z;
      if (material.Metadata.z == 1u &&
          material.BaseColorOpacity.a * ProbeTexture(material.TextureIndices0.x, material.SamplerIndices0.x, uv).a < material.LayerParameters.x)
        continue;
      hit.Distance = distance;
      hit.Found    = true;
      hit.Emissive = 0.0f;
      if (front || (material.Metadata.y & (1u << 10u)) != 0u)
        hit.Emissive = material.EmissiveColorAndRoughness.rgb * ProbeTexture(material.TextureIndices1.x, material.SamplerIndices1.x, uv).rgb;
    }
  }
  return true;
}
