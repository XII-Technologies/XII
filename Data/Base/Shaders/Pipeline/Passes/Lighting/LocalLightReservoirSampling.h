/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Pipeline/Passes/Lighting/LocalLightReservoirConstants.h>
#include <Shaders/Pipeline/Passes/ScreenSpaceGeometry.h>

Texture2D<uint> g_ReservoirObject;
#if XII_LOCAL_RESERVOIR_SEED
XII_STORAGE_IMAGE_FORMAT("rgba32ui")
RWTexture2D<uint4> g_SeedReservoirs;
#else
Texture2D<uint4>  g_CurrentReservoirs;
Texture2D<uint4>  g_PreviousReservoirs;
Texture2D<float2> g_ReservoirVelocity;
#endif

static const uint InvalidLight    = 0xffffffffu;
static const uint CandidateCount  = 4u;
static const uint PotentialShadow = 1u << 31u;

struct LocalReservoir
{
  uint4 Light;    // Compact index, stable identity, revision, normalized weight.
  uint4 Receiver; // World position and packed octahedral normal.
  uint4 Surface;  // Object, material signature, cluster, candidate count and shadow bit.
  uint4 Domain;   // Membership fingerprint, frame, and packed viewport size.
};

struct LocalSelection
{
  uint  Index;
  float Target;
  float Sum;
  uint  Count;
};

uint LocalHash(uint value)
{
  value = (value ^ (value >> 16u)) * 0x7feb352du;
  value = (value ^ (value >> 15u)) * 0x846ca68bu;
  return value ^ (value >> 16u);
}

float LocalRandom(inout uint state)
{
  state = LocalHash(state);
  return float(state & 0x00ffffffu) / 16777216.0f;
}

float LocalTarget(float3 radiance)
{
  return all(isfinite(radiance)) ? dot(max(radiance, 0), float3(0.2126f, 0.7152f, 0.0722f)) : 0;
}

void SelectLocal(inout LocalSelection selection, uint index, float target, float weight, uint count, inout uint randomState)
{
  selection.Count += count;
  if (target <= 0 || weight <= 0 || !isfinite(weight))
    return;
  selection.Sum += weight;
  if (LocalRandom(randomState) * selection.Sum < weight)
  {
    selection.Index  = index;
    selection.Target = target;
  }
}

LocalSelection FreshLocalSelection(uint2 range, SurfaceSample surface, inout uint randomState)
{
  LocalSelection selection = (LocalSelection)0;
  selection.Index          = InvalidLight;
  [unroll] for (uint candidate = 0; candidate < CandidateCount; ++candidate)
  {
    const uint  index  = g_LightIndices[range.x + min(uint(LocalRandom(randomState) * float(range.y)), range.y - 1u)];
    const float target = LocalTarget(EvaluateUnshadowedLocal(index, surface));
    SelectLocal(selection, index, target, target * float(range.y), 1u, randomState);
  }
  return selection;
}

uint LocalViewportKey()
{
  return (SurfaceExtent.x - 1u) | ((SurfaceExtent.y - 1u) << 16u);
}

uint LocalSurfaceKey(uint2 pixel)
{
  const uint4 material       = uint4(round(saturate(g_SurfaceMaterial.Load(int3(pixel, 0))) * 255.0f));
  const uint4 albedo         = uint4(round(saturate(g_SurfaceAlbedo.Load(int3(pixel, 0))) * 255.0f));
  const uint  packedMaterial = material.x | (material.y << 8u) | (material.z << 16u) | (material.w << 24u);
  const uint  packedAlbedo   = albedo.x | (albedo.y << 8u) | (albedo.z << 16u) | (albedo.w << 24u);
  return LocalHash(packedMaterial) ^ packedAlbedo;
}

uint2 LocalReservoirWriter(uint2 tile)
{
  const uint stride = LocalReservoirGrid.x;
  const uint phase  = LocalReservoirGrid.y % (stride * stride);
  return min(tile * stride + uint2(phase % stride, phase / stride), SurfaceExtent.xy - 1u);
}

