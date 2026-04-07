#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiPerFrameGlobalUploadData)
{
  UINT1(FrameIndex);         ///< This incrementing index can be used for shader-based frame counting and to detect buffer overwrites in the shader.
  FLOAT1(DeltaTime);         ///< The time difference between the current and previous frame in seconds, useful for time-based animations and effects.
  FLOAT1(GlobalTime);        ///< The total elapsed time since the application started, in seconds. This can be used for global animations or time-based effects that need to be consistent across frames.
  FLOAT1(WorldTime);         ///< The world simulation time, in seconds. This can be used for world-specific animations or effects.
  FLOAT4(RenderScaleJitter); ///< The current frame's render scale and jitter values, where x = render scale (dynamic resolution), y/z = jitter offsets for temporal anti-aliasing, and w is unused.
};

DECLARE_CONSTANT_BUFFER_AUTO(xiiPerFrameCameraUploadData)
{
  MAT4(ViewProjectionMatrix)
  [2]; ///< For transforming vertices and reconstructing view-space position from depth.
  MAT4(InverseProjectionMatrix)
  [2]; ///< For reconstructing view-space position from depth.
  FLOAT4(CameraPositionAndNearPlane)
  [2]; ///< xyz = position, w = near plane.
  FLOAT4(CameraDirectionAndFarPlane)
  [2]; ///< xyz = direction, w = far plane.
};
