/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiRayTracingDeformationConstants)
{
  UINT4(VertexLayout); // count, stride, position offset, normal offset (or invalid)
  UINT4(SkinLayout);   // tangent offset (or invalid), joints offset, weights offset, palette count
  UINT4(Deformation);  // morph count, palette offset, reserved, reserved
};
