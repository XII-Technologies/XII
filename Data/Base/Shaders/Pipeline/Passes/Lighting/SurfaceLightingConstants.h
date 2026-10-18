/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>
DECLARE_CONSTANT_BUFFER_AUTO(xiiSurfaceCameraConstants)
{
  MAT4(SurfaceViewProjection);
  MAT4(SurfaceInverseViewProjection);
  FLOAT4(SurfaceCameraPositionNear);
  FLOAT4(SurfaceCameraForwardFar);
};
DECLARE_CONSTANT_BUFFER_AUTO(xiiSurfaceLightingConstants)
{
  UINT4(SurfaceExtent); ///< width, height, visibility texture supplied, reserved.
  FLOAT4(SurfaceEvaluation); ///< scale, reserved.
};
