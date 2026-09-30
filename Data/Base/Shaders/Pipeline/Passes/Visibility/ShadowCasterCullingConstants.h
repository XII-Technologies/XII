/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiShadowCasterCullingConstants)
{
  UINT1(InstanceCount);
  UINT1(MaxCommandCount);
  UINT1(CullingActiveCascadeCount);
  UINT1(_Padding);
};
