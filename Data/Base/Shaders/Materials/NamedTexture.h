/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Materials/GpuSurfaceSampling.h>

/// Material-compiler lowering preserves named sampling syntax while residency is
/// resolved through the same bindless page tables as GPU scene materials.
struct xiiMaterialTexture2D
{
  uint   Index;
  float4 Sample(SamplerState samplerState, float2 uv)
  {
    return Index == 0xffffffffu ? 1.0f : xiiSampleSurfaceTexture(Index, 0xffffffffu, samplerState, uv);
  }
  float4 SampleLevel(SamplerState samplerState, float2 uv, float level)
  {
    return Index == 0xffffffffu ? 1.0f : xiiSampleSurfaceTextureLevel(Index, 0xffffffffu, samplerState, uv, level);
  }
  float4 SampleBias(SamplerState samplerState, float2 uv, float bias)
  {
    if (Index == 0xffffffffu)
      return 1.0f;
    uint4  size = xiiSurfaceTextureDimensions(Index);
    float2 dx = ddx(uv) * size.xy, dy = ddy(uv) * size.xy;
    float  level = 0.5f * log2(max(max(dot(dx, dx), dot(dy, dy)), 1.0f)) + bias;
    return xiiSampleSurfaceTextureLevel(Index, 0xffffffffu, samplerState, uv, level);
  }
  float4 SampleGrad(SamplerState samplerState, float2 uv, float2 dx, float2 dy)
  {
    if (Index == 0xffffffffu)
      return 1.0f;
    if ((Index & 0x80000000u) == 0u)
      return g_VirtualTextureTiles[NonUniformResourceIndex(Index)].SampleGrad(samplerState, uv, dx, dy);
    uint4 size = xiiSurfaceTextureDimensions(Index);
    dx *= size.xy;
    dy *= size.xy;
    float level = 0.5f * log2(max(max(dot(dx, dx), dot(dy, dy)), 1.0f));
    return xiiSampleSurfaceTextureLevel(Index, 0xffffffffu, samplerState, uv, level);
  }
  void GetDimensions(out uint width, out uint height)
  {
    uint4 size = Index == 0xffffffffu ? uint4(1u, 1u, 1u, 0u) : xiiSurfaceTextureDimensions(Index);
    width      = size.x;
    height     = size.y;
  }
  void GetDimensions(uint mip, out uint width, out uint height, out uint levels)
  {
    uint4 size = Index == 0xffffffffu ? uint4(1u, 1u, 1u, 0u) : xiiSurfaceTextureDimensions(Index);
    width      = max(1u, size.x >> mip);
    height     = max(1u, size.y >> mip);
    levels     = size.z;
  }
};

xiiMaterialTexture2D xiiMakeMaterialTexture(uint index)
{
  xiiMaterialTexture2D texture;
  texture.Index = index;
  return texture;
}
