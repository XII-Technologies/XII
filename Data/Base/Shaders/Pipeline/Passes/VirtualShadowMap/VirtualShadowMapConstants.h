/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiVirtualShadowFeedbackConstants)
{
  MAT4(VirtualShadowInverseViewProjection);
  UINT1(VirtualResolution);
  UINT1(PageSize);
  UINT1(MaxFeedbackRequests);
  UINT1(DirectionalLightId);
  FLOAT1(VirtualShadowNearPlane);
  FLOAT3(_VirtualShadowFeedbackPadding);
};

/// Frame-sliced virtual-to-physical lookup metadata shared by every VSM
/// consumer. The enable bit lets a pass bind the persistent resources while
/// retaining a deterministic fallback when no shadow-casting sun exists.
DECLARE_CONSTANT_BUFFER_AUTO(xiiVirtualShadowSamplingConstants)
{
  UINT1(VirtualShadowPageTableBaseIndex);
  UINT1(VirtualShadowPageTableCapacity);
  UINT1(VirtualShadowResolution);
  UINT1(VirtualShadowPageSize);
  UINT1(VirtualShadowPhysicalAtlasWidth);
  UINT1(VirtualShadowPhysicalAtlasHeight);
  UINT1(VirtualShadowDirectionalLightId);
  UINT1(VirtualShadowEnabled);
};
