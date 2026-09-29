/// Copyright (c) Theophilus Eriata. All Rights Reserved.

// Shared ReSTIR direct-illumination reservoir helpers.
#pragma once

#include "LightingData.h"

static const uint XII_RESTIR_INVALID_LIGHT = 0xFFFFFFFFu;
static const uint XII_RESTIR_CANDIDATES    = 8u;
static const uint XII_RESTIR_MAX_HISTORY_M = 20u;
static const uint XII_RESTIR_MAX_M         = 32u;

struct xiiReSTIRDIReservoir
{
  uint  LightIndex;
  uint  StableLightId;
  float WeightSum;
  uint  M;
};

uint ReSTIRHash(uint value)
{
  value ^= value >> 16u;
  value *= 0x7FEB352Du;
  value ^= value >> 15u;
  value *= 0x846CA68Bu;
  value ^= value >> 16u;
  return value;
}

float ReSTIRRandom01(inout uint state)
{
  state = ReSTIRHash(state);
  return (float)(state & 0x00FFFFFFu) * (1.0f / 16777216.0f);
}

xiiReSTIRDIReservoir ReSTIREmptyReservoir()
{
  xiiReSTIRDIReservoir reservoir;
  reservoir.LightIndex   = XII_RESTIR_INVALID_LIGHT;
  reservoir.StableLightId = 0u;
  reservoir.WeightSum    = 0.0f;
  reservoir.M            = 0u;
  return reservoir;
}

bool ReSTIRIsFinitePositive(float value)
{
  return value > 0.0f && value < 3.402823466e+38f;
}

bool ReSTIRValidateReservoir(xiiReSTIRDIReservoir reservoir)
{
  if (reservoir.LightIndex >= g_ActiveLightCount || reservoir.M == 0u || reservoir.M > XII_RESTIR_MAX_M || !ReSTIRIsFinitePositive(reservoir.WeightSum))
    return false;

  return g_Lights[reservoir.LightIndex].Metadata.x == reservoir.StableLightId;
}

xiiReSTIRDIReservoir ReSTIRUnpackReservoir(uint4 packedReservoir)
{
  xiiReSTIRDIReservoir reservoir;
  reservoir.LightIndex    = packedReservoir.x;
  reservoir.StableLightId = packedReservoir.y;
  reservoir.WeightSum     = asfloat(packedReservoir.z);
  reservoir.M             = packedReservoir.w;
  return reservoir;
}

uint4 ReSTIRPackReservoir(xiiReSTIRDIReservoir reservoir)
{
  return uint4(reservoir.LightIndex, reservoir.StableLightId, asuint(reservoir.WeightSum), reservoir.M);
}

void ReSTIRUpdateReservoir(inout xiiReSTIRDIReservoir reservoir, uint lightIndex, uint stableLightId, float candidateWeight, uint candidateM, inout uint randomState)
{
  candidateM = min(candidateM, XII_RESTIR_MAX_M - reservoir.M);
  if (candidateM == 0u)
    return;

  reservoir.M += candidateM;
  if (!ReSTIRIsFinitePositive(candidateWeight))
    return;

  reservoir.WeightSum += candidateWeight;
  if (ReSTIRRandom01(randomState) * reservoir.WeightSum <= candidateWeight)
  {
    reservoir.LightIndex    = lightIndex;
    reservoir.StableLightId = stableLightId;
  }
}

float ReSTIRDistanceAttenuation(float distanceToLight, float range)
{
  const float normalizedDistance = saturate(distanceToLight / max(range, 1e-3f));
  float attenuation = saturate(1.0f - normalizedDistance * normalizedDistance);
  attenuation *= attenuation;
  return attenuation / max(distanceToLight * distanceToLight, 0.25f);
}

float ReSTIRSpotAttenuation(xiiGpuLightData lightData, float3 pointToLightDirection)
{
  const float cosTheta = dot(-pointToLightDirection, normalize(lightData.DirectionAndType.xyz));
  const float cone = saturate((cosTheta - lightData.SpotAnglesAndRectSize.y) /
                              max(lightData.SpotAnglesAndRectSize.x - lightData.SpotAnglesAndRectSize.y, 1e-4f));
  return cone * cone * (3.0f - 2.0f * cone);
}

// Scalar importance target used by candidate generation and final normalization.
// Visibility is deliberately excluded: the selected sample is shadow-tested by the
// direct-lighting pass while the inexpensive target remains suitable for reuse.
float ReSTIREstimateTarget(xiiGpuLightData lightData, float3 worldPosition, float3 normal)
{
  const float3 representativePosition = GetRepresentativeLightPosition(lightData, worldPosition);
  const float3 toLight = representativePosition - worldPosition;
  const float distanceToLight = length(toLight);
  const float3 pointToLight = distanceToLight > 1e-4f ? toLight / distanceToLight : normalize(-lightData.DirectionAndType.xyz);

  float attenuation = ReSTIRDistanceAttenuation(distanceToLight, lightData.AttenuationAndSize.x);
  if (GetLightType(lightData) == XII_LIGHT_TYPE_SPOT)
    attenuation *= ReSTIRSpotAttenuation(lightData, pointToLight);

  attenuation *= GetPhotometricEmitterScale(lightData, pointToLight);
  const float luminance = dot(lightData.ColorAndIntensity.rgb, float3(0.2126f, 0.7152f, 0.0722f));
  return max(luminance * lightData.ColorAndIntensity.w * attenuation * saturate(dot(normal, pointToLight)), 0.0f);
}

void ReSTIRFinalizeReservoir(inout xiiReSTIRDIReservoir reservoir, float selectedTarget)
{
  if (reservoir.LightIndex == XII_RESTIR_INVALID_LIGHT || reservoir.M == 0u || !ReSTIRIsFinitePositive(selectedTarget))
  {
    reservoir = ReSTIREmptyReservoir();
    return;
  }

  reservoir.WeightSum = reservoir.WeightSum / ((float)reservoir.M * selectedTarget);
  if (!ReSTIRIsFinitePositive(reservoir.WeightSum))
    reservoir = ReSTIREmptyReservoir();
}

