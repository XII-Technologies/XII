/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Display-linear artistic grading controls. Identity values preserve calibrated output.
DECLARE_CONSTANT_BUFFER_AUTO(xiiColorGradingConstants)
{
  FLOAT4(ColorAdjustments); // saturation, contrast, vignette strength, vignette roundness.
  FLOAT4(FilmGrain);        // strength, frame index, unused, unused.
};

#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiColorGradingConstants) == 32U, "Color grading constants must remain byte-compatible with the shader.");
#endif
