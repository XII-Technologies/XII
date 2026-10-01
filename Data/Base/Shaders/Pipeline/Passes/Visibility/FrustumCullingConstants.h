/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// World-space outward-facing frustum planes and the number of populated bounds.
DECLARE_CONSTANT_BUFFER_AUTO(xiiFrustumCullingConstants)
{
  FLOAT4(FrustumPlane0);
  FLOAT4(FrustumPlane1);
  FLOAT4(FrustumPlane2);
  FLOAT4(FrustumPlane3);
  FLOAT4(FrustumPlane4);
  FLOAT4(FrustumPlane5);
  UINT1(InstanceCount);
  UINT3(_Padding);
};
