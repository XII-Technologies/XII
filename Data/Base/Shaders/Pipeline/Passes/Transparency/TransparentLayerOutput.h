/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
struct TransparentOutput
{
  float4 Accumulation : SV_Target0;
  float4 Revealage : SV_Target1;
  float4 Additive : SV_Target2;
  float4 Modulation : SV_Target3;
  float4 HistoryTrust : SV_Target4;
};
TransparentOutput xiiTransparentLayer(float3 lighting, float opacity, float viewDepth, uint blendMode, float2 weights)
{
  TransparentOutput output = (TransparentOutput)0;
  output.Modulation        = 1;
  output.HistoryTrust      = float4(0, 0, 0, opacity);
  if (blendMode == 3u)
    output.Additive = float4(lighting * opacity, 0);
  else if (blendMode == 4u)
    output.Modulation = float4(lerp(1, lighting, opacity), 1);
  else
  {
    const float nearRatio = max(viewDepth, 0) / weights.x;
    const float farRatio  = max(viewDepth, 0) / 200.0f;
    const float weight    = opacity * clamp(10.0f / (1.0e-5f + nearRatio * nearRatio + pow(farRatio, 6)), 0.01f, weights.y);
    output.Accumulation   = float4(lighting * opacity * weight, opacity * weight);
    output.Revealage      = float4(0, 0, 0, opacity);
  }
  return output;
}
