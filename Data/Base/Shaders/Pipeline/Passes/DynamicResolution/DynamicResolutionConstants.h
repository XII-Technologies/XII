/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiDynamicResolutionPassConstants)
{
  FLOAT1(FrameDeltaTimeMs);   ///< This is the time difference between the current frame and the previous frame in milliseconds. The dynamic resolution system can use this to understand how much time it has to render the current frame and adjust the resolution accordingly.
  FLOAT1(TargetFrameTimeMs);  ///< This is the target frame time in milliseconds that the dynamic resolution system should aim for. The system will try to adjust the render resolution each frame to match this target time as closely as possible, providing a smoother experience by maintaining a consistent frame rate.
  FLOAT1(MinimumRenderScale); ///< This is the minimum render scale that the dynamic resolution system can use. It is a multiplier for the render resolution relative to the native resolution. For example, a value of 0.5 means the render resolution can go down to 50% of the native resolution. This prevents the system from reducing the resolution too much, which could lead to unacceptable visual quality.
  FLOAT1(MaximumRenderScale); ///< This is the maximum render scale that the dynamic resolution system can use. It is a multiplier for the render resolution relative to the native resolution. For example, a value of 1.0 means the render resolution can go up to 100% of the native resolution. This prevents the system from increasing the resolution too much, which could lead to unnecessary performance costs.
  FLOAT1(CurrentGpuTimeMs);   ///< This is the latest measured GPU time in milliseconds. The dynamic resolution system can use this to understand how much time the GPU is taking to render the current frame and adjust the resolution accordingly.
  FLOAT1(SmoothedGpuTimeMs);  ///< This is the smoothed GPU time in milliseconds. The dynamic resolution system can use this to understand the average GPU time over multiple frames and adjust the resolution accordingly.
};
