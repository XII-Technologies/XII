#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiDynamicResolutionPassData)
{
  FLOAT1(FrameDeltaTimeMs);
  FLOAT1(TargetFrameTimeMs);
  FLOAT1(MininimumRenderScale);
  FLOAT1(MaximumRenderScale);
};
