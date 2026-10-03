/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Per-view constants for GPU-scene mesh-shader rasterization.
DECLARE_CONSTANT_BUFFER_AUTO(xiiGpuSceneRasterConstants)
{
  MAT4(ViewProjectionMatrix);
  UINT1(GeometryBaseIndex);
  UINT1(VertexStride);
  UINT1(MeshDispatchGroupCountX);
  UINT1(MeshDispatchGroupCountY);
};
