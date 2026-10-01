/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Pipeline/Passes/VirtualShadowMap/VirtualShadowMapConstants.h>

struct xiiGpuVirtualShadowPage
{
  uint VirtualKeyLow;
  uint VirtualKeyHigh;
  uint PhysicalPage;
  uint Flags;
};

StructuredBuffer<xiiGpuVirtualShadowPage> g_VirtualShadowPageTable;
Texture2D<float>                          g_VirtualShadowAtlas;

uint HashVirtualShadowPageKey(uint keyLow, uint keyHigh)
{
  uint hash = keyLow ^ (keyHigh * 0x9E3779B9u);
  hash ^= hash >> 16u;
  hash *= 0x7FEB352Du;
  hash ^= hash >> 15u;
  hash *= 0x846CA68Bu;
  hash ^= hash >> 16u;
  return hash;
}

uint2 PackVirtualShadowPageKey(uint lightId, uint mipLevel, uint2 page)
{
  return uint2(
    (lightId & 0x00FFFFFFu) | ((mipLevel & 0x3Fu) << 24u) | ((page.x & 0x3u) << 30u),
    ((page.x >> 2u) & 0x7FFFu) | ((page.y & 0x1FFFFu) << 15u));
}

/// Performs a bounded open-addressed lookup. A miss is always safe because
/// callers retain their conventional shadow-map fallback.
bool TryResolveVirtualShadowPage(uint2 key, out xiiGpuVirtualShadowPage page)
{
  page = (xiiGpuVirtualShadowPage)0;
  if (VirtualShadowEnabled == 0u || VirtualShadowPageTableCapacity == 0u)
    return false;

  const uint tableMask  = VirtualShadowPageTableCapacity - 1u;
  uint       bucket     = HashVirtualShadowPageKey(key.x, key.y) & tableMask;
  const uint probeCount = min(VirtualShadowPageTableCapacity, 32u);

  [loop] for (uint probe = 0u; probe < probeCount; ++probe)
  {
    const xiiGpuVirtualShadowPage candidate = g_VirtualShadowPageTable[VirtualShadowPageTableBaseIndex + bucket];
    if ((candidate.Flags & 1u) == 0u)
      return false;
    if (candidate.VirtualKeyLow == key.x && candidate.VirtualKeyHigh == key.y)
    {
      page = candidate;
      return (candidate.Flags & 2u) == 0u;
    }
    bucket = (bucket + 1u) & tableMask;
  }
  return false;
}

/// Samples a resident and fully-rendered directional page. Dirty or absent
/// pages return false so the caller can use the cascaded shadow-map fallback.
bool TrySampleVirtualDirectionalShadow(uint mipLevel, float2 shadowUV, float receiverDepth, out float visibility)
{
  visibility = 1.0f;
  if (VirtualShadowResolution == 0u || VirtualShadowPageSize == 0u || VirtualShadowPhysicalAtlasWidth == 0u || VirtualShadowPhysicalAtlasHeight == 0u)
    return false;

  const uint   basePages           = max(VirtualShadowResolution / VirtualShadowPageSize, 1u);
  const uint   pagesPerAxis        = max(basePages >> mipLevel, 1u);
  const float2 virtualPagePosition = saturate(shadowUV) * float(pagesPerAxis);
  const uint2  virtualPage         = min((uint2)virtualPagePosition, pagesPerAxis - 1u);

  xiiGpuVirtualShadowPage mapping;
  if (!TryResolveVirtualShadowPage(PackVirtualShadowPageKey(VirtualShadowDirectionalLightId, mipLevel, virtualPage), mapping))
    return false;

  const uint   physicalPagesPerRow = max(VirtualShadowPhysicalAtlasWidth / VirtualShadowPageSize, 1u);
  const uint2  physicalPage        = uint2(mapping.PhysicalPage % physicalPagesPerRow, mapping.PhysicalPage / physicalPagesPerRow);
  const float2 pageMinimum         = float2(physicalPage * VirtualShadowPageSize) + 1.5f;
  const float2 pageMaximum         = float2((physicalPage + 1u) * VirtualShadowPageSize) - 1.5f;
  const float2 pageTexel           = frac(virtualPagePosition) * float(VirtualShadowPageSize);
  const float2 atlasPixel          = clamp(float2(physicalPage * VirtualShadowPageSize) + pageTexel, pageMinimum, pageMaximum);
  const float2 inverseAtlasSize    = rcp(float2(VirtualShadowPhysicalAtlasWidth, VirtualShadowPhysicalAtlasHeight));

  visibility = 0.0f;
  [unroll] for (int y = -1; y <= 1; ++y)
  {
    [unroll] for (int x = -1; x <= 1; ++x)
    {
      const float2 samplePixel = clamp(atlasPixel + float2(x, y), pageMinimum, pageMaximum);
      const float  mapDepth    = g_VirtualShadowAtlas.SampleLevel(PointClampSampler, samplePixel * inverseAtlasSize, 0.0f);
      visibility += receiverDepth >= mapDepth ? 1.0f : 0.0f;
    }
  }
  visibility *= 1.0f / 9.0f;
  return true;
}
