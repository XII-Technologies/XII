#pragma once

#include "../../../Common/ShaderResourceMacros.h"

DECLARE_CONSTANT_BUFFER_AUTO(xiiBlurConstants)
{
  FLOAT2(SourceTexelSize); // 1/width, 1/height
  FLOAT2(Direction);       // For directional blur
  FLOAT2(RadialCenter);    // UV center for radial blur
  FLOAT1(RadiusPixels);
  FLOAT1(Sigma);
  FLOAT1(RadialStrength);
  UINT1(Iterations);
  UINT1(KernelCount);        // number of taps used
  FLOAT1(KernelWeights)[32]; // separable weights (Gaussian/Box)
  FLOAT1(KernelOffsets)[32]; // pixel offsets (paired tap optimization)
  FLOAT1(DepthSigma);
  FLOAT1(NormalSigma);
};
