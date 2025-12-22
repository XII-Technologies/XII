#pragma once

#include "ShaderResourceMacros.h"

DECLARE_CONSTANT_BUFFER_AUTO(xiiPassConstants)
{
  UINT1(MSAASampleCount);
};
