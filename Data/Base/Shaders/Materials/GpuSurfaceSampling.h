/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

// Shared descriptor table for authored texture samplers. Explicit descriptor-only
// material bindings retain each consumer's legacy sampler through the invalid index.
SamplerState g_MaterialSamplers[];

float4 xiiSampleSurfaceTexture(Texture2D<float4> texture, uint samplerIndex, SamplerState fallbackSampler, float2 uv)
{
  if (samplerIndex != 0xFFFFFFFFu)
    return texture.Sample(g_MaterialSamplers[NonUniformResourceIndex(samplerIndex)], uv);
  return texture.Sample(fallbackSampler, uv);
}

float4 xiiSampleSurfaceTextureLevel(Texture2D<float4> texture, uint samplerIndex, SamplerState fallbackSampler, float2 uv, float level)
{
  if (samplerIndex != 0xFFFFFFFFu)
    return texture.SampleLevel(g_MaterialSamplers[NonUniformResourceIndex(samplerIndex)], uv, level);
  return texture.SampleLevel(fallbackSampler, uv, level);
}
