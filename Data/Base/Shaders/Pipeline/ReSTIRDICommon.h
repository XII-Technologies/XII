/// Copyright (c) Theophilus Eriata. All Rights Reserved.

// Shared ReSTIR direct-illumination reservoir helpers.
#pragma once

#include "LightingData.h"
#include "PhotometricLightSampling.h"
#include <Shaders/Pipeline/Passes/ReSTIR/EmissiveLightData.h>

StructuredBuffer<xiiGpuEmissiveTriangle>    g_EmissiveTriangles;
StructuredBuffer<xiiGpuEmissiveLightHeader> g_EmissiveHeader;

static const uint XII_RESTIR_INVALID_LIGHT = 0xFFFFFFFFu;
static const uint XII_RESTIR_EMISSIVE_BIT  = 0x80000000u;
static const uint XII_RESTIR_SAMPLE_MASK   = 0x7FFFFFFFu;
static const uint XII_RESTIR_CANDIDATES    = 8u;
static const uint XII_RESTIR_MAX_HISTORY_M = 20u;
static const uint XII_RESTIR_MAX_M         = 32u;

struct xiiReSTIRDIReservoir
{
  uint  SampleIndex;
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
  reservoir.SampleIndex   = XII_RESTIR_INVALID_LIGHT;
  reservoir.StableLightId = 0u;
  reservoir.WeightSum     = 0.0f;
  reservoir.M             = 0u;
  return reservoir;
}

bool ReSTIRIsFinitePositive(float value)
{
  return value > 0.0f && value < 3.402823466e+38f;
}

bool ReSTIRValidateReservoir(xiiReSTIRDIReservoir reservoir)
{
  if (reservoir.M == 0u || reservoir.M > XII_RESTIR_MAX_M || !ReSTIRIsFinitePositive(reservoir.WeightSum))
    return false;

  if ((reservoir.SampleIndex & XII_RESTIR_EMISSIVE_BIT) != 0u)
  {
    uint triangleCapacity, triangleStride;
    uint headerCapacity, headerStride;
    g_EmissiveTriangles.GetDimensions(triangleCapacity, triangleStride);
    g_EmissiveHeader.GetDimensions(headerCapacity, headerStride);
    const uint triangleIndex = reservoir.SampleIndex & XII_RESTIR_SAMPLE_MASK;
    const uint triangleCount = headerCapacity > 0u ? min(g_EmissiveHeader[0].TriangleCount, triangleCapacity) : 0u;
    return triangleIndex < triangleCount && g_EmissiveTriangles[triangleIndex].Metadata.x == reservoir.StableLightId;
  }

  return reservoir.SampleIndex < g_ActiveLightCount && g_Lights[reservoir.SampleIndex].Metadata.x == reservoir.StableLightId;
}

xiiReSTIRDIReservoir ReSTIRUnpackReservoir(uint4 packedReservoir)
{
  xiiReSTIRDIReservoir reservoir;
  reservoir.SampleIndex   = packedReservoir.x;
  reservoir.StableLightId = packedReservoir.y;
  reservoir.WeightSum     = asfloat(packedReservoir.z);
  reservoir.M             = packedReservoir.w;
  return reservoir;
}

uint4 ReSTIRPackReservoir(xiiReSTIRDIReservoir reservoir)
{
  return uint4(reservoir.SampleIndex, reservoir.StableLightId, asuint(reservoir.WeightSum), reservoir.M);
}

void ReSTIRUpdateReservoir(inout xiiReSTIRDIReservoir reservoir, uint sampleIndex, uint stableLightId, float candidateWeight, uint candidateM, inout uint randomState)
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
    reservoir.SampleIndex   = sampleIndex;
    reservoir.StableLightId = stableLightId;
  }
}

// Scalar importance target used by candidate generation and final normalization.
// Visibility is deliberately excluded: the selected sample is shadow-tested by the
// direct-lighting pass while the inexpensive target remains suitable for reuse.
float ReSTIREstimateTarget(xiiGpuLightData lightData, float3 worldPosition, float3 normal)
{
  const float3 representativePosition = GetRepresentativeLightPosition(lightData, worldPosition);
  const float3 toLight                = representativePosition - worldPosition;
  const float  distanceToLight        = length(toLight);
  const float3 pointToLight           = distanceToLight > 1e-4f ? toLight / distanceToLight : normalize(-lightData.DirectionAndType.xyz);

  float attenuation = EvaluateFiniteEmitterAttenuation(distanceToLight, lightData.AttenuationAndSize.x, lightData.AttenuationAndSize.y);
  if (GetLightType(lightData) == XII_LIGHT_TYPE_SPOT)
    attenuation *= EvaluateSpotCone(lightData, pointToLight);

  attenuation *= SampleIESProfile(lightData, pointToLight);
  attenuation *= GetPhotometricEmitterScale(lightData, pointToLight);
  const float luminance = dot(lightData.ColorAndIntensity.rgb, float3(0.2126f, 0.7152f, 0.0722f));
  return max(luminance * lightData.ColorAndIntensity.w * attenuation * saturate(dot(normal, pointToLight)), 0.0f);
}

