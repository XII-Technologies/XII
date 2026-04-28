/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include "../Common/ShaderResourceMacros.h"

DECLARE_CONSTANT_BUFFER_AUTO(xiiBilateralBlurConstants)
{
  UINT1(BlurRadius);
  FLOAT1(GaussianFalloff); // 1 / (2 * sigma * sigma)
  FLOAT1(Sharpness);       // 0 is aquivalent with a Gaussian blur
};
