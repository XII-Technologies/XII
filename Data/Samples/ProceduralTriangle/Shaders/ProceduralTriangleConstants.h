/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

// One 16-byte row per group. This layout is shared by C++ and HLSL.
DECLARE_CONSTANT_BUFFER_AUTO(xiiProceduralTriangleConstants)
{
  FLOAT2(vResolution);       ///< Resolution of the render target in pixels.
  FLOAT1(fTime);             ///< Time in seconds since the start of the application, modulo 1000.0f.
  FLOAT1(fWireWidth);        ///< Width of the wireframe lines in pixels.
  FLOAT4(vSceneParameters);  ///< x = palette [0..3], y = quality [0..2], z = exposure, w = HUD [0..1].
  FLOAT4(vCameraParameters); ///< x = camera yaw offset, y = pitch offset, z = camera distance, w = reserved for future audio modulation.
};
