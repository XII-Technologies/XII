/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>
DECLARE_CONSTANT_BUFFER_AUTO(xiiScreenRefractionConstants)
{
  FLOAT4(RefractionOptions); // Perturbation scale, maximum UV offset, chromatic spread, reserved.
};
