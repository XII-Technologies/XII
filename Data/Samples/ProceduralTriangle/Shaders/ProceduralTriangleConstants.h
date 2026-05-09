#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiProceduralTriangleConstants)
{
  FLOAT2(vResolution);
  FLOAT1(fTime);
  FLOAT1(fWireWidth);
};