#if XII_LOCAL_RESERVOIR_SEED
void WriteLocalReservoir(uint2 tile, LocalReservoir value)
{
  const uint2 texel                     = tile * XII_LOCAL_LIGHT_RESERVOIR_TEXELS;
  g_SeedReservoirs[texel]               = value.Light;
  g_SeedReservoirs[texel + uint2(1, 0)] = value.Receiver;
  g_SeedReservoirs[texel + uint2(0, 1)] = value.Surface;
  g_SeedReservoirs[texel + uint2(1, 1)] = value.Domain;
}

LocalReservoir SeedLocalReservoir(uint2 pixel)
{
  LocalReservoir result = (LocalReservoir)0;
  SurfaceSample  surface;
  if (!LoadSurface(pixel, surface) || g_Overflow[0] != 0)
    return result;
  const uint  cluster = LocalClusterIdentity(pixel, surface.Position);
  const uint2 range   = LocalClusterRange(cluster);
  if (range.y <= XII_LOCAL_LIGHT_RESERVOIR_MIN_LIGHTS)
    return result;

  uint2 fingerprint = uint2(LocalHash(range.y), LocalHash(range.y ^ 0x9e3779b9u));
  bool  castsShadow = false;
  [loop] for (uint i = 0; i < range.y; ++i)
  {
    const uint index = g_LightIndices[range.x + i];
    if (index >= ActiveLocalLightRecordCount())
      return result;
    const xiiGpuLightData light = g_Lights[index];
    const uint            key   = LocalHash(light.Metadata.x ^ (light.Metadata.w * 0x9e3779b9u));
    fingerprint.x ^= key;
    fingerprint.y += LocalHash(key ^ 0x85ebca6bu);
    castsShadow = castsShadow || (GetLightType(light) == XII_LIGHT_TYPE_SPOT && light.ShadowData.x > 0.5f);
  }
  uint                 randomState = LocalHash(pixel.x + pixel.y * SurfaceExtent.x + LocalReservoirGrid.y * 0x9e3779b9u + 1u);
  const LocalSelection selection   = FreshLocalSelection(range, surface, randomState);
  const uint2          normal      = uint2(round(saturate(g_SurfaceNormal.Load(int3(pixel, 0))) * 65535.0f));
  result.Receiver                  = uint4(asuint(surface.Position), normal.x | (normal.y << 16u));
  result.Surface                   = uint4(g_ReservoirObject.Load(int3(pixel, 0)), LocalSurfaceKey(pixel), cluster, selection.Count | (castsShadow ? PotentialShadow : 0u));
  result.Domain                    = uint4(fingerprint, LocalReservoirGrid.y, LocalViewportKey());
  result.Light.x                   = InvalidLight;
  if (selection.Index != InvalidLight && isfinite(selection.Sum))
  {
    const xiiGpuLightData light  = g_Lights[selection.Index];
    const float           weight = (selection.Sum / float(selection.Count)) / selection.Target;
    if (isfinite(weight))
      result.Light = uint4(selection.Index, light.Metadata.x, light.Metadata.w, asuint(weight));
  }
  return result;
}
#else
LocalReservoir ReadLocalReservoir(Texture2D<uint4> atlas, uint2 tile)
{
  const uint2    texel = tile * XII_LOCAL_LIGHT_RESERVOIR_TEXELS;
  LocalReservoir result;
  result.Light    = atlas.Load(int3(texel, 0));
  result.Receiver = atlas.Load(int3(texel + uint2(1, 0), 0));
  result.Surface  = atlas.Load(int3(texel + uint2(0, 1), 0));
  result.Domain   = atlas.Load(int3(texel + uint2(1, 1), 0));
  return result;
}

