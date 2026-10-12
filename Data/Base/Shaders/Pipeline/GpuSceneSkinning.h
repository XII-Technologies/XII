// XII Engine - native mesh-space palette evaluation shared by all GPU-scene vertex consumers.
#pragma once

StructuredBuffer<float4x4> g_SkinningMatrices;

uint xiiSkinningByte(ByteAddressBuffer source, uint offset)
{
  return (source.Load(offset & ~3u) >> ((offset & 3u) * 8u)) & 255u;
}

bool xiiSkinningMatrix(GpuSceneInstance instance, GpuGeometryLod lod, uint sourceVertex, bool previous, out float4x4 matrix)
{
  matrix = float4x4(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
  const uint count = previous ? instance.PreviousSkinningCount : instance.SkinningCount;
  if (count == 0u)
    return true;
  uint paletteCapacity, paletteStride, vertexBytes;
  g_SkinningMatrices.GetDimensions(paletteCapacity, paletteStride);
  g_Buffers[lod.VertexBufferIndex].GetDimensions(vertexBytes);
  const uint paletteOffset = previous ? instance.PreviousSkinningOffset : instance.SkinningOffset;
  const uint stride = instance.SkinningVertexStride;
  const uint end = max(instance.BoneIndicesOffset + 8u, instance.BoneWeightsOffset + 4u);
  if (paletteOffset > paletteCapacity || count > paletteCapacity - paletteOffset ||
      stride < end || end < 8u || vertexBytes < end || sourceVertex >= lod.VertexCount)
    return false;
  if (sourceVertex > (vertexBytes - end) / stride)
    return false;
  const uint vertexOffset = sourceVertex * stride;
  uint4 joints, weights;
  [unroll] for (uint i = 0u; i < 4u; ++i)
  {
    const uint indexOffset = vertexOffset + instance.BoneIndicesOffset + i * 2u;
    joints[i] = xiiSkinningByte(g_Buffers[lod.VertexBufferIndex], indexOffset) |
      (xiiSkinningByte(g_Buffers[lod.VertexBufferIndex], indexOffset + 1u) << 8u);
    weights[i] = xiiSkinningByte(g_Buffers[lod.VertexBufferIndex], vertexOffset + instance.BoneWeightsOffset + i);
  }
  const uint total = weights.x + weights.y + weights.z + weights.w;
  if (total == 0u)
    return true;
  matrix = (float4x4)0;
  [unroll] for (uint j = 0u; j < 4u; ++j)
  {
    if (weights[j] != 0u)
    {
      if (joints[j] >= count)
        return false;
      matrix += g_SkinningMatrices[paletteOffset + joints[j]] * (float(weights[j]) / float(total));
    }
  }
  return true;
}

bool xiiSkinPosition(GpuSceneInstance instance, GpuGeometryLod lod, uint sourceVertex, inout float3 position)
{
  if (instance.SkinningCount == 0u)
    return all(isfinite(position));
  float4x4 skin;
  if (!xiiSkinningMatrix(instance, lod, sourceVertex, false, skin))
    return false;
  position = mul(skin, float4(position, 1)).xyz;
  return all(isfinite(position));
}

float3 xiiSkinningNormalize(float3 direction, float3 fallback)
{
  const float magnitude = dot(direction, direction);
  return magnitude > 1.0e-20f && isfinite(magnitude) ? direction * rsqrt(magnitude) : fallback;
}

bool xiiSkinSurface(GpuSceneInstance instance, GpuGeometryLod lod, uint sourceVertex,
  inout float3 position, inout float3 normal, inout float4 tangent, out float3 previousPosition)
{
  float4x4 current, previous;
  previousPosition = position;
  if (instance.SkinningCount == 0u && instance.PreviousSkinningCount == 0u)
    return all(isfinite(position));
  if (!xiiSkinningMatrix(instance, lod, sourceVertex, false, current) ||
      !xiiSkinningMatrix(instance, lod, sourceVertex, true, previous))
    return false;
  previousPosition = mul(previous, float4(position, 1)).xyz;
  position = mul(current, float4(position, 1)).xyz;
  const float3x3 skinLinear = (float3x3)current;
  const float3 row0 = cross(skinLinear[1], skinLinear[2]);
  const float3 row1 = cross(skinLinear[2], skinLinear[0]);
  const float3 row2 = cross(skinLinear[0], skinLinear[1]);
  const float determinant = dot(skinLinear[0], row0);
  if (abs(determinant) > 1.0e-12f)
    normal = xiiSkinningNormalize(mul(float3x3(row0, row1, row2), normal) / determinant, normal);
  tangent.xyz = xiiSkinningNormalize(mul(skinLinear, tangent.xyz), tangent.xyz);
  tangent.xyz = xiiSkinningNormalize(tangent.xyz - normal * dot(tangent.xyz, normal), tangent.xyz);
  tangent.w *= determinant < 0.0f ? -1.0f : 1.0f;
  return all(isfinite(position)) && all(isfinite(previousPosition));
}
