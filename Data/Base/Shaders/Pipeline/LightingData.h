/// Copyright (c) Theophilus Eriata. All Rights Reserved.

// XII Engine - GPU lighting data shared by clustered lighting passes.
#pragma once

static const uint XII_LIGHT_TYPE_DIRECTIONAL = 0u;
static const uint XII_LIGHT_TYPE_POINT       = 1u;
static const uint XII_LIGHT_TYPE_SPOT        = 2u;
static const uint XII_LIGHT_TYPE_RECTANGLE   = 3u;
static const uint XII_LIGHT_TYPE_DISC        = 4u;
static const uint XII_LIGHT_TYPE_SPHERE        = 5u;
static const uint XII_LIGHT_TYPE_TUBE          = 6u;
static const uint XII_LIGHT_TYPE_EMISSIVE_MESH = 7u;

struct xiiGpuLightData
{
  float4 PositionAndInvRange;   // xyz = position, w = 1 / range
  float4 DirectionAndType;      // xyz = direction, w = light type
  float4 ColorAndIntensity;     // rgb = normalized color, w = cd (local), lx (directional), or nt (area)
  float4 AttenuationAndSize;    // x = range, y = source radius, z = tube length
  float4 SpotAnglesAndRectSize; // x = cos inner, y = cos outer, zw = rectangle extents
  float4 ShadowData;            // x = casts shadow, y = camera fade-out distance in metres (0 = automatic/full), z = angular/source size
  float4 BoundsCenterAndRadius; // xyz = culling sphere center, w = radius
  float4 OrientationRightAndIES; // xyz = local right axis, w = compact IES profile index + 1
  uint4  Metadata;               // x = stable light ID, y = compact frame index, z = light type, w = flags
};

struct xiiClusterDescriptor
{
  float4 MinBounds;
  float4 MaxBounds;
};

struct xiiLocalShadowAtlasData
{
  float4x4 ViewProjection[6];
  float4   AtlasScaleBias[6]; // xy = tile scale, zw = tile bias for each face
  float4   LightPositionAndInvRange;
  uint4    Metadata; // x = face count, y = tile size, z = valid, w = light type
};

uint GetLightType(xiiGpuLightData lightData)
{
  return (uint)(lightData.DirectionAndType.w + 0.5f);
}

/// Returns the stable per-light shadow contribution for this camera. An authored zero keeps full
/// shadow coverage (the component's "Auto" setting); positive values fade over the final 20% of
/// the camera-to-light distance budget instead of being misinterpreted as a unitless strength.
float GetLocalShadowFadeStrength(xiiGpuLightData lightData)
{
  const float fadeEnd = lightData.ShadowData.y;
  if (fadeEnd <= 0.0f)
    return 1.0f;

  const float cameraDistance = length(g_CameraPositionAndNearPlane.xyz - lightData.PositionAndInvRange.xyz);
  const float fadeWidth = max(fadeEnd * 0.2f, 0.01f);
  return 1.0f - smoothstep(max(fadeEnd - fadeWidth, 0.0f), fadeEnd, cameraDistance);
}

float3 GetRepresentativeLightPosition(xiiGpuLightData lightData, float3 worldPosition)
{
  if (GetLightType(lightData) != XII_LIGHT_TYPE_TUBE)
    return lightData.PositionAndInvRange.xyz;

  const float3 axis       = normalize(lightData.DirectionAndType.xyz);
  const float  halfLength = 0.5f * lightData.AttenuationAndSize.z;
  const float  offset     = clamp(dot(worldPosition - lightData.PositionAndInvRange.xyz, axis), -halfLength, halfLength);
  return lightData.PositionAndInvRange.xyz + axis * offset;
}

/// Converts area-emitter luminance (nits) to on-axis luminous intensity by
/// evaluating its projected area toward the shaded point.
float GetPhotometricEmitterScale(xiiGpuLightData lightData, float3 pointToLightDirection)
{
  const uint lightType = GetLightType(lightData);

  if (lightType == XII_LIGHT_TYPE_RECTANGLE)
  {
    const float facing = saturate(dot(-pointToLightDirection, normalize(lightData.DirectionAndType.xyz)));
    return facing * max(lightData.SpotAnglesAndRectSize.z * lightData.SpotAnglesAndRectSize.w, 0.0f);
  }

  if (lightType == XII_LIGHT_TYPE_DISC)
  {
    const float radius = max(lightData.AttenuationAndSize.y, 0.0f);
    const float facing = saturate(dot(-pointToLightDirection, normalize(lightData.DirectionAndType.xyz)));
    return facing * 3.14159265f * radius * radius;
  }

  if (lightType == XII_LIGHT_TYPE_SPHERE || lightType == XII_LIGHT_TYPE_EMISSIVE_MESH)
  {
    const float radius = max(lightData.AttenuationAndSize.y, 0.0f);
    return 3.14159265f * radius * radius;
  }

  if (lightType == XII_LIGHT_TYPE_TUBE)
  {
    const float radius     = max(lightData.AttenuationAndSize.y, 0.0f);
    const float length     = max(lightData.AttenuationAndSize.z, 0.0f);
    const float axisCosine = abs(dot(pointToLightDirection, normalize(lightData.DirectionAndType.xyz)));
    const float sideOn     = sqrt(saturate(1.0f - axisCosine * axisCosine));
    return 2.0f * radius * length * sideOn + 3.14159265f * radius * radius;
  }

  return 1.0f;
}
