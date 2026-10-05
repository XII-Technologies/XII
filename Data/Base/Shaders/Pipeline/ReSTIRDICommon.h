/// Copyright (c) Theophilus Eriata. All Rights Reserved.

// Shared ReSTIR direct-illumination reservoir helpers.
#pragma once

#include "LightingData.h"
#include "PhotometricLightSampling.h"
#include <Shaders/Pipeline/SurfaceLighting.h>
#include <Shaders/Pipeline/Passes/ReSTIR/EmissiveLightData.h>

StructuredBuffer<xiiGpuEmissiveTriangle>    g_EmissiveTriangles;
StructuredBuffer<xiiGpuEmissiveLightHeader> g_EmissiveHeader;

static const uint XII_RESTIR_INVALID_LIGHT = 0xFFFFFFFFu;
static const uint XII_RESTIR_EMISSIVE_BIT  = 0x80000000u;
static const uint XII_RESTIR_SAMPLE_MASK   = 0x7FFFFFFFu;
static const uint XII_RESTIR_MAX_M         = 32u;
static const uint XII_RESTIR_M_BITS        = 6u;
static const uint XII_RESTIR_M_MASK        = (1u << XII_RESTIR_M_BITS) - 1u;
static const uint XII_RESTIR_REVISION_MASK = 0xFFFFu;
static const uint XII_RESTIR_REVISION_SHIFT = XII_RESTIR_M_BITS;
static const uint XII_RESTIR_VISIBILITY_SHIFT = XII_RESTIR_REVISION_SHIFT + 16u;
static const uint XII_RESTIR_VISIBILITY_MASK = 0xFFu;
static const uint XII_RESTIR_VISIBILITY_VALID_BIT = 1u << 30u;

struct xiiReSTIRDIReservoir
{
  uint  SampleIndex;
  uint  StableLightId;
  float WeightSum;
  uint  M;
  uint  SampleRevision;
  float Visibility;
  uint  VisibilityValid;
};

struct xiiReSTIRDISurface
{
  float3 Albedo;
  float  Roughness;
  float  Metallic;
  float  DielectricSpecular;
  float  Transmission;
};

xiiReSTIRDISurface ReSTIRMakeSurface(float3 albedo, float roughness, float metallic, float dielectricSpecular, float transmission)
{
  xiiReSTIRDISurface surface;
  surface.Albedo = max(albedo, 0.0f);
  surface.Roughness = clamp(roughness, 0.01f, 1.0f);
  surface.Metallic = saturate(metallic);
  surface.DielectricSpecular = saturate(dielectricSpecular);
  surface.Transmission = saturate(transmission);
  return surface;
}

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
  reservoir.SampleRevision = 0u;
  reservoir.Visibility     = 1.0f;
  reservoir.VisibilityValid = 0u;
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
    return triangleIndex < triangleCount &&
      g_EmissiveTriangles[triangleIndex].Metadata.x == reservoir.StableLightId &&
      (g_EmissiveTriangles[triangleIndex].Metadata.w >> 16u) == reservoir.SampleRevision;
  }

  return reservoir.SampleIndex < g_ActiveLightCount &&
    g_Lights[reservoir.SampleIndex].Metadata.x == reservoir.StableLightId &&
    (g_Lights[reservoir.SampleIndex].Metadata.w & XII_RESTIR_REVISION_MASK) == reservoir.SampleRevision;
}

xiiReSTIRDIReservoir ReSTIRUnpackReservoir(uint4 packedReservoir)
{
  xiiReSTIRDIReservoir reservoir;
  reservoir.SampleIndex   = packedReservoir.x;
  reservoir.StableLightId = packedReservoir.y;
  reservoir.WeightSum     = asfloat(packedReservoir.z);
  reservoir.M             = packedReservoir.w & XII_RESTIR_M_MASK;
  reservoir.SampleRevision = (packedReservoir.w >> XII_RESTIR_REVISION_SHIFT) & XII_RESTIR_REVISION_MASK;
  reservoir.Visibility     = (float)((packedReservoir.w >> XII_RESTIR_VISIBILITY_SHIFT) & XII_RESTIR_VISIBILITY_MASK) * (1.0f / 255.0f);
  reservoir.VisibilityValid = (packedReservoir.w & XII_RESTIR_VISIBILITY_VALID_BIT) != 0u ? 1u : 0u;
  return reservoir;
}

