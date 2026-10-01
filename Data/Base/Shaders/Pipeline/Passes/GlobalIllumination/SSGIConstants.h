/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Runtime controls for the non-ray-tracing near-field GI fallback.
DECLARE_CONSTANT_BUFFER_AUTO(xiiSSGIConstants)
{
  FLOAT1(RayLength);  ///< Maximum world-space trace distance in metres.
  UINT1(SampleCount); ///< Cosine-weighted hemisphere rays per pixel.
  FLOAT1(Thickness);  ///< Linear-depth hit acceptance thickness in metres.
  FLOAT1(Intensity);  ///< Physically based fallback contribution scale.
};
