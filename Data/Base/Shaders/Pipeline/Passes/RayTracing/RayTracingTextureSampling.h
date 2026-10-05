#pragma once

// Shared explicit-LOD material sampling for ray-tracing stages. Derivatives are
// unavailable in closest-hit shaders, so callers provide doubled world/UV
// triangle areas and the propagated world-space ray-cone width.
float xiiComputeRayTracingTextureMip(uint textureIndex, float worldTriangleArea,
                                     float uvTriangleArea, float rayConeWidth)
{
  const uint resourceIndex = NonUniformResourceIndex(textureIndex);
  uint       textureWidth;
  uint       textureHeight;
  uint       mipCount;
  g_RayTracingTextures[resourceIndex].GetDimensions(0u, textureWidth, textureHeight, mipCount);

  // Ratios of doubled triangle areas preserve UV-to-world texel density while
  // avoiding two unnecessary 0.5 factors.
  const float texelsPerWorldUnit = sqrt(max(uvTriangleArea * (float)(textureWidth * textureHeight) /
                                              max(worldTriangleArea, 1e-12f),
                                            1e-12f));
  return clamp(log2(max(rayConeWidth * texelsPerWorldUnit, 1.0f)),
               0.0f, (float)max(mipCount, 1u) - 1.0f);
}

float4 xiiSampleRayTracingMaterialTexture(uint textureIndex, float2 texCoord,
                                          float worldTriangleArea, float uvTriangleArea,
                                          float rayConeWidth)
{
  const uint  resourceIndex = NonUniformResourceIndex(textureIndex);
  const float mipLevel      = xiiComputeRayTracingTextureMip(textureIndex, worldTriangleArea,
                                                             uvTriangleArea, rayConeWidth);
  return g_RayTracingTextures[resourceIndex].SampleLevel(g_sAnisotropicWrap, texCoord, mipLevel);
}