uint4 ReSTIRPackReservoir(xiiReSTIRDIReservoir reservoir)
{
  const uint quantizedVisibility = (uint)round(saturate(reservoir.Visibility) * 255.0f);
  const uint packedMetadata = (reservoir.M & XII_RESTIR_M_MASK) |
    ((reservoir.SampleRevision & XII_RESTIR_REVISION_MASK) << XII_RESTIR_REVISION_SHIFT) |
    ((quantizedVisibility & XII_RESTIR_VISIBILITY_MASK) << XII_RESTIR_VISIBILITY_SHIFT) |
    (reservoir.VisibilityValid != 0u ? XII_RESTIR_VISIBILITY_VALID_BIT : 0u);
  return uint4(reservoir.SampleIndex, reservoir.StableLightId, asuint(reservoir.WeightSum), packedMetadata);
}

void ReSTIRUpdateReservoir(inout xiiReSTIRDIReservoir reservoir, uint sampleIndex, uint stableLightId, uint sampleRevision,
                           float candidateVisibility, uint candidateVisibilityValid, float candidateWeight, uint candidateM, inout uint randomState)
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
    reservoir.SampleRevision = sampleRevision & XII_RESTIR_REVISION_MASK;
    reservoir.Visibility     = saturate(candidateVisibility);
    reservoir.VisibilityValid = candidateVisibilityValid != 0u ? 1u : 0u;
  }
}

float ReSTIRContributionLuminance(float3 contribution)
{
  const float luminance = dot(max(contribution, 0.0f), float3(0.2126f, 0.7152f, 0.0722f));
  return isfinite(luminance) ? max(luminance, 0.0f) : 0.0f;
}

float3 ReSTIRGetViewDirection(float3 worldPosition, float3 normal)
{
  const float3 toCamera = g_CameraPositionAndNearPlane.xyz - worldPosition;
  const float distanceToCamera = length(toCamera);
  return distanceToCamera > 1e-5f ? toCamera / distanceToCamera : normal;
}

// Scalar importance target used by candidate generation and final normalization. It evaluates
// the same unshadowed layered material response as final shading. Visibility remains excluded so
// reused samples can be re-evaluated at a new receiver before one final shadow test.
float ReSTIREstimateTarget(xiiGpuLightData lightData, float3 worldPosition, float3 normal, xiiReSTIRDISurface surface)
{
  const float3 viewDirection = ReSTIRGetViewDirection(worldPosition, normal);
  const float3 contribution = EvaluateLocalLightLayered(
    lightData, worldPosition, normal, viewDirection, surface.Albedo, surface.Roughness, surface.Metallic,
    surface.DielectricSpecular, surface.Transmission, 0.0f, 0.1f);
  return ReSTIRContributionLuminance(contribution);
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

float ReSTIREstimateEmissiveTarget(xiiGpuEmissiveTriangle emissiveTriangle, float3 worldPosition, float3 normal, xiiReSTIRDISurface surface)
{
  float3 directionToLight;
  const float solidAngle = ReSTIREmissiveSolidAngle(emissiveTriangle, worldPosition, directionToLight);
  const float3 viewDirection = ReSTIRGetViewDirection(worldPosition, normal);
  const float3 contribution = BRDFLayered(
    surface.Albedo, surface.Roughness, surface.Metallic, surface.DielectricSpecular, surface.Transmission,
    0.0f, 0.1f, normal, directionToLight, viewDirection) * emissiveTriangle.RadianceAndLuminance.rgb * solidAngle;
  return ReSTIRContributionLuminance(contribution);
}

float ReSTIREstimateSampleTarget(uint sampleIndex, float3 worldPosition, float3 normal, xiiReSTIRDISurface surface)
{
  if ((sampleIndex & XII_RESTIR_EMISSIVE_BIT) != 0u)
  {
    const uint triangleIndex = sampleIndex & XII_RESTIR_SAMPLE_MASK;
    return triangleIndex < ReSTIRGetEmissiveTriangleCount() ? ReSTIREstimateEmissiveTarget(g_EmissiveTriangles[triangleIndex], worldPosition, normal, surface) : 0.0f;
  }
  return sampleIndex < g_ActiveLightCount ? ReSTIREstimateTarget(g_Lights[sampleIndex], worldPosition, normal, surface) : 0.0f;
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
