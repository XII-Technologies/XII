/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiEmissiveLightBuildConstants)
{
  UINT1(GeometryBaseIndex);
  UINT1(MaterialBaseIndex);
  UINT1(InstanceCount);
  UINT1(MaxTriangleCount);
  UINT1(VertexStride);
  UINT1(TexCoordOffset);
  UINT1(MaxMeshletCount);
  UINT1(FrameIndex);
  UINT4(DispatchDimensions); ///< xy = actual instance dispatch Y/Z extents; zw reserved.
};

#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiEmissiveLightBuildConstants) == 48U, "Emissive build constants must match the HLSL constant buffer.");
#endif
