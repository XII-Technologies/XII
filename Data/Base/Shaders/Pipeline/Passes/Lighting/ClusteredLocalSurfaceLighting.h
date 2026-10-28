/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/StorageImageFormat.h>
#include <Shaders/Pipeline/Passes/Lighting/LightClusterConstants.h>
#include <Shaders/Pipeline/Passes/Lighting/SurfaceInputs.h>
#define XII_BRDF_VIEW_MODE ((uint)SurfaceEvaluation.z)
#include <Shaders/Pipeline/SurfaceLocalLight.h>
StructuredBuffer<xiiGpuLightData> g_Lights;
StructuredBuffer<uint4>           g_Counts;
StructuredBuffer<uint>            g_Candidates;
StructuredBuffer<uint2>           g_LightGrid;
StructuredBuffer<uint>            g_LightIndices;
StructuredBuffer<uint>            g_Overflow;
XII_STORAGE_IMAGE_FORMAT("rgba16f")
RWTexture2D<float4> g_Lighting;
#if XII_SPOT_SHADOWS
#  include <Shaders/Pipeline/Passes/Shadows/SpotShadowSampling.h>
Texture2D<uint>         g_SpotObjectIds;
StructuredBuffer<uint2> g_SpotReceiverFlags;
XII_STORAGE_IMAGE_FORMAT("rgba16f")
RWTexture2D<float4> g_SpotReactive;

bool ReceivesSpotShadows(uint2 pixel)
{
  const uint identity = g_SpotObjectIds.Load(int3(pixel, 0));
  uint       count, stride;
  g_SpotReceiverFlags.GetDimensions(count, stride);
  uint low = 0, high = count;
  while (low < high)
  {
    const uint middle = low + (high - low) / 2;
    if (g_SpotReceiverFlags[middle].x < identity)
      low = middle + 1;
    else
      high = middle;
  }
  return low < count && g_SpotReceiverFlags[low].x == identity && (g_SpotReceiverFlags[low].y & (1u << 3u)) != 0;
}
#endif
float3 Evaluate(uint index, SurfaceSample surface, bool receives, inout float reactive)
{
  if ((uint)SurfaceEvaluation.z == ViewRenderMode_ClusterID || (uint)SurfaceEvaluation.z == ViewRenderMode_LightComplexity)
    return 0;
  uint count, stride;
  g_Lights.GetDimensions(count, stride);
  if (index >= min(count, g_Counts[0].x))
    return 0;
  const xiiGpuLightData light = g_Lights[index];
  const uint            type  = GetLightType(light);
  if (type == XII_LIGHT_TYPE_DIRECTIONAL || type > XII_LIGHT_TYPE_TUBE || light.ColorAndIntensity.w <= 0)
    return 0;
  float3 result = EvaluateLocalLightLayered(light, surface.Position, surface.Normal, surface.View, surface.Albedo, surface.Roughness, surface.Metallic, surface.Specular, surface.Transmission, 0, 0.1f);
#if XII_SPOT_SHADOWS
  if (receives && xiiHasSpotShadow(index))
  {
    // Mark potential incident light before visibility: a fully shadowed receiver
    // must also reject its history after this shadow-producing light disappears.
    if (any(result > 0))
      reactive = 1;
    result *= xiiSampleSpotShadow(index, surface.Position, surface.Normal, SurfaceCameraPositionNear.xyz);
  }
#endif
  return result;
}
[numthreads(8, 8, 1)] void main(uint3 tid : SV_DispatchThreadID) {
  if (any(tid.xy >= SurfaceExtent.xy))
    return;
  SurfaceSample surface;
  float3        result          = 0;
  float         reactive        = 0;
  uint          evaluatedLights = 0;
  uint          clusterIdentity = 0;
  if (LoadSurface(tid.xy, surface))
  {
    const float clusterDepth = dot(surface.Position - CameraPosition.xyz, CameraForward.xyz);
    if (clusterDepth >= DepthRange.x && clusterDepth <= DepthRange.y)
    {
      const float t     = DepthRange.w > 0 ? log2(max(clusterDepth, DepthRange.x) / DepthRange.x) / DepthRange.z : (clusterDepth - DepthRange.x) / (DepthRange.y - DepthRange.x);
      const uint  slice = min((uint)(saturate(t) * ClusterCounts.z), ClusterCounts.z - 1);
      const uint2 cell  = min(tid.xy / Viewport.z, ClusterCounts.xy - 1);
      clusterIdentity   = (slice * ClusterCounts.y + cell.y) * ClusterCounts.x + cell.x + 1u;
    }
    bool receives = true;
#if XII_SPOT_SHADOWS
    receives = ReceivesSpotShadows(tid.xy);
#endif
    if (g_Overflow[0] != 0)
    {
      // Capacity overflow preserves semantics by evaluating this view's full candidate set.
      uint count, stride;
      g_Candidates.GetDimensions(count, stride);
      evaluatedLights = count;
      [loop] for (uint i = 0; i < count; ++i) result += Evaluate(g_Candidates[i], surface, receives, reactive);
    }
    else
    {
      if (clusterIdentity != 0u)
      {
        const uint cluster = clusterIdentity - 1u;
        uint       gridCount, stride;
        g_LightGrid.GetDimensions(gridCount, stride);
        if (cluster < gridCount)
        {
          const uint2 range = g_LightGrid[cluster];
          uint        capacity;
          g_LightIndices.GetDimensions(capacity, stride);
          evaluatedLights = range.x < capacity ? min(min(range.y, Viewport.w), capacity - range.x) : 0u;
          [loop] for (uint i = 0; i < min(range.y, Viewport.w); ++i) if (range.x < capacity && i < capacity - range.x) result += Evaluate(g_LightIndices[range.x + i], surface, receives, reactive);
        }
      }
    }
  }
  result *= SurfaceEvaluation.x;
  if ((uint)SurfaceEvaluation.z == ViewRenderMode_ClusterID)
    result = ViewIdentityColor(clusterIdentity);
  if ((uint)SurfaceEvaluation.z == ViewRenderMode_LightComplexity)
    result = evaluatedLights == 0u ? 0.0f : ViewHeatColor(saturate(log2(float(evaluatedLights) + 1.0f) / 5.0f));
  g_Lighting[tid.xy] = float4(all(isfinite(result)) ? clamp(result, 0, 65504.0f) : 0, 1);
#if XII_SPOT_SHADOWS
  g_SpotReactive[tid.xy] = float4(reactive.xxx, 0);
#endif
}
