/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Pipeline/Passes/ReSTIR/EmissiveLightData.h>

StructuredBuffer<xiiGpuEmissiveTriangle>    g_EmissiveTriangles;
StructuredBuffer<xiiGpuEmissiveLightHeader> g_EmissiveHeader;
#include <Shaders/Pipeline/Passes/ReSTIR/EmissiveLightSampling.h>

bool IrradianceHasEmissiveProposal()
{
  return EmissiveSamples.x != 0u && xiiHasEmissiveAliasDistribution() &&
    g_EmissiveHeader[0].FrameIndex == EmissiveSamples.y;
}

float IrradianceEmitterProbability(uint index)
{
  // Full-domain support also covers power lost to floating-point rounding.
  return 0.95f * xiiGetEmissiveTriangleProbability(index) + 0.05f / float(xiiGetEmissiveTriangleCount());
}

float IrradianceEmitterSolidAnglePDF(uint index, float distanceSquared, float3 direction)
{
  const xiiGpuEmissiveTriangle emitter       = g_EmissiveTriangles[index];
  const float                  projectedArea = emitter.Position0AndArea.w * abs(dot(emitter.NormalAndTwoSided.xyz, -direction));
  const float                  result        = projectedArea > 1e-12f ? IrradianceEmitterProbability(index) * distanceSquared / projectedArea : 0;
  return isfinite(result) ? result : 0;
}

float IrradianceEmitterHitWeight(xiiRayTracingGeometryData geometry, uint primitive, float distance, float3 direction, float previousPDF)
{
  if (!IrradianceHasEmissiveProposal())
    return 1;
  const uint count = xiiGetEmissiveTriangleCount(), base = geometry.PrimitiveRange.y;
  if (base >= count || primitive >= count - base)
    return 1;
  const float lightPDF   = IrradianceEmitterSolidAnglePDF(base + primitive, distance * distance, direction);
  const float scaledBSDF = previousPDF / float(EmissiveSamples.x);
  return lightPDF > 0 ? scaledBSDF / (scaledBSDF + lightPDF) : 1;
}

// Returns emitted radiance divided by the solid-angle PDF and weighted against
// the paired cosine path sample. The caller applies its receiver cosine or BRDF.
bool IrradianceSampleEmitter(float3 position, float3 normal, float coneWidth, float coneSpread,
                             inout uint randomState, out float3 direction, out float3 weightedRadiance)
{
  direction        = 0;
  weightedRadiance = 0;
  if (!IrradianceHasEmissiveProposal())
    return false;
  uint index;
  if (IrradianceRandom(randomState) < 0.05f)
    index = min(uint(IrradianceRandom(randomState) * xiiGetEmissiveTriangleCount()), xiiGetEmissiveTriangleCount() - 1u);
  else
  {
    const float a = IrradianceRandom(randomState), b = IrradianceRandom(randomState);
    const float c = IrradianceRandom(randomState), d = IrradianceRandom(randomState);
    float       ignoredPDF;
    if (!xiiSampleEmissiveTriangleAlias(float4(a, b, c, d), index, ignoredPDF))
      return false;
  }
  const xiiGpuEmissiveTriangle emitter = g_EmissiveTriangles[index];
  const float                  radius = sqrt(IrradianceRandom(randomState)), second = IrradianceRandom(randomState);
  const float3                 barycentrics    = float3(1 - radius, radius * (1 - second), radius * second);
  const float3                 emitterPosition = emitter.Position0AndArea.xyz * barycentrics.x + emitter.Position1AndPowerWeight.xyz * barycentrics.y + emitter.Position2AndInvArea.xyz * barycentrics.z;
  const float3                 origin          = position + normal * RayDistances.y;
  const float3                 delta           = emitterPosition - origin;
  const float                  distance        = length(delta);
  if (!isfinite(distance) || distance <= 0.001f || distance > RayDistances.x)
    return false;
  direction            = delta / distance;
  const float cosine   = max(dot(normal, direction), 0);
  const float lightPDF = IrradianceEmitterSolidAnglePDF(index, distance * distance, direction);
  if (cosine <= 0 || lightPDF <= 0)
    return false;
  IrradiancePayload emissionQuery = (IrradiancePayload)0;
  emissionQuery.Radiance = 0;
  emissionQuery.Flags    = XII_IRRADIANCE_EMITTER_RAY;
  // These fields carry the exact target identity on a nonrecursive emission query.
  emissionQuery.Random      = emitter.Metadata.y;
  emissionQuery.Depth       = emitter.Metadata.z;
  emissionQuery.ConeWidth   = coneWidth;
  emissionQuery.ConeSpread  = coneSpread;
  emissionQuery.PreviousPDF = 0;
  RayDesc ray;
  ray.Origin    = origin;
  ray.Direction = direction;
  ray.TMin      = 0.001f;
  ray.TMax      = distance + max(0.0001f, distance * 0.00001f);
  TraceRay(g_Scene, 0, 0xff, 0, 1, 0, ray, emissionQuery);
  // Cancel the light PDF algebraically to avoid overflow in N * lightPDF
  // and underflow in the intermediate balanceWeight / lightPDF.
  weightedRadiance = emissionQuery.Radiance / (lightPDF + cosine / (3.14159265359f * float(EmissiveSamples.x)));
  return all(isfinite(weightedRadiance));
}
