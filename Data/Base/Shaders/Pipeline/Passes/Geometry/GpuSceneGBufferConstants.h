/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Per-view constants for canonical material G-buffer rasterization. Vertex stream offsets are
/// explicit so the shader consumes the same packed layout registered by xiiMeshBufferResource.
DECLARE_CONSTANT_BUFFER_AUTO(xiiGpuSceneGBufferConstants)
{
  MAT4(ViewProjectionMatrix);
  MAT4(PreviousViewProjectionMatrix);
  FLOAT2(InvRenderSize);
  FLOAT2(CurrentJitter);
  FLOAT2(PreviousJitter);
  UINT1(GeometryBaseIndex);
  UINT1(MaterialBaseIndex);
  UINT1(VertexStride);
  UINT1(NormalOffset);
  UINT1(TangentOffset);
  UINT1(TexCoordOffset);
  UINT1(MeshDispatchGroupCountX);
  UINT1(MeshDispatchGroupCountY);
};
