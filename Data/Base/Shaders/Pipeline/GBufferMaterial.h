/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

// The G-buffer stores evaluated surfaces, not runtime texture bindings. Bits 1..7
// retain the low material features; bit 0 replaces the already-baked normal-map
// flag with an explicit unlit marker. Transmission remains bit 6.
static const uint XII_GBUFFER_UNLIT = 1u;

uint xiiGBufferMaterialFlags(float encoded)
{
  return (uint)round(saturate(encoded) * 255.0f);
}

bool xiiGBufferReceivesLighting(float4 material)
{
  return (xiiGBufferMaterialFlags(material.a) & XII_GBUFFER_UNLIT) == 0;
}

float xiiPackGBufferMaterialFlags(uint runtimeFlags, bool unlit)
{
  return float((runtimeFlags & 0xFEu) | (unlit ? XII_GBUFFER_UNLIT : 0u)) / 255.0f;
}
