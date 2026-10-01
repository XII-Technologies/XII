/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiDDGIProbeConstants)
{
  INT4(MinimumCellAndAtlasWidth);   ///< xyz = logical minimum cell, w = atlas width.
  UINT4(ProbeCountsAndUpdateCount); ///< xyz = probe dimensions, w = scheduled update count.
  FLOAT4(SpacingHysteresisDistance); ///< x = spacing, y = hysteresis, z = maximum trace distance, w = unused.
};
