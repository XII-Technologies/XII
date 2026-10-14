/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiViewRenderModeConstants)
{
  UINT1(RenderMode);
  FLOAT3(_Padding);
};
