#pragma once

#include "ShaderResourceMacros.h"

#define MIN_PERCEPTUAL_ROUGHNESS 0.045
#define MIN_ROUGHNESS            0.002025

DECLARE_CONSTANT_BUFFER(xiiGlobalConstants, 0, 0)
{
  // Use functions from CameraConstantsAccess.h to access these and derived camera properties.
  // clang-format off
  MAT4(CameraToScreenMatrix)[2];
  MAT4(ScreenToCameraMatrix)[2];
  MAT4(WorldToCameraMatrix) [2];
  MAT4(CameraToWorldMatrix) [2];
  MAT4(WorldToScreenMatrix) [2];
  MAT4(ScreenToWorldMatrix) [2];
  // clang-format on

  FLOAT4(ViewportSize); ///< Describes x = width, y = height, z = 1 / width, w = 1 / height.
  FLOAT4(ClipPlanes);   ///< Describes x = near, y = far, z = 1 / far.
  FLOAT1(MaxZValue);    ///< Any screenspace z values smaller than this value are clamped. Used for directional shadows.

  FLOAT1(DeltaTime);
  FLOAT1(GlobalTime);
  FLOAT1(WorldTime);

  FLOAT1(Exposure);
};

#include "CameraConstantsAccess.h"
