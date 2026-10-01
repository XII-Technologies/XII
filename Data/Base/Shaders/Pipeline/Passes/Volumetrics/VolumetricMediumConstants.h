/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Per-view controls for injecting sparse participating media into the froxel grid.
DECLARE_CONSTANT_BUFFER_AUTO(xiiVolumetricMediumConstants)
{
  UINT1(ActiveMediumCount);
  UINT3(_VolumetricMediumPadding);
};

#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiVolumetricMediumConstants) == 16U, "Volumetric medium constants must remain byte-compatible with the shaders.");
#endif
