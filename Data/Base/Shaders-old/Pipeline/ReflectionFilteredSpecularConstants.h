#pragma once

#include "../Common/ShaderResourceMacros.h"

DECLARE_CONSTANT_BUFFER_AUTO(xiiReflectionFilteredSpecularConstants)
{
  UINT1(MipLevel);
  FLOAT1(Intensity);
  FLOAT1(Saturation);
};
