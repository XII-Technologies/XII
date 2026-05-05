// XII Engine - GPU lighting data shared by clustered lighting passes.
#pragma once

static const uint XII_LIGHT_TYPE_DIRECTIONAL = 0u;
static const uint XII_LIGHT_TYPE_POINT       = 1u;
static const uint XII_LIGHT_TYPE_SPOT        = 2u;
static const uint XII_LIGHT_TYPE_RECTANGLE   = 3u;
static const uint XII_LIGHT_TYPE_DISC        = 4u;

struct xiiGpuLightData
{
    float4 PositionAndInvRange;      // xyz = position, w = 1 / range
    float4 DirectionAndType;         // xyz = direction, w = light type
    float4 ColorAndIntensity;        // rgb = color, w = intensity
    float4 AttenuationAndSize;       // x = range, y = source radius, z = tube length
    float4 SpotAnglesAndRectSize;    // x = cos inner, y = cos outer, zw = rectangle extents
    float4 ShadowData;               // x = casts shadow, y = shadow fade, z = angular/source size
    float4 BoundsCenterAndRadius;    // xyz = culling sphere center, w = radius
    float4 UserData;
};

struct xiiClusterDescriptor
{
    float4 MinBounds;
    float4 MaxBounds;
};

struct xiiLocalShadowAtlasData
{
    float4 ScaleBias; // xy = tile scale, zw = tile bias
    float4 Meta;      // x = base tile, y = face count, z = tile size, w = light type
};

uint GetLightType(xiiGpuLightData lightData)
{
    return (uint)(lightData.DirectionAndType.w + 0.5f);
}