uint ReSTIRGetEmissiveTriangleCount()
{
  uint triangleCapacity, triangleStride;
  uint headerCapacity, headerStride;
  g_EmissiveTriangles.GetDimensions(triangleCapacity, triangleStride);
  g_EmissiveHeader.GetDimensions(headerCapacity, headerStride);
  return headerCapacity > 0u ? min(g_EmissiveHeader[0].TriangleCount, triangleCapacity) : 0u;
}

float ReSTIREmissiveSolidAngle(xiiGpuEmissiveTriangle emissiveTriangle, float3 worldPosition, out float3 directionToLight)
{
  const float3 d0 = emissiveTriangle.Position0AndArea.xyz - worldPosition;
  const float3 d1 = emissiveTriangle.Position1AndPowerWeight.xyz - worldPosition;
  const float3 d2 = emissiveTriangle.Position2AndInvArea.xyz - worldPosition;
  const float l0 = length(d0);
  const float l1 = length(d1);
  const float l2 = length(d2);
  const float3 centroidDirection = (emissiveTriangle.Position0AndArea.xyz + emissiveTriangle.Position1AndPowerWeight.xyz + emissiveTriangle.Position2AndInvArea.xyz) / 3.0f - worldPosition;
  const float centroidDistance = length(centroidDirection);
  directionToLight = centroidDistance > 1e-5f ? centroidDirection / centroidDistance : emissiveTriangle.NormalAndTwoSided.xyz;
  if (min(l0, min(l1, l2)) <= 1e-5f)
    return 0.0f;

  const float emitterCosine = dot(emissiveTriangle.NormalAndTwoSided.xyz, -directionToLight);
  if (emissiveTriangle.NormalAndTwoSided.w < 0.5f && emitterCosine <= 0.0f)
    return 0.0f;

  const float3 a = d0 / l0;
  const float3 b = d1 / l1;
  const float3 c = d2 / l2;
  const float numerator = abs(dot(a, cross(b, c)));
  const float denominator = 1.0f + dot(a, b) + dot(b, c) + dot(c, a);
  return 2.0f * atan2(numerator, denominator);
}

float ReSTIREstimateEmissiveTarget(xiiGpuEmissiveTriangle emissiveTriangle, float3 worldPosition, float3 normal)
{
  float3 directionToLight;
  const float solidAngle = ReSTIREmissiveSolidAngle(emissiveTriangle, worldPosition, directionToLight);
  return max(emissiveTriangle.RadianceAndLuminance.w * solidAngle * saturate(dot(normal, directionToLight)), 0.0f);
}

float ReSTIREstimateSampleTarget(uint sampleIndex, float3 worldPosition, float3 normal)
{
  if ((sampleIndex & XII_RESTIR_EMISSIVE_BIT) != 0u)
  {
    const uint triangleIndex = sampleIndex & XII_RESTIR_SAMPLE_MASK;
    return triangleIndex < ReSTIRGetEmissiveTriangleCount() ? ReSTIREstimateEmissiveTarget(g_EmissiveTriangles[triangleIndex], worldPosition, normal) : 0.0f;
  }
  return sampleIndex < g_ActiveLightCount ? ReSTIREstimateTarget(g_Lights[sampleIndex], worldPosition, normal) : 0.0f;
}

void ReSTIRFinalizeReservoir(inout xiiReSTIRDIReservoir reservoir, float selectedTarget)
{
  if (reservoir.SampleIndex == XII_RESTIR_INVALID_LIGHT || reservoir.M == 0u || !ReSTIRIsFinitePositive(selectedTarget))
  {
    reservoir = ReSTIREmptyReservoir();
    return;
  }

  reservoir.WeightSum = reservoir.WeightSum / ((float)reservoir.M * selectedTarget);
  if (!ReSTIRIsFinitePositive(reservoir.WeightSum))
    reservoir = ReSTIREmptyReservoir();
}
