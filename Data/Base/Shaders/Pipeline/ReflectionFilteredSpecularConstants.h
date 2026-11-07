/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include "../Common/ShaderResourceMacros.h"

DECLARE_CONSTANT_BUFFER_AUTO(xiiReflectionFilteredSpecularConstants)
{
  UINT1(MipLevel);
  UINT1(MipCount);
  UINT1(FaceExtent);
  UINT1(SampleCount);
};
