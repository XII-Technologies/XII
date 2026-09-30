/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiDrawCommandBuildConstants)
{
  UINT1(InstanceCount);
  UINT1(MaxCommandCount);
  UINT2(_Padding);
};
