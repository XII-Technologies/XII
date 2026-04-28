/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include "../Common/ShaderResourceMacros.h"

DECLARE_CONSTANT_BUFFER_AUTO(xiiSelectionHighlightConstants)
{
  COLOR4F(HighlightColor);
  FLOAT1(OverlayOpacity);
};
