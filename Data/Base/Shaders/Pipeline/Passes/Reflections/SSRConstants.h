/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Tunable screen-space reflection tracing parameters.
///
/// The same layout is consumed by C++ and shader code so Debug builds validate
/// the resource contract instead of relying on an unbound ad-hoc cbuffer.
DECLARE_CONSTANT_BUFFER_AUTO(xiiSSRConstants)
{
  UINT1(MaxSteps);
  FLOAT1(Thickness);
  FLOAT1(MaxRoughness);
  FLOAT1(StrideZCutoff);
  FLOAT2(InvResolution);
  UINT1(HiZMipCount);
  FLOAT1(_Padding);
};

