#pragma once

#include "../Common/ShaderResourceMacros.h"

DECLARE_CONSTANT_BUFFER(xiiSSAOConstants, 3, 0)
{
  FLOAT2(TexCoordsScale);
  FLOAT2(FadeOutParams);

  FLOAT1(WorldRadius);
  FLOAT1(MaxScreenSpaceRadius);
  FLOAT1(Contrast);
  FLOAT1(Intensity);

  FLOAT1(PositionBias);
  FLOAT1(MipLevelScale);
  FLOAT1(DepthBlurScale);
  FLOAT1(Padding);
};
