/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include "../Common/ShaderResourceMacros.h"

DECLARE_CONSTANT_BUFFER_AUTO(xiiDownscaleDepthConstants)
{
  FLOAT2(PixelSize);
  BOOL1(LinearizeDepth);
};
