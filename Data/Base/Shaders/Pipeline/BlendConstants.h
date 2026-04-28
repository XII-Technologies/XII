/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include "../Common/ShaderResourceMacros.h"

BEGIN_PUSH_CONSTANTS(xiiBlendConstants)
{
  FLOAT1(BlendFactor);
}
END_PUSH_CONSTANTS(xiiBlendConstants)
