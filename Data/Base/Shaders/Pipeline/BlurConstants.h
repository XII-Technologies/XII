#pragma once

#include "../Common/ShaderResourceMacros.h"

DECLARE_CONSTANT_BUFFER(xiiBlurConstants, 3, 0)
{
  INT1(BlurRadius);
};
