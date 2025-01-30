#pragma once

#include "../Common/ShaderResourceMacros.h"

DECLARE_CONSTANT_BUFFER(xiiCopyConstants, 3, 0)
{
  INT2(Offset);
};
