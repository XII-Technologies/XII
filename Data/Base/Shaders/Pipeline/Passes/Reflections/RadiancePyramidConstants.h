/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiRadiancePyramidConstants)
{
  UINT2(SourceSize);
  UINT2(TargetSize);
  UINT1(Reduce);
  UINT3(Padding);
};
