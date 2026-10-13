/// Copyright (c) Theophilus Eriata. All Rights Reserved.

// Shared implementation for meshlet frustum, cone, and optional temporal Hi-Z culling.

#include "GpuSceneCommon.h"

StructuredBuffer<GpuSceneInstance>  g_SceneInstances;
StructuredBuffer<GpuVisibilityView> g_VisibilityView;
StructuredBuffer<GpuGeometryRecord> g_Geometry;
StructuredBuffer<GpuMeshlet>        g_Meshlets;
StructuredBuffer<uint>              g_VisibleInstances;
StructuredBuffer<uint>              g_VisibleInstanceCount;
StructuredBuffer<uint4>             g_CurrentLodHistory;
RWStructuredBuffer<GpuLodFeedback>  g_LodFeedback;
RWStructuredBuffer<uint2>           g_VisibleMeshlets;
RWStructuredBuffer<uint>            g_VisibleMeshletCount;

uint HashLodFeedbackKey(uint key)
{
  key ^= key >> 16u;
  key *= 0x7FEB352Du;
  key ^= key >> 15u;
  key *= 0x846CA68Bu;
  return key ^ (key >> 16u);
}

void SubmitLodFeedback(uint geometryIndex, GpuGeometryRecord geometry, uint desiredLod, uint selectedLod,
                       uint purposeMask, uint objectUserData, float projectedCoverage)
{
  uint entryCount, entryStride;
  g_LodFeedback.GetDimensions(entryCount, entryStride);
  if (entryCount <= 1u || purposeMask == 0u || desiredLod >= min(geometry.LodCount, 8u))
    return;
  if ((geometry.ResidentLodMask & (1u << desiredLod)) != 0u)
    return;

  uint ignored;
  InterlockedAdd(g_LodFeedback[0].Key, 1u, ignored);

  const uint capacity   = entryCount - 1u;
  const uint requestKey = (((desiredLod & 7u) << 20u) | (geometryIndex & 0xFFFFFu)) + 1u;
  const uint baseSlot   = HashLodFeedbackKey(requestKey) & (capacity - 1u);
  [loop] for (uint probe = 0u; probe < min(capacity, 32u); ++probe)
  {
    const uint entryIndex = 1u + ((baseSlot + probe) & (capacity - 1u));
    uint       observedKey;
    InterlockedCompareExchange(g_LodFeedback[entryIndex].Key, 0u, requestKey, observedKey);
    if (observedKey != 0u && observedKey != requestKey)
      continue;

    if (observedKey == 0u)
      InterlockedAdd(g_LodFeedback[0].GeometryGeneration, 1u, ignored);
    InterlockedMax(g_LodFeedback[entryIndex].GeometryGeneration, geometry.Generation);
    InterlockedMax(g_LodFeedback[entryIndex].DesiredLod, desiredLod);
    InterlockedMax(g_LodFeedback[entryIndex].SelectedLod, selectedLod);
    InterlockedOr(g_LodFeedback[entryIndex].PurposeMask, purposeMask);
    InterlockedMax(g_LodFeedback[entryIndex].ObjectUserData, objectUserData);
    InterlockedMax(g_LodFeedback[entryIndex].Priority, min((uint)(max(projectedCoverage, 0.0f) * 1024.0f), 0x3FFFFFFFu));
    return;
  }

  InterlockedAdd(g_LodFeedback[0].DesiredLod, 1u, ignored);
}

#if XII_GPU_SCENE_MESHLET_HIZ
Texture2D<float> g_HiZ;

// Projects a conservative world-space box around the meshlet sphere. Bounds crossing the
// projection plane cannot be represented by a finite rectangle and remain visible.
bool ProjectMeshletBounds(float3 center, float radius, GpuVisibilityView view, out float4 screenRect, out float nearestDepth)
{
  float2 minimumUV = float2(1.0f, 1.0f);
  float2 maximumUV = float2(0.0f, 0.0f);
  nearestDepth     = 0.0f;

  [unroll] for (uint cornerIndex = 0u; cornerIndex < 8u; ++cornerIndex)
  {
    const float3 cornerSign = float3((cornerIndex & 1u) != 0u ? 1.0f : -1.0f,
                                     (cornerIndex & 2u) != 0u ? 1.0f : -1.0f,
                                     (cornerIndex & 4u) != 0u ? 1.0f : -1.0f);
    const float4 clip = mul(view.OcclusionViewProjectionMatrix, float4(center + cornerSign * radius, 1.0f));
    if (clip.w <= 0.0001f)
    {
      screenRect = float4(0.0f, 0.0f, 1.0f, 1.0f);
      return false;
    }

    const float3 ndc = clip.xyz / clip.w;
    const float2 uv  = ndc.xy * float2(0.5f, -0.5f) + 0.5f;
    minimumUV        = min(minimumUV, uv);
    maximumUV        = max(maximumUV, uv);
    nearestDepth     = max(nearestDepth, saturate(ndc.z));
  }

  screenRect = saturate(float4(minimumUV, maximumUV));
  return true;
}

