#pragma once

#include "../Common/ShaderResourceMacros.h"

DECLARE_CONSTANT_BUFFER(xiiVRCompanionViewConstants, 2, 0)
{
  FLOAT2(TargetSize);
};
