/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Per-view camera history used to reproject the current depth buffer.
///
/// Jitter is expressed in clip-space NDC. It is zero until the view's temporal
/// sample generator is enabled, but keeping it in the shared ABI prevents a
/// future TAA integration from changing the pipeline layout.
DECLARE_CONSTANT_BUFFER_AUTO(xiiMotionVectorConstants)
{
  MAT4(PreviousViewProjectionMatrix);
  FLOAT2(CurrentJitter);
  FLOAT2(PreviousJitter);
};
