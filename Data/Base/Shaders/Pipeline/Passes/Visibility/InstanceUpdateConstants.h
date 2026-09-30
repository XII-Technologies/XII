/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiInstanceUpdateConstants)
{
  UINT1(InstanceCount);
  UINT3(_Padding);
};
