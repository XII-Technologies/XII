/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

struct GpuSceneInstance
{
  float4x4 GlobalTransform;
  float4x4 PreviousGlobalTransform;
  float4   NormalTransformRow0;
  float4   NormalTransformRow1;
  float4   NormalTransformRow2;
  float4   BoundsCenterRadius;
  float4   BoundsExtents;
  uint     GeometryIndex;
  uint     MaterialIndex;
  uint     ObjectId;
  uint     Flags;
  uint     VisibilityMask;
  uint     UserData;
  uint     FirstPrimitive;
  uint     PrimitiveCount;
};

struct GpuVisibilityView
{
  float4x4 ViewProjectionMatrix;
  float4x4 OcclusionViewProjectionMatrix;
  float4   FrustumPlanes[6];
  float4   CameraPosition;
  float4   ViewportAndHiZ;
  float4   LodParameters;
  uint     InstanceCount;
  uint     VisibilityMask;
  uint     RequiredFlags;
  uint     ExcludedFlags;
  uint     GeometryBaseIndex;
  uint     MaxVisibleMeshlets;
  uint2    Padding;
};

struct GpuGeometryLod
{
  uint  VertexBufferIndex;
  uint  IndexBufferIndex;
  uint  MeshletBufferIndex;
  uint  MeshletRemapBufferIndex;
  uint  MeshletPrimitiveBufferIndex;
  uint  VertexCount;
  uint  IndexCount;
  uint  MeshletCount;
  float MinimumScreenCoverage;
  uint  IndexType;
  uint  MeshletMetadataOffset;
  uint  Padding;
};

struct GpuGeometryRecord
{
  float4         BoundsCenterRadius;
  float4         BoundsExtents;
  GpuGeometryLod Lods[8];
  uint           LodCount;
  uint           Generation;
  uint           ResidentLodMask;
  uint           Flags;
};

struct GpuMeshlet
{
  uint4  Header;
  uint   PackedPrimitiveVertexCount;
  uint   PackedLodSection;
  uint2  Padding;
  float4 Bounds;
  float4 Cone;
};

struct GpuLodFeedback
{
  uint Key;
  uint GeometryGeneration;
  uint DesiredLod;
  uint SelectedLod;
  uint PurposeMask;
  uint ObjectUserData;
  uint Priority;
  uint Padding;
};

uint SelectDesiredLod(GpuGeometryRecord geometry, float worldSpaceRadius, float distanceToCamera, float viewportHeight)
{
  if (geometry.LodCount == 0u)
    return 0xFFFFFFFFu;

  const float projectedCoverage = worldSpaceRadius * viewportHeight / max(distanceToCamera, 0.001f);
  const uint  lodCount          = min(geometry.LodCount, 8u);
  [unroll] for (uint lod = 0u; lod < lodCount; ++lod)
  {
    if (projectedCoverage >= geometry.Lods[lod].MinimumScreenCoverage)
      return lod;
  }
  return lodCount - 1u;
}

uint SelectResidentLod(GpuGeometryRecord geometry, float worldSpaceRadius, float distanceToCamera, float viewportHeight)
{
  float projectedCoverage = worldSpaceRadius * viewportHeight / max(distanceToCamera, 0.001f);
  uint  fallback          = 0xFFFFFFFFu;
  [unroll] for (uint lod = 0u; lod < geometry.LodCount; ++lod)
  {
    if ((geometry.ResidentLodMask & (1u << lod)) == 0u)
      continue;
    fallback = lod;
    if (projectedCoverage >= geometry.Lods[lod].MinimumScreenCoverage)
      return lod;
  }
  return fallback;
}

uint SelectResidentLodWithHistory(GpuGeometryRecord geometry, float worldSpaceRadius, float distanceToCamera, float viewportHeight,
                                  uint instanceUserData, uint4 previousHistory, uint previousHistoryToken, float hysteresis)
{
  const uint desiredLod = SelectResidentLod(geometry, worldSpaceRadius, distanceToCamera, viewportHeight);
  if (desiredLod == 0xFFFFFFFFu)
    return desiredLod;

  const uint previousLod    = previousHistory.x;
  const bool historyMatches = previousHistory.w == previousHistoryToken &&
    previousHistory.y == geometry.Generation &&
    previousHistory.z == instanceUserData &&
    previousLod < min(geometry.LodCount, 8u) &&
    (geometry.ResidentLodMask & (1u << previousLod)) != 0u;
  if (!historyMatches || previousLod == desiredLod)
    return desiredLod;

  const float projectedCoverage = worldSpaceRadius * viewportHeight / max(distanceToCamera, 0.001f);
  const float deadBand          = clamp(hysteresis, 0.0f, 0.49f);
  if (desiredLod < previousLod)
  {
    // Promote only after crossing above the finer LOD threshold plus a dead band.
    const float promotionThreshold = geometry.Lods[desiredLod].MinimumScreenCoverage * (1.0f + deadBand);
    return projectedCoverage >= promotionThreshold ? desiredLod : previousLod;
  }

  // Demote only after crossing below the current LOD threshold minus a dead band.
  const float demotionThreshold = geometry.Lods[previousLod].MinimumScreenCoverage * (1.0f - deadBand);
  return projectedCoverage < demotionThreshold ? desiredLod : previousLod;
}

bool SphereInsideFrustum(float3 center, float radius, GpuVisibilityView view)
{
  // xiiFrustum planes point outwards. A sphere is outside when its signed distance in the
  // outward normal direction is greater than its radius.
  [unroll] for (uint plane = 0u; plane < 6u; ++plane) if (dot(view.FrustumPlanes[plane].xyz, center) + view.FrustumPlanes[plane].w > radius) return false;
  return true;
}
