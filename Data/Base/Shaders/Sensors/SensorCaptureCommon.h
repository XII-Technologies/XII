/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Pipeline/PipelineCommon.xiiShader>
#include <Shaders/Pipeline/Passes/Sensors/SensorOutputConstants.h>
Texture2D<float> g_SceneDepth;
#if SENSOR_ROLLING_SHUTTER
Texture2D<float2> g_Velocity;
#endif
RWTexture2D<float4> g_SensorOutput;
static const uint NOISE_NONE = 0U;
static const uint NOISE_GAUSSIAN = 1U;
static const uint NOISE_PHOTON_SHOT_READ = 2U;
float HashToUnitFloat(uint value)
{
  value ^= value >> 16U;
  value *= 0x7feb352dU;
  value ^= value >> 15U;
  value *= 0x846ca68bU;
  value ^= value >> 16U;
  return (float(value) + 0.5f) * (1.0f / 4294967296.0f);
}

float GaussianNoise(uint2 pixel, uint stream)
{
  const uint seed = OutputDescription.w ^ (FrameAndResolution.x * 0x9e3779b9U) ^
                    (pixel.x * 0x85ebca6bU) ^ (pixel.y * 0xc2b2ae35U) ^ stream;
  const float u1 = max(HashToUnitFloat(seed), 1e-7f);
  const float u2 = HashToUnitFloat(seed ^ 0x68bc21ebU);
  return sqrt(-2.0f * log(u1)) * cos(6.28318530718f * u2);
}

float QuantizeNormalized(float value)
{
  const uint bits = min(OutputDescription.z, 24U);
  const float levels = float((1U << bits) - 1U);
  return round(saturate(value) * levels) / levels;
}

float AddRangeNoise(float rangeMeters, uint2 pixel)
{
  if (OutputDescription.y == NOISE_NONE)
    return rangeMeters;

  const float sigma = NoiseParameters.z + NoiseParameters.w * rangeMeters;
  return max(rangeMeters + GaussianNoise(pixel, 0x13a5ba1dU) * sigma, 0.0f);
}

float3 ConvertRadianceToDigital(float3 radiance, uint2 pixel)
{
  float3 electrons = max(radiance, 0.0f) * RangeAndExposure.z * SpectralSensitivityAndQE.w * SignalConversion.x;
  if (OutputDescription.y == NOISE_GAUSSIAN)
  {
    electrons += float3(GaussianNoise(pixel, 0x18U), GaussianNoise(pixel, 0x29U), GaussianNoise(pixel, 0x3aU)) * NoiseParameters.x;
  }
  else if (OutputDescription.y == NOISE_PHOTON_SHOT_READ)
  {
    const float3 sigma = sqrt(max(electrons, 0.0f)) * NoiseParameters.y + NoiseParameters.x;
    electrons += float3(GaussianNoise(pixel, 0x4bU), GaussianNoise(pixel, 0x5cU), GaussianNoise(pixel, 0x6dU)) * sigma;
  }

  const float3 normalized = electrons * SignalConversion.y / max(SignalConversion.z, 1.0f);
  return float3(QuantizeNormalized(normalized.x), QuantizeNormalized(normalized.y), QuantizeNormalized(normalized.z));
}

float2 UndistortSensorRay(float2 distortedRay)
{
  float2 ray = distortedRay;

  // Invert the Brown-Conrady model because output pixels are measured sensor
  // coordinates while the source render is an ideal pinhole image.
  [unroll]
  for (uint iteration = 0U; iteration < 5U; ++iteration)
  {
    const float r2 = dot(ray, ray);
    const float r4 = r2 * r2;
    const float r6 = r4 * r2;
    const float radial = 1.0f + RadialDistortion.x * r2 + RadialDistortion.y * r4 + RadialDistortion.z * r6;
    if (abs(radial) < 1e-5f)
      break;

    const float2 tangential = float2(
      2.0f * TangentialDistortion.x * ray.x * ray.y + TangentialDistortion.y * (r2 + 2.0f * ray.x * ray.x),
      TangentialDistortion.x * (r2 + 2.0f * ray.y * ray.y) + 2.0f * TangentialDistortion.y * ray.x * ray.y);
    ray = (distortedRay - tangential) / radial;
  }

  return ray;
}


bool ResolveSensorSample(uint2 pixel, out uint2 sourcePixel, out float2 sourceUV, out float3 sensorRay, out float axialDepth, out float radialRange)
{
  sourcePixel = 0; sourceUV = 0; sensorRay = 0; axialDepth = 0; radialRange = 0;
  uint2 outputDimensions, sourceDimensions;
  g_SensorOutput.GetDimensions(outputDimensions.x, outputDimensions.y);
  g_SceneDepth.GetDimensions(sourceDimensions.x, sourceDimensions.y);
  if (any(pixel >= outputDimensions) || any(sourceDimensions == 0)) return false;
  const float2 distortedRay = (float2(pixel) + 0.5f - Intrinsics.zw) / max(Intrinsics.xy, 1e-5f);
  const float2 calibratedRay = UndistortSensorRay(distortedRay);
  sensorRay = float3(calibratedRay, 1.0f);
  sourceUV = (calibratedRay * Intrinsics.xy + Intrinsics.zw) / float2(outputDimensions);
  if (any(sourceUV <= 0.0f) || any(sourceUV >= 1.0f)) return false;
#if SENSOR_ROLLING_SHUTTER
  const uint2 velocityPixel = min((uint2)(sourceUV * float2(sourceDimensions)), sourceDimensions - 1U);
  const float2 velocity = g_Velocity.Load(int3(velocityPixel, 0));
  const float readoutFrameFraction = saturate(RangeAndExposure.w / max(g_DeltaTimeMs * 0.001f, 1e-6f));
  const float scanlineHistory = 1.0f - (float(pixel.y) + 0.5f) / float(outputDimensions.y);
  sourceUV -= velocity * float2(0.5f, -0.5f) * (readoutFrameFraction * scanlineHistory);
#endif
  if (any(sourceUV <= 0.0f) || any(sourceUV >= 1.0f)) return false;
  sourcePixel = min((uint2)(sourceUV * float2(sourceDimensions)), sourceDimensions - 1U);
  const float depth = g_SceneDepth.Load(int3(sourcePixel, 0));
  if (depth <= 0.0f) return false;
  // Reconstruct with the supplied projection, including finite far planes.
  const float3 worldPosition = ReconstructWorldPos(sourceUV, depth);
  axialDepth = dot(worldPosition - g_CameraPositionAndNearPlane.xyz, normalize(g_CameraForwardAndFarPlane.xyz));
  radialRange = axialDepth * length(sensorRay);
  return isfinite(radialRange) && radialRange >= RangeAndExposure.x && radialRange <= RangeAndExposure.y;
}
