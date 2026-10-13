/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiSceneSelectionConstants)
{
  FLOAT4(VisibleColor);
  FLOAT4(OccludedColor);
  FLOAT1(FillOpacity);
  UINT1(OutlineWidth);
  FLOAT2(Padding);
};

#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiSceneSelectionConstants) == 48U);
#endif
