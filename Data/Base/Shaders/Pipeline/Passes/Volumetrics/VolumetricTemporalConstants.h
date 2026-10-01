/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Camera history and rejection controls for volumetric temporal accumulation.
/// Sky pixels use the previous view-projection matrix because they have no
/// surface motion vector; opaque pixels continue to use geometry velocity.
DECLARE_CONSTANT_BUFFER_AUTO(xiiVolumetricTemporalConstants)
{
  MAT4(PreviousViewProjectionMatrix);
  FLOAT1(HistoryWeight);
  FLOAT1(DepthThreshold);
  FLOAT1(NormalThreshold);
  FLOAT1(SpatialWeight);
  UINT1(HistoryValid);
  UINT3(_Padding);
};
