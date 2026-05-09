#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiProceduralTriangleConstants)
{
  MAT4(mModelViewMatrix);
  FLOAT3(vCameraPos);
  FLOAT1(fTime);
  FLOAT2(vResolution);
  FLOAT1(fWireWidth);
};
