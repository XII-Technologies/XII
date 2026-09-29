/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiVirtualShadowFeedbackConstants)
{
  UINT1(VirtualResolution);
  UINT1(PageSize);
  UINT1(MaxFeedbackRequests);
  UINT1(DirectionalLightId);
};

