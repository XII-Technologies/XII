/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>
DECLARE_CONSTANT_BUFFER_AUTO(xiiAmbientOcclusionConstants)
{
  FLOAT1(Radius);
  FLOAT1(RayOriginBias);
  FLOAT1(RayTMin);
  FLOAT1(NormalBias);
  FLOAT1(PixelStride);
  FLOAT1(MinSampleDistance);
  UINT1(DirectionCount);
  UINT1(StepCount);
};
