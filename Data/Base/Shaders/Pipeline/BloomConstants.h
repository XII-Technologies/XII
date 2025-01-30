#pragma once

#include "../Common/ShaderResourceMacros.h"

DECLARE_CONSTANT_BUFFER(xiiBloomConstants, 3, 0)
{
  FLOAT2(PixelSize);
  FLOAT1(BloomThreshold);
  FLOAT1(BloomIntensity);

  COLOR4F(TintColor);
};
