/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

#define XII_DDGI_DIRECTION_COUNT 6

DECLARE_CONSTANT_BUFFER_AUTO(xiiDDGIProbeConstants)
{
  INT4(MinimumCellAndAtlasWidth);    ///< xyz = logical minimum cell, w = probe grid width in the atlas.
  UINT4(ProbeCountsAndUpdateCount);  ///< xyz = probe dimensions, w = scheduled update count.
  FLOAT4(SpacingHysteresisDistance); ///< x = spacing, y = hysteresis, z = maximum trace distance, w = frame for GPU diagnostics.
};
