/// Copyright (c) Theophilus Eriata. All Rights Reserved.

// Shared two-level alias sampling for the GPU emissive-triangle database.
#pragma once

#include <Shaders/Pipeline/Passes/ReSTIR/EmissiveLightData.h>

StructuredBuffer<xiiGpuEmissiveAliasEntry>  g_EmissiveTriangleAliases;
StructuredBuffer<xiiGpuEmissiveAliasEntry>  g_EmissiveBlockAliases;
StructuredBuffer<xiiGpuEmissiveAliasHeader> g_EmissiveAliasHeader;

bool xiiEmissiveIsFinitePositive(float value)
{
  return value > 0.0f && value < 3.402823466e+38f;
}

uint xiiGetEmissiveTriangleCount()
{
  uint triangleCapacity, triangleStride;
  uint headerCapacity, headerStride;
  g_EmissiveTriangles.GetDimensions(triangleCapacity, triangleStride);
  g_EmissiveHeader.GetDimensions(headerCapacity, headerStride);
  return headerCapacity > 0u ? min(g_EmissiveHeader[0].TriangleCount, triangleCapacity) : 0u;
}

bool xiiHasEmissiveAliasDistribution()
{
  uint blockAliasCount, blockAliasStride;
  uint aliasHeaderCount, aliasHeaderStride;
  uint emissiveHeaderCount, emissiveHeaderStride;
  g_EmissiveBlockAliases.GetDimensions(blockAliasCount, blockAliasStride);
  g_EmissiveAliasHeader.GetDimensions(aliasHeaderCount, aliasHeaderStride);
  g_EmissiveHeader.GetDimensions(emissiveHeaderCount, emissiveHeaderStride);
  if (aliasHeaderCount == 0u || emissiveHeaderCount == 0u)
    return false;

  const xiiGpuEmissiveAliasHeader aliasHeader = g_EmissiveAliasHeader[0];
  return g_EmissiveHeader[0].OverflowCount == 0u && xiiGetEmissiveTriangleCount() > 0u &&
    aliasHeader.BlockCount > 0u && aliasHeader.BlockCount <= blockAliasCount &&
    aliasHeader.TrianglesPerBlock > 0u && xiiEmissiveIsFinitePositive(aliasHeader.TotalPower) &&
    aliasHeader.FrameIndex == g_EmissiveHeader[0].FrameIndex;
}

bool xiiSampleEmissiveTriangleAlias(float4 randomSample, out uint triangleIndex, out float proposalProbability)
{
  triangleIndex       = 0u;
  proposalProbability = 0.0f;

  uint triangleAliasCount, triangleAliasStride;
  uint blockAliasCount, blockAliasStride;
  uint aliasHeaderCount, aliasHeaderStride;
  uint emissiveHeaderCount, emissiveHeaderStride;
  g_EmissiveTriangleAliases.GetDimensions(triangleAliasCount, triangleAliasStride);
  g_EmissiveBlockAliases.GetDimensions(blockAliasCount, blockAliasStride);
  g_EmissiveAliasHeader.GetDimensions(aliasHeaderCount, aliasHeaderStride);
  g_EmissiveHeader.GetDimensions(emissiveHeaderCount, emissiveHeaderStride);
  if (aliasHeaderCount == 0u || emissiveHeaderCount == 0u)
    return false;

  const xiiGpuEmissiveAliasHeader aliasHeader   = g_EmissiveAliasHeader[0];
  const uint                      triangleCount = xiiGetEmissiveTriangleCount();
  if (aliasHeader.BlockCount == 0u || aliasHeader.BlockCount > blockAliasCount || aliasHeader.TrianglesPerBlock == 0u ||
      !xiiEmissiveIsFinitePositive(aliasHeader.TotalPower) || aliasHeader.FrameIndex != g_EmissiveHeader[0].FrameIndex)
    return false;

  const uint                     blockColumn = min((uint)(saturate(randomSample.x) * (float)aliasHeader.BlockCount), aliasHeader.BlockCount - 1u);
  const xiiGpuEmissiveAliasEntry blockEntry  = g_EmissiveBlockAliases[blockColumn];
  const uint                     blockIndex  = randomSample.y < blockEntry.Probability ? blockColumn : blockEntry.AliasIndex;
  if (blockIndex >= aliasHeader.BlockCount)
    return false;

  const uint firstTriangle = blockIndex * aliasHeader.TrianglesPerBlock;
  const uint localCount    = firstTriangle < triangleCount ? min(aliasHeader.TrianglesPerBlock, triangleCount - firstTriangle) : 0u;
  if (localCount == 0u || firstTriangle + localCount > triangleAliasCount)
    return false;

  const uint                     triangleColumn = firstTriangle + min((uint)(saturate(randomSample.z) * (float)localCount), localCount - 1u);
  const xiiGpuEmissiveAliasEntry triangleEntry  = g_EmissiveTriangleAliases[triangleColumn];
  triangleIndex                                 = randomSample.w < triangleEntry.Probability ? triangleColumn : triangleEntry.AliasIndex;
  if (triangleIndex >= triangleCount)
    return false;

  proposalProbability = max(g_EmissiveTriangles[triangleIndex].Position1AndPowerWeight.w, 0.0f) / aliasHeader.TotalPower;
  return xiiEmissiveIsFinitePositive(proposalProbability);
}
