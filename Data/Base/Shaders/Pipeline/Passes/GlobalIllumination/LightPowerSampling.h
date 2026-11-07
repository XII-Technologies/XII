/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
// The including hit shader supplies RayDistances and IrradianceRandom().
StructuredBuffer<float> g_LightCDF;
StructuredBuffer<float> g_BlockCDF;
static const float      XII_LIGHT_UNIFORM_MIXTURE = 0.05f;

// Search the half-open CDF intervals, so zero-mass entries are never selected.
uint IrradianceFindBlock(float target, uint count)
{
  uint low = 0, high = count;
  [loop] while (low < high)
  {
    const uint middle = low + (high - low) / 2;
    if (g_BlockCDF[middle] <= target)
      low = middle + 1;
    else
      high = middle;
  }
  return min(low, count - 1);
}
uint IrradianceFindLight(float target, uint first, uint end)
{
  uint low = first, high = end;
  [loop] while (low < high)
  {
    const uint middle = low + (high - low) / 2;
    if (g_LightCDF[middle] <= target)
      low = middle + 1;
    else
      high = middle;
  }
  return min(low, end - 1);
}
float IrradiancePowerPDF(uint candidate, uint candidateCount, float total)
{
  const uint  block = candidate / 256, first = block * 256, end = min(first + 256, candidateCount);
  const float blockMass  = max(g_BlockCDF[block] - (block > 0 ? g_BlockCDF[block - 1] : 0), 0);
  const float localTotal = g_LightCDF[end - 1];
  const float localMass  = max(g_LightCDF[candidate] - (candidate > first ? g_LightCDF[candidate - 1] : 0), 0);
  // Use the actual stored interval widths, including their rounding, rather
  // than reconstructing a different probability from theoretical source power.
  return localTotal > 0 ? (blockMass / total) * (localMass / localTotal) : 0;
}
uint IrradianceSelectLight(float stratum, uint count, inout uint randomState, out float probability)
{
  uint candidate = min(uint(stratum * float(count)), count - 1);
  probability    = 1.0f / float(count);
  if (RayDistances.z < 0.5f || count == 1)
    return candidate;
  uint blockCount, stride;
  g_BlockCDF.GetDimensions(blockCount, stride);
  const float total = g_BlockCDF[blockCount - 1];
  if (total <= 0 || !isfinite(total))
    return candidate;
  if (IrradianceRandom(randomState) >= XII_LIGHT_UNIFORM_MIXTURE)
  {
    const uint  block = IrradianceFindBlock(min(stratum * total, asfloat(asuint(total) - 1u)), blockCount);
    const uint  first = block * 256, end = min(first + 256, count);
    const float localTotal = g_LightCDF[end - 1];
    candidate              = IrradianceFindLight(min(IrradianceRandom(randomState) * localTotal, asfloat(asuint(localTotal) - 1u)), first, end);
  }
  probability = XII_LIGHT_UNIFORM_MIXTURE / float(count) + (1 - XII_LIGHT_UNIFORM_MIXTURE) * IrradiancePowerPDF(candidate, count, total);
  return candidate;
}
