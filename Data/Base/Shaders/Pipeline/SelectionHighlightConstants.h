#pragma once

#include "../Common/ShaderResourceMacros.h"

DECLARE_CONSTANT_BUFFER(xiiSelectionHighlightConstants, 3, 0)
{
  COLOR4F(HighlightColor);
  FLOAT1(OverlayOpacity);
};