void MergeLocal(LocalReservoir source, LocalReservoir current, SurfaceSample surface, uint2 receiverKey, uint frame, uint limit,
                inout LocalSelection selection, inout uint randomState)
{
  const uint count = min(source.Surface.w & ~PotentialShadow, limit);
  if (count == 0 || any(source.Domain.xy != current.Domain.xy) || source.Domain.z != frame || source.Domain.w != LocalViewportKey() || any(source.Surface.xy != receiverKey))
    return;
  const uint   packedNormal = source.Receiver.w;
  const float2 oct          = float2(packedNormal & 0xffffu, packedNormal >> 16u) / 65535.0f;
  const float3 delta        = asfloat(source.Receiver.xyz) - surface.Position;
  if (dot(xiiScreenNormal(oct), surface.Normal) < 0.9f || abs(dot(delta, surface.Normal)) > 0.06f || dot(delta, delta) > 0.25f)
    return;
  const uint index = source.Light.x;
  if (index == InvalidLight)
  {
    selection.Count += count;
    return;
  }
  if (index >= ActiveLocalLightRecordCount())
    return;
  const xiiGpuLightData light = g_Lights[index];
  if (light.Metadata.x != source.Light.y || light.Metadata.w != source.Light.z)
    return;
  const float target = LocalTarget(EvaluateUnshadowedLocal(index, surface));
  SelectLocal(selection, index, target, target * asfloat(source.Light.w) * float(count), count, randomState);
}

bool ResolveLocalReservoir(uint2 pixel, uint cluster, SurfaceSample surface, bool receives, inout float reactive, out float3 result)
{
  result                       = 0;
  const uint2          tile    = pixel / LocalReservoirGrid.x;
  const LocalReservoir current = ReadLocalReservoir(g_CurrentReservoirs, tile);
  if (current.Surface.z != cluster || current.Domain.z != LocalReservoirGrid.y)
    return false;

  const uint2    receiverKey = uint2(g_ReservoirObject.Load(int3(pixel, 0)), LocalSurfaceKey(pixel));
  const uint2    range       = LocalClusterRange(cluster);
  uint           randomState = LocalHash(pixel.x + pixel.y * SurfaceExtent.x + LocalReservoirGrid.y * 0x9e3779b9u + 1u);
  LocalSelection selection   = (LocalSelection)0;
  selection.Index            = InvalidLight;
  if (all(pixel == LocalReservoirWriter(tile)))
    MergeLocal(current, current, surface, receiverKey, LocalReservoirGrid.y, CandidateCount, selection, randomState);
  else
    selection = FreshLocalSelection(range, surface, randomState);

  const int2 tileCount = int2((SurfaceExtent.xy - 1u) / LocalReservoirGrid.x + 1u);
  if (LocalReservoirGrid.w != 0)
  {
    const float2 uv         = (float2(pixel) + 0.5f) / float2(SurfaceExtent.xy);
    const float2 previousUV = uv + (LocalReservoirReprojection.xy - g_ReservoirVelocity.Load(int3(pixel, 0))) * float2(0.5f, -0.5f);
    if (all(isfinite(previousUV)) && all(previousUV >= 0) && all(previousUV < 1))
    {
      const uint2 previousTile = uint2(previousUV * float2(SurfaceExtent.xy)) / LocalReservoirGrid.x;
      MergeLocal(ReadLocalReservoir(g_PreviousReservoirs, previousTile), current, surface, receiverKey, LocalReservoirGrid.z, CandidateCount, selection, randomState);
    }
  }
  const int2 axis = (LocalHash(randomState) & 1u) != 0u ? int2(1, 0) : int2(0, 1);
  [unroll] for (int offset = -1; offset <= 1; offset += 2)
  {
    const int2 neighbor = int2(tile) + axis * offset;
    if (all(neighbor >= 0) && all(neighbor < tileCount))
      MergeLocal(ReadLocalReservoir(g_CurrentReservoirs, uint2(neighbor)), current, surface, receiverKey, LocalReservoirGrid.y, 2u, selection, randomState);
  }
#  if XII_SPOT_SHADOWS
  // Reactivity describes potential visibility changes for the whole cluster.
  // The selected light alone cannot provide a reliable reactive mask.
  if (receives && (current.Surface.w & PotentialShadow) != 0)
    reactive = 1;
#  endif
  if (selection.Index != InvalidLight && selection.Count > 0 && selection.Target > 0 && isfinite(selection.Sum))
    result = Evaluate(selection.Index, surface, receives, reactive) * ((selection.Sum / float(selection.Count)) / selection.Target);
  return true;
}
#endif
