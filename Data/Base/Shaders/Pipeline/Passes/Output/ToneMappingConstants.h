/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Scene-linear tone mapping and target-display calibration.
DECLARE_CONSTANT_BUFFER_AUTO(xiiToneMappingConstants)
{
  UINT1(Operator);
  UINT1(OutputMode);
  FLOAT1(BloomStrength);
  FLOAT1(PaperWhiteNits);
  FLOAT1(MaximumDisplayNits);
  FLOAT3(_Padding);
};

#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiToneMappingConstants) == 32U, "Tone mapping constants must remain byte-compatible with the shader.");
#endif
