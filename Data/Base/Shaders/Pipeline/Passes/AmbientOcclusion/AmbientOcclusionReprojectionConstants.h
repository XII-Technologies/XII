/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiAmbientOcclusionReprojectionConstants)
{
  FLOAT4(PreviousCameraPosition);
  FLOAT4(PreviousCameraForward);
  FLOAT2(JitterDelta);
  FLOAT2(_ReprojectionPadding);
};
