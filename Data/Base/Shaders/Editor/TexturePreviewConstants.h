/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiTexturePreviewConstants)
{
  MAT4(ViewProjection);
  INT1(Channel);
  FLOAT1(Mip);
  UINT1(IsLinear);
  FLOAT1(AlphaThreshold);
  UINT1(OutputIsSrgb);
  FLOAT3(Padding);
};

#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiTexturePreviewConstants) == 96U);
#endif
