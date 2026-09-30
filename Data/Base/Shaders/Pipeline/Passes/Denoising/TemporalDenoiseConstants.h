/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Controls motion-aware temporal accumulation for stochastic lighting signals.
///
/// The signal mode selects scalar ambient visibility (0) or RGBA radiance and
/// confidence (1). History is explicitly invalidated after allocation or resize
/// so freshly-created persistent textures are never sampled as valid data.
DECLARE_CONSTANT_BUFFER_AUTO(xiiTemporalDenoiseConstants)
{
  FLOAT1(HistoryWeight);
  FLOAT1(DepthThreshold);
  FLOAT1(NormalThreshold);
  FLOAT1(SpatialWeight);
  UINT1(HistoryValid);
  UINT1(SignalMode);
  FLOAT2(_Padding);
};

