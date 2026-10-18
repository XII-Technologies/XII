/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>
DECLARE_CONSTANT_BUFFER_AUTO(xiiDepthRegionClearConstants)
{
  FLOAT1(DepthValue);
  UINT3(DepthClearPadding);
};
