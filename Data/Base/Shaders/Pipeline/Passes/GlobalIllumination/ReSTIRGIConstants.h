/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Runtime controls shared by the temporal and spatial ReSTIR GI passes.
DECLARE_CONSTANT_BUFFER_AUTO(xiiReSTIRGIConstants)
{
  UINT1(HistoryValid);
  UINT1(MaxTemporalM);
  UINT1(SpatialSamples);
  FLOAT1(RadianceClamp);
};

#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiReSTIRGIConstants) == 16U, "ReSTIR GI constants must match the shader constant-buffer layout.");
#endif
