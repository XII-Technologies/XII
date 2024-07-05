#pragma once

#include "../Common/ConstantBufferMacros.h"

#include "../Common/Platforms.h"

CONSTANT_BUFFER(xiiVRCompanionViewConstants, 2)
{
  FLOAT2(TargetSize);
};
