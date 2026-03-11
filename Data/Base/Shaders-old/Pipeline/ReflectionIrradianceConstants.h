#pragma once

#include "../Common/ShaderResourceMacros.h"

DECLARE_CONSTANT_BUFFER_AUTO(xiiReflectionIrradianceConstants)
{
  FLOAT1(LodLevel);
  FLOAT1(Intensity);
  FLOAT1(Saturation);
  UINT1(OutputIndex);
};
