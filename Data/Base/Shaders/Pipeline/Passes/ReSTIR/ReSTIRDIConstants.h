/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Per-view ReSTIR DI history state. History is explicitly invalidated after
/// allocation, resize, camera cuts, and other temporal discontinuities.
DECLARE_CONSTANT_BUFFER_AUTO(xiiReSTIRDIConstants)
{
  UINT1(HistoryValid);
  UINT3(_Padding);
};

#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiReSTIRDIConstants) == 16U, "ReSTIR DI constants must remain byte-compatible with the shader.");
#endif
