#pragma once

#include "../Common/ShaderResourceMacros.h"

DECLARE_CONSTANT_BUFFER_AUTO(xiiBlurConstants)
{
  INT1(BlurRadius);
};
