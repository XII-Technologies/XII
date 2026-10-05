/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiEmissiveAliasBuildConstants)
{
  UINT1(TriangleCapacity);
  UINT1(BlockCapacity);
  UINT1(TrianglesPerBlock);
  UINT1(BlockDispatchWidth);
  UINT1(FrameIndex);
  UINT3(_Padding);
};

#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiEmissiveAliasBuildConstants) == 32U, "Emissive alias build constants must match the HLSL constant buffer.");
#endif
