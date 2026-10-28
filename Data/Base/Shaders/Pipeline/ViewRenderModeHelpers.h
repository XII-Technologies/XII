/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#define XII_VIEW_RENDER_MODE(Name, Value) static const uint ViewRenderMode_##Name = Value;
#include <Shaders/Pipeline/ViewRenderModes.h>
#undef XII_VIEW_RENDER_MODE

float3 ViewIdentityColor(uint id)
{
  id ^= id >> 16u;
  id *= 0x7feb352du;
  id ^= id >> 15u;
  id *= 0x846ca68bu;
  id ^= id >> 16u;
  return float3(id & 255u, (id >> 8u) & 255u, (id >> 16u) & 255u) / 255.0f;
}

float3 ViewHeatColor(float value)
{
  return saturate(float3(1.5f - abs(4.0f * value - 3.0f), 1.5f - abs(4.0f * value - 2.0f), 1.5f - abs(4.0f * value - 1.0f)));
}
