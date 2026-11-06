/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiAtmosphereEnvironmentConstants)
{
  FLOAT4(CaptureDirectionAndIlluminance);
  FLOAT4(CaptureColorAndAltitudeMeters);
  UINT4(CaptureExtent);
};
