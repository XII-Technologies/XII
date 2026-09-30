/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Log-luminance mapping used while building the per-frame histogram.
DECLARE_CONSTANT_BUFFER_AUTO(xiiExposureHistogramConstants)
{
  FLOAT4(LogLuminanceRange); // minimum, maximum, inverse range, range.
  UINT2(InputResolution);
  FLOAT2(_Padding);
};

/// Physical camera and temporal adaptation controls.
DECLARE_CONSTANT_BUFFER_AUTO(xiiExposureAdaptationConstants)
{
  FLOAT4(LogLuminanceRange); // minimum, maximum, range, unused.
  FLOAT4(Metering);          // low percentile, high percentile, brighten speed, darken speed.
  FLOAT4(Exposure);          // delta seconds, minimum EV100, maximum EV100, compensation EV.
  FLOAT1(ManualExposure);
  UINT1(AutomaticExposure);
  UINT1(HistoryValid);
  FLOAT1(_Padding);
};

#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiExposureHistogramConstants) == 32U, "Exposure histogram constants must remain byte-compatible with the shader.");
static_assert(sizeof(xiiExposureAdaptationConstants) == 64U, "Exposure adaptation constants must remain byte-compatible with the shader.");
#endif