bool MeshletVisibleInHiZ(float3 center, float radius, GpuVisibilityView view)
{
  float4 rect;
  float  nearestDepth;
  if (!ProjectMeshletBounds(center, radius, view, rect, nearestDepth))
    return true;

  uint width, height, levels;
  g_HiZ.GetDimensions(0, width, height, levels);
  const float2 footprint = (rect.zw - rect.xy) * float2(width, height);
  const uint   mip       = min((uint)max(0.0f, floor(log2(max(max(footprint.x, footprint.y), 1.0f)))), max(levels, 1u) - 1u);

  uint mipWidth, mipHeight, ignoredLevels;
  g_HiZ.GetDimensions(mip, mipWidth, mipHeight, ignoredLevels);
  const int2  maximumPixel  = int2(max(mipWidth, 1u) - 1u, max(mipHeight, 1u) - 1u);
  const int2  p0            = clamp(int2(rect.xy * float2(mipWidth, mipHeight)), int2(0, 0), maximumPixel);
  const int2  p1            = clamp(int2(rect.zw * float2(mipWidth, mipHeight)), int2(0, 0), maximumPixel);
  const float occluderDepth = min(min(g_HiZ.Load(int3(p0, mip)), g_HiZ.Load(int3(p1.x, p0.y, mip))),
                                  min(g_HiZ.Load(int3(p0.x, p1.y, mip)), g_HiZ.Load(int3(p1, mip))));
  return nearestDepth >= occluderDepth - view.ViewportAndHiZ.w;
}
#endif

bool ReserveVisibleMeshlet(uint maximumCount, out uint destination)
{
  uint capacity, stride;
  uint counterCapacity, counterStride;
  g_VisibleMeshlets.GetDimensions(capacity, stride);
  g_VisibleMeshletCount.GetDimensions(counterCapacity, counterStride);
  capacity = min(capacity, maximumCount);
  if (capacity == 0u || counterCapacity == 0u)
  {
    destination = 0u;
    return false;
  }

  // A plain InterlockedAdd can leave the counter larger than the output buffer. The
  // indirect mesh dispatch consumes this counter, so reserve with compare/exchange and
  // never publish an out-of-range workgroup count.
  for (;;)
  {
    const uint currentCount = g_VisibleMeshletCount[0];
    if (currentCount >= capacity)
    {
      destination = 0u;
      return false;
    }

    uint observedCount;
    InterlockedCompareExchange(g_VisibleMeshletCount[0], currentCount, currentCount + 1u, observedCount);
    if (observedCount == currentCount)
    {
      destination = currentCount;
      return true;
    }
  }
}

