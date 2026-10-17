/// Copyright (c) Theophilus Eriata. All Rights Reserved.

// XII Engine - canonical surface attributes and transforms for mesh and indexed rasterization.
#pragma once
#include <Shaders/Pipeline/GpuSceneSkinning.h>

GBufferVertex xiiInvalidSceneSurfaceVertex()
{
  GBufferVertex result = (GBufferVertex)0;
  result.Position      = float4(0, 0, -1, 1);
  return result;
}

GBufferVertex xiiLoadSceneSurfaceVertex(GpuSceneInstance instance, GpuGeometryLod lod, uint sourceVertex)
{
  uint vertexBufferBytes;
  g_Buffers[lod.VertexBufferIndex].GetDimensions(vertexBufferBytes);
  const uint lastAttributeEnd = max(max(12u, NormalOffset + 12u), max(TangentOffset + 16u, TexCoordOffset + 8u));
  if (VertexStride < lastAttributeEnd || sourceVertex >= lod.VertexCount ||
      vertexBufferBytes < lastAttributeEnd || sourceVertex > (vertexBufferBytes - lastAttributeEnd) / VertexStride)
    return xiiInvalidSceneSurfaceVertex();
  const uint   vertexOffset  = sourceVertex * VertexStride;
  float3       localPosition = asfloat(g_Buffers[lod.VertexBufferIndex].Load3(vertexOffset));
  float3       localNormal   = asfloat(g_Buffers[lod.VertexBufferIndex].Load3(vertexOffset + NormalOffset));
  float4       localTangent  = asfloat(g_Buffers[lod.VertexBufferIndex].Load4(vertexOffset + TangentOffset));
  const float2 texCoord      = asfloat(g_Buffers[lod.VertexBufferIndex].Load2(vertexOffset + TexCoordOffset));
  float3       previousLocalPosition;
#if defined(XII_SCENE_SURFACE_CURRENT_ONLY)
  // Surface queries do not consume velocity. Keep pose history out of their shader.
  previousLocalPosition = localPosition;
  if (!xiiSkinCurrentSurface(instance, lod, sourceVertex, localPosition, localNormal, localTangent))
#else
  if (!xiiSkinSurface(instance, lod, sourceVertex, localPosition, localNormal, localTangent, previousLocalPosition))
#endif
    return xiiInvalidSceneSurfaceVertex();

  const float3 worldPosition         = mul(instance.GlobalTransform, float4(localPosition, 1.0f)).xyz;
  const float3 previousWorldPosition = mul(instance.PreviousGlobalTransform, float4(previousLocalPosition, 1.0f)).xyz;
  float3       worldNormal           = normalize(float3(dot(instance.NormalTransformRow0.xyz, localNormal), dot(instance.NormalTransformRow1.xyz, localNormal), dot(instance.NormalTransformRow2.xyz, localNormal)));
  float3       worldTangent          = normalize(mul((float3x3)instance.GlobalTransform, localTangent.xyz));
  worldTangent                       = normalize(worldTangent - worldNormal * dot(worldTangent, worldNormal));

  GBufferVertex output;
  output.Position             = mul(ViewProjectionMatrix, float4(worldPosition, 1.0f));
#if !defined(XII_SCENE_DEPTH_ONLY)
  output.WorldNormal          = worldNormal;
#endif
#if !defined(XII_SCENE_SURFACE_QUERY) && !defined(XII_SCENE_DEPTH_ONLY)
  output.WorldTangent         = worldTangent;
  output.TangentSign          = localTangent.w;
#endif
  output.TexCoord             = texCoord;
  output.MaterialIndex        = instance.MaterialIndex;
#if !defined(XII_SCENE_SURFACE_QUERY) && !defined(XII_SCENE_DEPTH_ONLY)
  output.PreviousClipPosition = mul(PreviousViewProjectionMatrix, float4(previousWorldPosition, 1.0f));
#endif
#if !defined(XII_SCENE_DEPTH_ONLY)
  output.ObjectId             = instance.ObjectId;
#endif
  return output;
}
