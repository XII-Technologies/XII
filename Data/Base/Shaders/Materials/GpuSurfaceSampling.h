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

float4 xiiSampleVirtualTexture(uint index, uint samplerIndex, SamplerState fallbackSampler, float2 uv, float level)
{
  uint  table    = NonUniformResourceIndex(index & 0x00ffffffu);
  uint4 header   = g_VirtualTextureTables[table].Load4(0u);
  uint  feedback = g_VirtualTextureTables[table].Load(160u + ((index >> 24u) & 0x7fu) * 4u);
  uint  wanted   = (uint)clamp(floor(level), 0.0f, (float)header.z - 1.0f);
  uint  address  = g_VirtualTextureTables[table].Load(144u);
  uint2 modes    = uint2(address & 3u, (address >> 2u) & 3u);
  uv             = float2(modes.x == 1u ? saturate(uv.x) : modes.x == 2u ? 1.0f - abs(frac(uv.x * 0.5f) * 2.0f - 1.0f) :
                                                                           frac(uv.x),
                          modes.y == 1u ? saturate(uv.y) : modes.y == 2u ? 1.0f - abs(frac(uv.y * 0.5f) * 2.0f - 1.0f) :
                                                                           frac(uv.y));
  for (uint mip = wanted; mip < header.z; ++mip)
  {
    uint2  dimensions = max(uint2(1u, 1u), header.xy >> mip);
    uint2  grid       = (dimensions + 127u) / 128u;
    float2 texel      = min(uv * dimensions, dimensions - 0.0001f);
    uint2  tile       = min((uint2)texel / 128u, grid - 1u);
    uint   first      = g_VirtualTextureTables[table].Load(16u + mip * 4u);
    uint   page       = first + tile.y * grid.x + tile.x;
    uint2  mapping    = g_VirtualTextureTables[table].Load2(416u + page * 8u);
    // Resident demand remains in the feedback stream so visible pages stay warm.
    xiiRecordPageDemand(feedback, mapping.y);
    if (mapping.x != 0xffffffffu)
    {
      float2 physicalUV = (texel - tile * 128u + 2.0f) / 132.0f;
      if (samplerIndex != 0xffffffffu)
        return g_VirtualTextureTiles[NonUniformResourceIndex(mapping.x)].SampleLevel(g_MaterialSamplers[NonUniformResourceIndex(samplerIndex)], physicalUV, 0.0f);
      return g_VirtualTextureTiles[NonUniformResourceIndex(mapping.x)].SampleLevel(fallbackSampler, physicalUV, 0.0f);
    }
  }
  return float4(1.0f, 0.0f, 1.0f, 1.0f);
}

float4 xiiSampleSurfaceTexture(uint textureIndex, uint samplerIndex, SamplerState fallbackSampler, float2 uv)
{
  if ((textureIndex & 0x80000000u) != 0u)
  {
    uint4  dimensions = xiiSurfaceTextureDimensions(textureIndex);
    float2 dx = ddx(uv) * dimensions.xy, dy = ddy(uv) * dimensions.xy;
    float  level = 0.5f * log2(max(max(dot(dx, dx), dot(dy, dy)), 1.0f));
    return xiiSampleVirtualTexture(textureIndex, samplerIndex, fallbackSampler, uv, level);
  }
  if (samplerIndex != 0xFFFFFFFFu)
    return g_VirtualTextureTiles[NonUniformResourceIndex(textureIndex)].Sample(g_MaterialSamplers[NonUniformResourceIndex(samplerIndex)], uv);
  return g_VirtualTextureTiles[NonUniformResourceIndex(textureIndex)].Sample(fallbackSampler, uv);
}

float4 xiiSampleSurfaceTextureLevel(uint textureIndex, uint samplerIndex, SamplerState fallbackSampler, float2 uv, float level)
{
  if ((textureIndex & 0x80000000u) != 0u)
    return xiiSampleVirtualTexture(textureIndex, samplerIndex, fallbackSampler, uv, level);
  if (samplerIndex != 0xFFFFFFFFu)
    return g_VirtualTextureTiles[NonUniformResourceIndex(textureIndex)].SampleLevel(g_MaterialSamplers[NonUniformResourceIndex(samplerIndex)], uv, level);
  return g_VirtualTextureTiles[NonUniformResourceIndex(textureIndex)].SampleLevel(fallbackSampler, uv, level);
}
