/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Signal-specific temporal and variance controls for ray-traced reflections.
DECLARE_CONSTANT_BUFFER_AUTO(xiiRTReflectionDenoiseConstants)
{
  FLOAT1(HistoryWeight);
  FLOAT1(MomentsHistoryWeight);
  FLOAT1(ConfidenceDecay);
  FLOAT1(SpatialWeight);

  FLOAT1(DepthThreshold);
  FLOAT1(NormalThreshold);
  FLOAT1(HitDistanceRelativeThreshold);
  FLOAT1(HitDistanceAbsoluteThreshold);

  FLOAT1(VarianceGamma);
  UINT1(HistoryValid);
  UINT2(_Padding);
};
