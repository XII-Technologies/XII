/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>
DECLARE_CONSTANT_BUFFER_AUTO(xiiScreenIrradianceConstants)
{
  FLOAT4(TraceDistances); // World ray length, view-depth crossing tolerance, world surface bias, reserved.
  UINT4(TraceCounts);     // Samples, steps, sample sequence offset, reserved.
};
