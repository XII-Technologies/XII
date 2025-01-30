#pragma once

#include "../Common/ShaderResourceMacros.h"

DECLARE_CONSTANT_BUFFER(xiiDownscaleDepthConstants, 3, 0)
{
  FLOAT2(PixelSize);
  BOOL1(LinearizeDepth);
};
