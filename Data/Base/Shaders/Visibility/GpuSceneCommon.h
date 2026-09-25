#pragma once

struct GpuSceneInstance
{
  float4x4 GlobalTransform;
  float4x4 PreviousGlobalTransform;
  float4 BoundsCenterRadius;
  float4 BoundsExtents;
  uint GeometryIndex;
  uint MaterialIndex;
  uint ObjectIndex;
  uint Flags;
  uint VisibilityMask;
  uint UserData;
  uint Padding0;
  uint Padding1;
};

struct GpuVisibilityView
{
  float4x4 ViewProjectionMatrix;
  float4 FrustumPlanes[6];
  float4 CameraPosition;
  float4 ViewportAndHiZ;
  uint InstanceCount;
  uint VisibilityMask;
  uint RequiredFlags;
  uint ExcludedFlags;
  uint GeometryBaseIndex;
  uint3 Padding;
};

struct GpuGeometryLod
{
  uint VertexBufferIndex;
  uint IndexBufferIndex;
  uint MeshletBufferIndex;
  uint MeshletRemapBufferIndex;
  uint MeshletPrimitiveBufferIndex;
  uint VertexCount;
  uint IndexCount;
  uint MeshletCount;
  float MinimumScreenCoverage;
  uint IndexType;
  uint MeshletMetadataOffset;
  uint Padding;
};

struct GpuGeometryRecord
{
  float4 BoundsCenterRadius;
  float4 BoundsExtents;
  GpuGeometryLod Lods[8];
  uint LodCount;
  uint Generation;
  uint ResidentLodMask;
  uint Flags;
};

struct GpuMeshlet
{
  uint4 Header;
  uint PackedPrimitiveVertexCount;
  uint PackedLodSection;
  float4 Bounds;
  float4 Cone;
};

uint SelectResidentLod(GpuGeometryRecord geometry, float worldSpaceRadius, float distanceToCamera, float viewportHeight)
{
  float projectedCoverage = worldSpaceRadius * viewportHeight / max(distanceToCamera, 0.001f);
  uint fallback = 0xFFFFFFFFu;
  [unroll]
  for (uint lod = 0u; lod < geometry.LodCount; ++lod)
  {
    if ((geometry.ResidentLodMask & (1u << lod)) == 0u)
      continue;
    fallback = lod;
    if (projectedCoverage >= geometry.Lods[lod].MinimumScreenCoverage)
      return lod;
  }
  return fallback;
}

bool SphereInsideFrustum(float3 center, float radius, GpuVisibilityView view)
{
  [unroll]
  for (uint plane = 0u; plane < 6u; ++plane)
    if (dot(view.FrustumPlanes[plane].xyz, center) + view.FrustumPlanes[plane].w < -radius)
      return false;
  return true;
}
