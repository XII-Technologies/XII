#pragma once

#include "../Common/ShaderResourceMacros.h"

DECLARE_CONSTANT_BUFFER_AUTO(xiiVRCompanionViewConstants)
{
  FLOAT2(TargetSize);
};
