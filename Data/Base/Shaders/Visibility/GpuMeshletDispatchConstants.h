/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Configuration used while translating a compact visible-instance count into an
/// indirect, two-dimensional meshlet-culling dispatch.
DECLARE_CONSTANT_BUFFER_AUTO(xiiGpuMeshletDispatchConstants)
{
  UINT1(MaxMeshletGroups);
  UINT3(Padding);
};
