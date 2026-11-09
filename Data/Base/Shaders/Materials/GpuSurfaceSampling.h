/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once

SamplerState      g_MaterialSamplers[];
Texture2D<float4> g_VirtualTextureTiles[];
ByteAddressBuffer g_VirtualTextureTables[];
#include <Shaders/Streaming/GpuResidencyFeedback.h>

uint4 xiiSurfaceTextureDimensions(uint index)
{
  if ((index & 0x80000000u) != 0u)
    return g_VirtualTextureTables[NonUniformResourceIndex(index & 0x00ffffffu)].Load4(0u);
  uint width, height, mips;
  g_VirtualTextureTiles[NonUniformResourceIndex(index)].GetDimensions(0u, width, height, mips);
  return uint4(width, height, mips, 0u);
}

float xiiAddressVirtualTexture(float uv, uint mode)
{
  if (mode == 1u)
    return saturate(uv);
  if (mode == 2u)
    return 1.0f - abs(frac(uv * 0.5f) * 2.0f - 1.0f);
  return frac(uv);
}

uint2 xiiVirtualTextureMapping(uint table, uint mip, uint2 grid, uint2 tile)
{
  uint first = g_VirtualTextureTables[NonUniformResourceIndex(table)].Load(16u + mip * 4u);
  return g_VirtualTextureTables[NonUniformResourceIndex(table)].Load2(416u + (first + tile.y * grid.x + tile.x) * 8u);
}

float4 xiiSampleVirtualTexture(uint index, uint samplerIndex, SamplerState fallbackSampler, float2 uv, float level, float2 footprint)
{
  uint   table         = index & 0x00ffffffu;
  uint4  header        = g_VirtualTextureTables[NonUniformResourceIndex(table)].Load4(0u);
  uint   feedback      = g_VirtualTextureTables[NonUniformResourceIndex(table)].Load(160u + ((index >> 24u) & 0x7fu) * 4u);
  uint3  settings      = g_VirtualTextureTables[NonUniformResourceIndex(table)].Load3(144u);
  uint2  modes         = uint2(settings.x & 3u, (settings.x >> 2u) & 3u);
  float  mipLevel      = clamp(level + asfloat(settings.y), (float)min(settings.z, header.z - 1u), (float)header.z - 1.0f);
  uint   wanted        = (uint)floor(mipLevel);
  float2 addressed     = float2(xiiAddressVirtualTexture(uv.x, modes.x), xiiAddressVirtualTexture(uv.y, modes.y));
  float4 fineSample    = 0.0f;
  bool   hasFineSample = false;
  for (uint mip = wanted; mip < header.z; ++mip)
  {
    uint2  dimensions = max(uint2(1u, 1u), header.xy >> mip);
    uint2  grid       = (dimensions + 127u) / 128u;
    float2 texel      = min(addressed * dimensions, dimensions - 0.0001f);
    uint2  tile       = min((uint2)texel / 128u, grid - 1u);
    uint2  mapping    = xiiVirtualTextureMapping(table, mip, grid, tile);
    xiiRecordPageDemand(feedback, mapping.y);

    // Predict at most one adjacent page, and only when the footprint crosses a tile boundary.
    if (mip == wanted && any(grid > 1u) && any(footprint != 0.0f))
    {
      float2 ahead   = uv + clamp(footprint * dimensions * 2.0f, -32.0f, 32.0f) / dimensions;
      ahead          = float2(xiiAddressVirtualTexture(ahead.x, modes.x), xiiAddressVirtualTexture(ahead.y, modes.y));
      uint2 adjacent = min((uint2)(ahead * dimensions) / 128u, grid - 1u);
      if (any(adjacent != tile))
        xiiRecordPageDemand(feedback, xiiVirtualTextureMapping(table, mip, grid, adjacent).y);
    }

    if (mapping.x == 0xffffffffu && mip + 1u != header.z)
    {
      // Keep the fine sample when the next mip is missing. Its request is already recorded.
      if (hasFineSample)
        return fineSample;
      continue;
    }

    float4 value;
    if (mip + 1u == header.z)
    {
      value = float4(uint4(header.w, header.w >> 8u, header.w >> 16u, header.w >> 24u) & 255u) / 255.0f;
      if (g_VirtualTextureTables[NonUniformResourceIndex(table)].Load(156u) != 0u)
        value.rgb = lerp(value.rgb / 12.92f, pow((value.rgb + 0.055f) / 1.055f, 2.4f), step(0.04045f, value.rgb));
    }
    else
    {
      float2 physicalUV = (texel - tile * 128u + 2.0f) / 132.0f;
      uint   physical   = NonUniformResourceIndex(mapping.x);
      value             = samplerIndex != 0xffffffffu ?
                    g_VirtualTextureTiles[physical].SampleLevel(g_MaterialSamplers[NonUniformResourceIndex(samplerIndex)], physicalUV, 0.0f) :
                    g_VirtualTextureTiles[physical].SampleLevel(fallbackSampler, physicalUV, 0.0f);
    }
    if (hasFineSample)
      return lerp(fineSample, value, frac(mipLevel));
    if (mip != wanted || mip + 1u == header.z || frac(mipLevel) == 0.0f)
      return value;
    fineSample    = value;
    hasFineSample = true;
  }
  return float4(1.0f, 0.0f, 1.0f, 1.0f);
}

float4 xiiSampleVirtualTextureGrad(uint index, uint samplerIndex, SamplerState fallbackSampler, float2 uv, float2 dx, float2 dy, float bias)
{
  uint2 size  = xiiSurfaceTextureDimensions(index).xy;
  float dx2   = dot(dx * size, dx * size);
  float dy2   = dot(dy * size, dy * size);
  float level = 0.5f * log2(max(max(dx2, dy2), 1e-8f)) + bias;
  return xiiSampleVirtualTexture(index, samplerIndex, fallbackSampler, uv, level, dx2 >= dy2 ? dx : dy);
}

float4 xiiSampleSurfaceTexture(uint textureIndex, uint samplerIndex, SamplerState fallbackSampler, float2 uv)
{
  if ((textureIndex & 0x80000000u) != 0u)
  {
    return xiiSampleVirtualTextureGrad(textureIndex, samplerIndex, fallbackSampler, uv, ddx(uv), ddy(uv), 0.0f);
  }
  if (samplerIndex != 0xFFFFFFFFu)
    return g_VirtualTextureTiles[NonUniformResourceIndex(textureIndex)].Sample(g_MaterialSamplers[NonUniformResourceIndex(samplerIndex)], uv);
  return g_VirtualTextureTiles[NonUniformResourceIndex(textureIndex)].Sample(fallbackSampler, uv);
}

float4 xiiSampleSurfaceTextureLevel(uint textureIndex, uint samplerIndex, SamplerState fallbackSampler, float2 uv, float level)
{
  if ((textureIndex & 0x80000000u) != 0u)
    return xiiSampleVirtualTexture(textureIndex, samplerIndex, fallbackSampler, uv, level, 0.0f);
  if (samplerIndex != 0xFFFFFFFFu)
    return g_VirtualTextureTiles[NonUniformResourceIndex(textureIndex)].SampleLevel(g_MaterialSamplers[NonUniformResourceIndex(samplerIndex)], uv, level);
  return g_VirtualTextureTiles[NonUniformResourceIndex(textureIndex)].SampleLevel(fallbackSampler, uv, level);
}
