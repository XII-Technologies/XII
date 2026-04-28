/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#if !defined(RENDER_PASS) || !defined(BLEND_MODE)
#  error "RENDER_PASS and BLEND_MODE permutations must be defined"
#endif

#ifndef USE_WORLDPOS
#  define USE_WORLDPOS
#endif

#if SHADING_QUALITY == SHADING_QUALITY_MEDIUM
#  include <Shaders/Materials/MaterialPixelShaderNormal.h>
#elif SHADING_QUALITY == SHADING_QUALITY_LOW
#  include <Shaders/Materials/MaterialPixelShaderSimplified.h>
#else
#  error "Unknown shading quality configuration."
#endif
