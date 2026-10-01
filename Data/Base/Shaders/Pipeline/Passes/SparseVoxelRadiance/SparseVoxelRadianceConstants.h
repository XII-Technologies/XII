/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiSparseVoxelRadianceConstants)
{
  UINT4(ClipmapAndUpdateCounts); ///< x = levels, y = brick grid resolution, z = voxels per brick axis, w = update count.
  UINT4(PoolLayout);             ///< x = voxels per brick, y = maximum resident bricks, z = frame index, w = unused.
  FLOAT4(RadianceSettings);      ///< x = temporal hysteresis, yzw = reserved.
};
