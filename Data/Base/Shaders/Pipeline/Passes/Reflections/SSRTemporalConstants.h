/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Temporal validation and confidence controls for screen-space reflections.
DECLARE_CONSTANT_BUFFER_AUTO(xiiSSRTemporalConstants)
{
  FLOAT1(HistoryWeight);
  FLOAT1(ConfidenceDecay);
  FLOAT1(DepthThreshold);
  FLOAT1(NormalThreshold);
  UINT1(HistoryValid);
  UINT3(_Padding);
};
