/// Copyright (c) Theophilus Eriata. All Rights Reserved.

// Shared reservoir operations for spatiotemporal indirect-radiance reuse.
#pragma once

static const uint XII_RESTIR_GI_MAX_M = 32U;

struct xiiReSTIRGIReservoir
{
  float4 Sample;
  float  WeightSum;
  uint   M;
};

uint ReSTIRGIHash(uint value)
{
  value ^= value >> 16U;
  value *= 0x7FEB352DU;
  value ^= value >> 15U;
  value *= 0x846CA68BU;
  value ^= value >> 16U;
  return value;
}

float ReSTIRGIRandom01(inout uint state)
{
  state = ReSTIRGIHash(state);
  return (float)(state & 0x00FFFFFFU) * (1.0f / 16777216.0f);
}

float ReSTIRGITarget(float3 radiance, float radianceClamp)
{
  const float luminance = dot(max(radiance, 0.0f), float3(0.2126f, 0.7152f, 0.0722f));
  return min(luminance, max(radianceClamp, 1.0f));
}

bool ReSTIRGIIsFinitePositive(float value)
{
  return value > 0.0f && value < 3.402823466e+38f;
}

xiiReSTIRGIReservoir ReSTIRGIEmptyReservoir()
{
  xiiReSTIRGIReservoir reservoir;
  reservoir.Sample    = 0.0f;
  reservoir.WeightSum = 0.0f;
  reservoir.M         = 0U;
  return reservoir;
}

bool ReSTIRGIValidateReservoir(xiiReSTIRGIReservoir reservoir, float radianceClamp)
{
  return reservoir.Sample.a > 0.0f && reservoir.M > 0U && reservoir.M <= XII_RESTIR_GI_MAX_M &&
         ReSTIRGIIsFinitePositive(reservoir.WeightSum) && ReSTIRGIIsFinitePositive(ReSTIRGITarget(reservoir.Sample.rgb, radianceClamp));
}

xiiReSTIRGIReservoir ReSTIRGIUnpackReservoir(float4 sample, float2 state)
{
  xiiReSTIRGIReservoir reservoir;
  reservoir.Sample    = sample;
  reservoir.WeightSum = state.x;
  reservoir.M         = (uint)(state.y + 0.5f);
  return reservoir;
}

void ReSTIRGIUpdateReservoir(inout xiiReSTIRGIReservoir reservoir, float4 candidateSample, float candidateWeight, uint candidateM, inout uint randomState)
{
  candidateM = min(candidateM, XII_RESTIR_GI_MAX_M - reservoir.M);
  if (candidateM == 0U)
    return;

  reservoir.M += candidateM;
  if (!ReSTIRGIIsFinitePositive(candidateWeight))
    return;

  reservoir.WeightSum += candidateWeight;
  if (ReSTIRGIRandom01(randomState) * reservoir.WeightSum <= candidateWeight)
    reservoir.Sample = candidateSample;
}

void ReSTIRGIFinalizeReservoir(inout xiiReSTIRGIReservoir reservoir, float radianceClamp)
{
  const float selectedTarget = ReSTIRGITarget(reservoir.Sample.rgb, radianceClamp);
  if (reservoir.M == 0U || reservoir.Sample.a <= 0.0f || !ReSTIRGIIsFinitePositive(selectedTarget))
  {
    reservoir = ReSTIRGIEmptyReservoir();
    return;
  }

  reservoir.WeightSum /= (float)reservoir.M * selectedTarget;
  if (!ReSTIRGIIsFinitePositive(reservoir.WeightSum))
    reservoir = ReSTIRGIEmptyReservoir();
}

bool ReSTIRGISurfaceMatches(float4 previousSurface, float linearDepth, float3 normal, float albedoLuminance)
{
  if (previousSurface.z <= 0.0f)
    return false;

  const float3 previousNormal = DecodeOctNormal(previousSurface.xy);
  return abs(previousSurface.z - linearDepth) <= max(linearDepth * 0.08f, 0.02f) &&
         dot(previousNormal, normal) >= 0.85f &&
         abs(previousSurface.w - albedoLuminance) <= 0.20f;
}
