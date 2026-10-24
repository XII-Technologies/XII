/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiEditorGridConstants)
{
  MAT4(RelativeViewProjection);
  MAT4(InverseRelativeViewProjection);
  FLOAT4(Plane);
  FLOAT4(AxisU);
  FLOAT4(AxisV);
  FLOAT4(Spacing);
  FLOAT4(Appearance);
};

#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiEditorGridConstants) == 208);
#endif
