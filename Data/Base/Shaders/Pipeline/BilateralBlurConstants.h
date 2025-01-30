#pragma once

#include "../Common/ShaderResourceMacros.h"

DECLARE_CONSTANT_BUFFER(xiiBilateralBlurConstants, 3, 0)
{
  UINT1(BlurRadius);
  FLOAT1(GaussianFalloff); // 1 / (2 * sigma * sigma)
  FLOAT1(Sharpness);       // 0 is aquivalent with a Gaussian blur
};
