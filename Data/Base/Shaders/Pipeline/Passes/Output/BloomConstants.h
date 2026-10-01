/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Paper-white-relative bloom extraction controls.
DECLARE_CONSTANT_BUFFER_AUTO(xiiBloomConstants)
{
  FLOAT4(BloomParameters); // threshold, soft knee, filter radius in pixels, unused.
};

#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiBloomConstants) == 16U, "Bloom constants must remain byte-compatible with the shader.");
#endif
