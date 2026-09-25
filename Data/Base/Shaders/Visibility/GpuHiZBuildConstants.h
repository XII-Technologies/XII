/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiGpuHiZBuildConstants)
{
  UINT2(SrcSize);
  UINT2(DstSize);
  UINT1(Reduce);
  UINT3(Padding);
};
