/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiViewRenderModeConstants)
{
  UINT1(RenderMode);
  UINT1(TexCoord1Offset);
  UINT1(ColorOffset);
  FLOAT1(MotionVectorScale);
};
