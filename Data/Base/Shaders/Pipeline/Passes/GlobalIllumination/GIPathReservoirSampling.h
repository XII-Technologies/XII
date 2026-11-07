/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Pipeline/Passes/GlobalIllumination/GIPathReservoirConstants.h>

#if GI_PATH_RESERVOIRS == GI_PATH_RESERVOIRS_SEED
RWStructuredBuffer<xiiGIPathReservoir> g_CurrentReservoirs;
#else
StructuredBuffer<xiiGIPathReservoir> g_PreviousReservoirs;
StructuredBuffer<xiiGIPathReservoir> g_SpatialReservoirs;
Texture2D<float2>                    g_ReceiverVelocity;
#endif
Texture2D<uint>   g_ReceiverObject;
Texture2D<float4> g_ReceiverMaterial;

static const float IrradiancePi          = 3.14159265359f;
static const uint  ReservoirHistoryLimit = 32u;

struct PathSelection
{
  float3 Endpoint;
  float  WeightSum;
  float  Target;
  uint   Count;
  uint   Seed;
};

float PathLuminance(float3 radiance)
{
  return dot(radiance, float3(0.2126f, 0.7152f, 0.0722f));
}

// The target is in endpoint area measure. This accounts for the changing
// solid-angle projection when connecting the same sample to a new receiver.
float PathAreaTarget(IrradiancePayload path, float3 direction, float3 normal, float distanceSquared)
{
  const float receiverCosine = max(dot(normal, direction), 0);
  const float emitterCosine  = abs(dot(path.EndpointNormal, direction));
  return PathLuminance(path.Radiance) * receiverCosine * emitterCosine / max(distanceSquared, 1e-6f);
}

void SelectPath(inout PathSelection selection, IrradiancePayload path, uint seed, float target, float weight, uint count, inout uint randomState)
{
  selection.Count += count;
  if (target <= 0 || weight <= 0 || !isfinite(weight) || !isfinite(target))
    return;
  selection.WeightSum += weight;
  if (IrradianceRandom(randomState) * selection.WeightSum < weight)
  {
    selection.Endpoint = path.Endpoint;
    selection.Target   = target;
    selection.Seed     = seed;
  }
}

uint4 ReceiverKeys(uint2 pixel, float2 encodedNormal)
{
  const uint2 normal   = uint2(round(saturate(encodedNormal) * 65535.0f));
  const uint4 material = uint4(round(saturate(g_ReceiverMaterial.Load(int3(pixel, 0))) * 255.0f));
  return uint4(0, normal.x | (normal.y << 16u), g_ReceiverObject.Load(int3(pixel, 0)),
               material.x | (material.y << 8u) | (material.z << 16u) | (material.w << 24u));
}

uint2 ReservoirWriter(uint2 tile, uint2 extent)
{
  const uint stride = ReservoirGrid.z;
  const uint phase  = EmissiveSamples.y % (stride * stride);
  return min(tile * stride + uint2(phase % stride, phase / stride), extent - 1u);
}

#if GI_PATH_RESERVOIRS == GI_PATH_RESERVOIRS_RESOLVE
struct PathEstimate
{
  float3 Sum;
  float  Count;
};

void ReconnectPath(int2 tile, bool sameFrame, uint4 receiverKeys, float3 receiver, float3 normal, float footprint, float coneSpread, float confidence,
                   inout PathEstimate estimate)
{
  if (any(tile < 0) || any(tile >= int2(ReservoirGrid.xy)))
    return;
  const uint         index = uint(tile.y) * ReservoirGrid.x + uint(tile.x);
  xiiGIPathReservoir previous;
  if (sameFrame)
    previous = g_SpatialReservoirs[index];
  else
    previous = g_PreviousReservoirs[index];
  if (previous.ReceiverAndCount.w <= 0 || any(previous.Keys.zw != receiverKeys.zw))
    return;
  const float2 previousOct   = float2(previous.Keys.y & 0xffffu, previous.Keys.y >> 16u) / 65535.0f;
  const float3 receiverDelta = previous.ReceiverAndCount.xyz - receiver;
  if (dot(xiiScreenNormal(previousOct), normal) < 0.9f || abs(dot(receiverDelta, normal)) > max(0.025f, footprint) ||
      length(receiverDelta) > clamp(footprint * float(ReservoirGrid.z) * 6.0f, 0.08f, 0.75f))
    return;

  // Zero-target proposals still belong in the mixture denominator.
  const float count = min(previous.ReceiverAndCount.w, float(ReservoirHistoryLimit)) * confidence;
  estimate.Count += count;
  if (previous.EndpointAndWeight.w <= 0)
    return;

  RayDesc ray;
  ray.Origin                   = receiver + normal * RayDistances.y;
  const float3 delta           = previous.EndpointAndWeight.xyz - ray.Origin;
  const float  distanceSquared = dot(delta, delta);
  const float  distance        = sqrt(distanceSquared);
  if (!isfinite(distance) || distance <= 0.003f || distance > RayDistances.x)
    return;
  ray.Direction = delta / distance;
  if (dot(normal, ray.Direction) <= 0)
    return;
  // Ray replay re-evaluates the current material, lights, and all subsequent
  // bounces. The first intersection must still be the retained endpoint.
  const float tolerance  = max(0.001f, distance * 0.00005f);
  ray.TMin               = 0.001f;
  ray.TMax               = min(RayDistances.x, distance + tolerance);
  IrradiancePayload path = (IrradiancePayload)0;
  path.Random            = previous.Keys.x;
  path.ConeWidth         = footprint;
  path.ConeSpread        = coneSpread;
  path.Endpoint          = previous.EndpointAndWeight.xyz;
  path.EndpointNormal    = 0;
  path.Flags             = XII_IRRADIANCE_RECONNECTION_RAY;
#  if GI_EMISSIVE_SAMPLING || GI_ENVIRONMENT_SAMPLING
  path.PreviousPDF = max(dot(normal, ray.Direction), 0) / IrradiancePi;
#  endif
  TraceRay(g_Scene, 0, 0xff, 0, 1, 0, ray, path);
  path.Radiance               = all(isfinite(path.Radiance)) ? max(path.Radiance, 0) : 0;
  const float  receiverCosine = max(dot(normal, ray.Direction), 0);
  const float  endpointCosine = abs(dot(path.EndpointNormal, ray.Direction));
  const float  scale          = receiverCosine * endpointCosine / distanceSquared * previous.EndpointAndWeight.w * count;
  const float3 contribution   = path.Radiance * scale;
  if (all(isfinite(contribution)))
    estimate.Sum += contribution;
}

#else
void StorePathReservoir(uint2 tile, float3 receiver, uint4 keys, PathSelection selection, float3 freshRadiance)
{
  const uint         index     = tile.y * ReservoirGrid.x + tile.x;
  xiiGIPathReservoir reservoir = (xiiGIPathReservoir)0;
  if (selection.Count > 0)
  {
    reservoir.ReceiverAndCount = float4(receiver, float(min(selection.Count, ReservoirHistoryLimit)));
    reservoir.Keys             = uint4(selection.Seed, keys.yzw);
    reservoir.FreshRadiance    = float4(freshRadiance, 0);
  }
  if (selection.Count > 0 && selection.Target > 0 && isfinite(selection.WeightSum))
  {
    const float normalization = (selection.WeightSum / float(selection.Count)) / selection.Target;
    if (isfinite(normalization) && normalization > 0)
      reservoir.EndpointAndWeight = float4(selection.Endpoint, normalization);
  }
  g_CurrentReservoirs[index] = reservoir;
}
#endif
