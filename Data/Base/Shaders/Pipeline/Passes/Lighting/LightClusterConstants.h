/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>
DECLARE_CONSTANT_BUFFER_AUTO(xiiLightClusterConstants)
{
  MAT4(InverseViewProjection);
  MAT4(ViewProjection);
  UINT4(ClusterCounts); ///< xyz = dimensions, w = total.
  UINT4(Viewport);      ///< xy = exact viewport, z = tile size, w = list capacity.
  FLOAT4(DepthRange);   ///< near, far, log2(far/near), logarithmic flag.
  FLOAT4(CameraPosition);
  FLOAT4(CameraForward);
  UINT4(Work); ///< work item count, index capacity, dispatch width, assignment uses candidates.
};
struct XII_SHADER_STRUCT xiiGpuClusterBounds
{
  FLOAT4(MinBounds);
  FLOAT4(MaxBounds);
};
#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiGpuClusterBounds) == 32);
#endif
