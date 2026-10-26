/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

// Matches the canonical 128-byte xiiParticleGPUState. Integration and rendering share this shader record.
struct xiiParticleState
{
  float4 PositionAge;
  float4 PreviousPositionRadius;
  float4 VelocityLifetime;
  float4 Color;
  float4 ForceInverseMass;
  float4 Custom0;
  float4 Custom1;
  uint   Id;
  uint   Flags;
  uint   Emitter;
  uint   CellId;
};
