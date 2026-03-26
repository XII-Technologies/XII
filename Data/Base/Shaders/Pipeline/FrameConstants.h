#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

// Shared frame-level constants for orchestration and post-processing passes.
DECLARE_CONSTANT_BUFFER_AUTO(xiiFrameConstants)
{
  UINT1(FrameIndex);
  FLOAT1(FrameDeltaTimeMs);
  FLOAT1(GlobalTime);
  FLOAT1(WorldTime);

  FLOAT1(TargetFrameTimeMs);
  FLOAT1(MinDynamicResolutionScale);
  FLOAT1(MaxDynamicResolutionScale);
  FLOAT1(Reserved0);
};