[numthreads(64, 1, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID, uint3 groupId : SV_GroupID) {
  uint viewCount, viewStride;
  uint sceneCount, sceneStride;
  uint geometryCount, geometryStride;
  uint meshletCount, meshletStride;
  uint visibleInstanceCapacity, visibleInstanceStride;
  uint visibleCountCapacity, visibleCountStride;
  uint currentHistoryCapacity, currentHistoryStride;
  g_VisibilityView.GetDimensions(viewCount, viewStride);
  g_SceneInstances.GetDimensions(sceneCount, sceneStride);
  g_Geometry.GetDimensions(geometryCount, geometryStride);
  g_Meshlets.GetDimensions(meshletCount, meshletStride);
  g_VisibleInstances.GetDimensions(visibleInstanceCapacity, visibleInstanceStride);
  g_VisibleInstanceCount.GetDimensions(visibleCountCapacity, visibleCountStride);
  g_CurrentLodHistory.GetDimensions(currentHistoryCapacity, currentHistoryStride);
  if (viewCount == 0u || visibleCountCapacity == 0u)
    return;

  const uint visibleCount         = min(g_VisibleInstanceCount[0], visibleInstanceCapacity);
  const uint visibleInstanceIndex = groupId.y + groupId.z * 65535u;
  if (visibleInstanceIndex >= visibleCount)
    return;

  const uint instanceIndex = g_VisibleInstances[visibleInstanceIndex];
  if (instanceIndex >= sceneCount)
    return;

  const GpuSceneInstance  instance = g_SceneInstances[instanceIndex];
  const GpuVisibilityView view     = g_VisibilityView[0];
  if (view.GeometryBaseIndex >= geometryCount || instance.GeometryIndex >= geometryCount - view.GeometryBaseIndex)
    return;

  // Instance culling already selected the LOD before routing and occlusion tests.
  // Streaming feedback uses the same representative and active-instance bounds.
  uint selectionIndex = instanceIndex;
  if (instance.LodSelectionIndex < min(view.InstanceCount, sceneCount) && g_SceneInstances[instance.LodSelectionIndex].GeometryIndex == instance.GeometryIndex)
    selectionIndex = instance.LodSelectionIndex;
  const GpuSceneInstance  selection         = g_SceneInstances[selectionIndex];
  const GpuGeometryRecord geometry          = g_Geometry[view.GeometryBaseIndex + instance.GeometryIndex];
  const float             distanceToCamera  = length(selection.BoundsCenterRadius.xyz - view.CameraPosition.xyz);
  const float             projectedCoverage = GetInstanceLodRadius(selection) * view.ViewportAndHiZ.y / max(distanceToCamera, 0.001f);
  const uint              desiredLod        = SelectInstanceDesiredLod(geometry, selection, distanceToCamera, view.ViewportAndHiZ.y);
  const uint              lodIndex          = instanceIndex < currentHistoryCapacity ? g_CurrentLodHistory[instanceIndex].x : 0xFFFFFFFFu;
  if (dispatchThreadId.x == 0u)
  {
    SubmitLodFeedback(instance.GeometryIndex, geometry, desiredLod, lodIndex, (uint)view.LodParameters.w, instance.UserData, projectedCoverage);
  }
  if (lodIndex == 0xFFFFFFFFu || lodIndex >= min(geometry.LodCount, 8u))
    return;

  // Section ranges and material routing belong to one authored LOD. All candidates make
  // the same geometry-level selection, then only that LOD's candidates reach rasterization.
  if (instance.DrawLod != 0xFFFFFFFFu && instance.DrawLod != lodIndex)
    return;

  const GpuGeometryLod lod = geometry.Lods[lodIndex];
  float3               scale;
  scale.x              = length(instance.GlobalTransform[0].xyz);
  scale.y              = length(instance.GlobalTransform[1].xyz);
  scale.z              = length(instance.GlobalTransform[2].xyz);
  const float maxScale = max(scale.x, max(scale.y, scale.z));

  const uint localMeshlet = dispatchThreadId.x;
  if (localMeshlet >= lod.MeshletCount)
    return;
  if (lod.MeshletMetadataOffset >= meshletCount || localMeshlet >= meshletCount - lod.MeshletMetadataOffset)
    return;

  const uint       meshletIndex          = lod.MeshletMetadataOffset + localMeshlet;
  const GpuMeshlet meshlet               = g_Meshlets[meshletIndex];
  const uint       meshletPrimitiveCount = meshlet.PackedPrimitiveVertexCount & 0xFFFFu;
  if (instance.PrimitiveCount != 0u)
  {
    const uint selectedEnd = instance.FirstPrimitive + instance.PrimitiveCount;
    const uint meshletEnd  = meshlet.Header.x + meshletPrimitiveCount;
    if (meshlet.Header.x >= selectedEnd || meshletEnd <= instance.FirstPrimitive)
      return;
  }

  const bool   posed  = instance.SkinningCount != 0u;
  const float3 center = posed ? instance.BoundsCenterRadius.xyz : mul(instance.GlobalTransform, float4(meshlet.Bounds.xyz, 1.0f)).xyz;
  const float  radius = posed ? instance.BoundsCenterRadius.w : meshlet.Bounds.w * maxScale;
  if (!SphereInsideFrustum(center, radius, view))
    return;

  // The stored cutoff is the cosine of the largest deviation from the cone axis.
  // Reject only when the complete normal cone faces away from the camera. Non-uniform
  // transforms distort the cone, so conservatively disable this test for those instances.
  const float minScale       = min(scale.x, min(scale.y, scale.z));
  const bool  isUniformScale = maxScale - minScale <= max(maxScale, 0.0001f) * 0.001f;
  if (!posed && meshlet.Cone.w >= 0.0f && isUniformScale)
  {
    const float3 coneAxis     = normalize(mul((float3x3)instance.GlobalTransform, meshlet.Cone.xyz));
    const float3 toCamera     = normalize(view.CameraPosition.xyz - center);
    const float  sinHalfAngle = sqrt(saturate(1.0f - meshlet.Cone.w * meshlet.Cone.w));
    if (dot(coneAxis, toCamera) <= -sinHalfAngle)
      return;
  }

#if XII_GPU_SCENE_MESHLET_HIZ
  // Temporal Hi-Z is valid only for static geometry. Moving instances remain visible until
  // their current depth has reached the history ring on a later frame.
  const float transformDelta =
    dot(abs(instance.GlobalTransform[0] - instance.PreviousGlobalTransform[0]), 1.0f.xxxx) +
    dot(abs(instance.GlobalTransform[1] - instance.PreviousGlobalTransform[1]), 1.0f.xxxx) +
    dot(abs(instance.GlobalTransform[2] - instance.PreviousGlobalTransform[2]), 1.0f.xxxx) +
    dot(abs(instance.GlobalTransform[3] - instance.PreviousGlobalTransform[3]), 1.0f.xxxx);
  if (!posed && instance.PreviousSkinningCount == 0u && transformDelta <= 1.0e-5f && !MeshletVisibleInHiZ(center, radius, view))
    return;
#endif

  uint destination;
  if (ReserveVisibleMeshlet(view.MaxVisibleMeshlets, destination))
    g_VisibleMeshlets[destination] = uint2(instanceIndex, meshletIndex);
}
