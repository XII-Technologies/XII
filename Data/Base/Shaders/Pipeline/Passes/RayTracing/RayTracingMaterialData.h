/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Canonical surface record used by ray-tracing hit shaders.
///
/// Authored material schemas remain free to arrange their own parameter blocks. The ray-tracing
/// scene resolves the well-known PBR properties into this stable representation so hit shaders do
/// not depend on a particular schema layout or frame-sliced material-buffer offset.
struct XII_SHADER_STRUCT xiiRayTracingMaterialData
{
  FLOAT4(BaseColorOpacity);             ///< xyz = linear base color, w = opacity.
  FLOAT4(EmissiveColorAndRoughness);    ///< xyz = emitted radiance in nits, w = perceptual roughness.
  FLOAT4(SurfaceParameters);            ///< x = metallic, y = dielectric specular, z = transmission, w = occlusion.
  UINT4(Metadata);                      ///< x = material slot, y = stable object ID, z = shading model, w = feature flags.
  UINT4(TextureIndices0);               ///< Base color, normal, metallic-roughness and occlusion bindless SRVs.
  UINT4(TextureIndices1);               ///< Emissive, height, clear-coat and transmission bindless SRVs.
};

/// Geometry addressing record parallel to xiiRayTracingMaterialData and indexed by
/// HLSL InstanceIndex(). Buffer indices address the engine-wide bindless buffer SRV table.
struct XII_SHADER_STRUCT xiiRayTracingGeometryData
{
  UINT4(BufferIndices); ///< x = vertex SRV, y = index SRV or invalid, z = index stride, w = normal stride.
  UINT4(VertexLayout);  ///< x = vertex stride, y = position offset, z = normal offset or invalid, w = UV0 offset or invalid.
  UINT4(VertexAttributes); ///< x = tangent offset or invalid, y = tangent stride, z = UV0 stride, w = reserved.
};

#if XII_ENABLED(XII_SHADER_PLATFORM)
uint xiiLoadRayTracingIndex(ByteAddressBuffer indexBuffer, uint index, uint indexStride)
{
  const uint byteOffset = index * indexStride;
  if (indexStride == 4U)
    return indexBuffer.Load(byteOffset);

  const uint packedIndices = indexBuffer.Load(byteOffset & ~3U);
  return (packedIndices >> ((byteOffset & 2U) << 3U)) & 0xFFFFU;
}

float3 xiiLoadRayTracingPosition(ByteAddressBuffer vertexBuffer, xiiRayTracingGeometryData geometry, uint vertexIndex)
{
  return asfloat(vertexBuffer.Load3(vertexIndex * geometry.VertexLayout.x + geometry.VertexLayout.y));
}

float3 xiiLoadRayTracingNormal(ByteAddressBuffer vertexBuffer, xiiRayTracingGeometryData geometry, uint vertexIndex)
{
  return asfloat(vertexBuffer.Load3(vertexIndex * geometry.BufferIndices.w + geometry.VertexLayout.z));
}

float4 xiiLoadRayTracingTangent(ByteAddressBuffer vertexBuffer, xiiRayTracingGeometryData geometry, uint vertexIndex)
{
  return asfloat(vertexBuffer.Load4(vertexIndex * geometry.VertexAttributes.y + geometry.VertexAttributes.x));
}

float2 xiiLoadRayTracingTexCoord(ByteAddressBuffer vertexBuffer, xiiRayTracingGeometryData geometry, uint vertexIndex)
{
  return asfloat(vertexBuffer.Load2(vertexIndex * geometry.VertexAttributes.z + geometry.VertexLayout.w));
}

float3 xiiDecodeRayTracingNormal(float4 normalSample)
{
  const float2 xy = normalSample.xy * 2.0f - 1.0f;
  return float3(xy, sqrt(max(1.0f - dot(xy, xy), 0.0f)));
}
#else
static_assert(sizeof(xiiRayTracingMaterialData) == 96U, "Ray-tracing material records must match the HLSL structured-buffer stride.");
static_assert(alignof(xiiRayTracingMaterialData) == 16U, "Ray-tracing material records require aligned CPU storage.");
static_assert(sizeof(xiiRayTracingGeometryData) == 48U, "Ray-tracing geometry records must match the HLSL structured-buffer stride.");
static_assert(alignof(xiiRayTracingGeometryData) == 16U, "Ray-tracing geometry records require aligned CPU storage.");
#endif
