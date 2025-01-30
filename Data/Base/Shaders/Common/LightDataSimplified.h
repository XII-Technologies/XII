#pragma once

#if SHADING_QUALITY != SHADING_QUALITY_SIMPLIFIED
#  error "Functions in LightDataSimplified.h are only for SIMPLIFIED shading quality. Include LightData.h instead."
#endif

#include "ShaderResourceMacros.h"

DECLARE_CONSTANT_BUFFER(xiiSimplifiedDataConstants, 3, 0)
{
  UINT1(SkyIrradianceIndex);
};
