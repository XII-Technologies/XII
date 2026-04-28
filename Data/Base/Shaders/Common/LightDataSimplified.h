/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#if SHADING_QUALITY != SHADING_QUALITY_LOW
#  error "Functions in LightDataSimplified.h are only for SIMPLIFIED shading quality. Include LightData.h instead."
#endif

#include "ShaderResourceMacros.h"

DECLARE_CONSTANT_BUFFER_AUTO(xiiSimplifiedDataConstants)
{
  UINT1(SkyIrradianceIndex);
};
