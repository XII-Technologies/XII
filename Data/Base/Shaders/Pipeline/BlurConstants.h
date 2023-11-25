#pragma once

#include "../Common/Platforms.h"
#include "../Common/ConstantBufferMacros.h"

CONSTANT_BUFFER(xiiBlurConstants, 3)
{
  INT1(BlurRadius);
};

