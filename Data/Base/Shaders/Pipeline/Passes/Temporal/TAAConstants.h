/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Runtime controls for temporal anti-aliasing history reconstruction.
DECLARE_CONSTANT_BUFFER_AUTO(xiiTAAConstants)
{
  UINT1(HistoryValid);
  FLOAT1(BaseBlendAlpha);   ///< Current-frame contribution for stable pixels.
  FLOAT1(MotionBlendScale); ///< Additional current-frame contribution per NDC velocity unit.
  FLOAT1(Padding);
};
