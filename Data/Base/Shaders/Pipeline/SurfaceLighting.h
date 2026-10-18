/// Copyright (c) Theophilus Eriata. All Rights Reserved.

// Shared physically based surface-light evaluation for deferred and Forward+ shading.
// The including shader must include PipelineCommon.xiiShader first so camera and
// clustered-light constants, LinearizeDepth(), and the common samplers are available.
#pragma once

#include <Shaders/Pipeline/SurfaceLocalLight.h>

uint ComputeClusterIndex(float2 uv, float depth)
{
  if (g_ClusterCountX == 0u || g_ClusterCountY == 0u || g_ClusterCountZ == 0u)
    return 0u;

  const float linearDepth = LinearizeDepth(depth);
  const float zSlice      = max(0.0f, log2(max(linearDepth, g_ClusterNearPlane) / max(g_ClusterNearPlane, 1e-4f)) / max(g_ClusterLogFarOverNear, 1e-4f) * (float)g_ClusterCountZ);

  const uint cX = min((uint)(uv.x * (float)g_ClusterCountX), g_ClusterCountX - 1u);
  const uint cY = min((uint)(uv.y * (float)g_ClusterCountY), g_ClusterCountY - 1u);
  const uint cZ = min((uint)zSlice, g_ClusterCountZ - 1u);
  return (cZ * g_ClusterCountY + cY) * g_ClusterCountX + cX;
}
