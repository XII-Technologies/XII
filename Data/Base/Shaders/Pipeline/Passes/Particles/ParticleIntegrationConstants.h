/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiParticleIntegrationConstants)
{
  FLOAT1(DeltaTimeS);
  FLOAT1(GravityScale);
  FLOAT1(DragCoefficient);
  FLOAT1(TurbulenceScale);
  FLOAT3(WindVelocity);
  UINT1(ActiveCount); // Zero integrates every element in the supplied state buffer.
};
DECLARE_CONSTANT_BUFFER_AUTO(xiiParticleIntegrationDispatchConstants)
{
  UINT1(DispatchGroupsX);
  UINT1(DispatchParticleCount);
  UINT2(_Padding);
};
