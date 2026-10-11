/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>

struct xiiSceneShapeIconData
{
  FLOAT4(ClipRect); // left, top, width, height; clip coordinates
  FLOAT4(Color);
  FLOAT1(Depth);
  UINT1(PickingID);
  FLOAT2(Padding);
};

#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiSceneShapeIconData) == 48U);
#endif
