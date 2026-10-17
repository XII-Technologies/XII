/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>
DECLARE_CONSTANT_BUFFER_AUTO(xiiDepthOcclusionConstants)
{
  FLOAT1(DepthBias);
  UINT3(OcclusionPadding);
};
