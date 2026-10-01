/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Runtime controls for screen-space transmission/refraction.
DECLARE_CONSTANT_BUFFER_AUTO(xiiSSRefractionConstants)
{
  FLOAT1(PerturbScale); ///< Screen-space distortion scale before depth attenuation.
  FLOAT1(MaxDistance);  ///< Maximum UV displacement from the source pixel.
  FLOAT1(Chromatic);    ///< Relative red/blue dispersion around the green sample.
  FLOAT1(Padding);
};
