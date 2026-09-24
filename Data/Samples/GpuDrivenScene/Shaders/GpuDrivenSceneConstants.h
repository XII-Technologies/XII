/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Frame-local constants shared by the GPU-driven mesh and pixel stages.
/// Geometry and material bases select the current frame-in-flight slices.
DECLARE_CONSTANT_BUFFER_AUTO(xiiGpuDrivenSceneConstants)
{
  MAT4(ViewProjectionMatrix);
  UINT1(GeometryBaseIndex);
  UINT1(MaterialFrameBase);
  UINT1(MaterialStride);
  UINT1(VertexStride);
  FLOAT4(SunDirectionIntensity);
  FLOAT4(AmbientColor);
};

