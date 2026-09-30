/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Per-view constants for conservative reversed-Z occlusion culling.
DECLARE_CONSTANT_BUFFER_AUTO(xiiHiZOcclusionConstants)
{
  MAT4(ViewProjectionMatrix);
  UINT2(HiZSize);
  UINT1(HiZMipCount);
  UINT1(CandidateCapacity);
  FLOAT1(DepthBias);
  FLOAT3(_Padding);
};

