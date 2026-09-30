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

#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiExposureHistogramConstants) == 32U, "Exposure histogram constants must remain byte-compatible with the shader.");
#endif
