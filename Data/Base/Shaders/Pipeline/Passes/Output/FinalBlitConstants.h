/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Transfer-function controls for the final presentation draw.
DECLARE_CONSTANT_BUFFER_AUTO(xiiFinalBlitConstants)
{
  UINT1(OutputMode);
  UINT1(ApplySRGBTransfer);
  FLOAT1(PaperWhiteNits);
  FLOAT1(MaximumDisplayNits);
};

#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiFinalBlitConstants) == 16U, "Final blit constants must remain byte-compatible with the shader.");
#endif
