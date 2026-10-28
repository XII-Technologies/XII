/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

// Shared by normal grading and its sampled color-cube inspection. Spatial
// vignette and stochastic film grain are not part of a color lookup table.
float3 ApplyColorGradingTransform(float3 color, float saturation, float contrast)
{
  const float luminance = GetLuminance(color);
  color                 = lerp(luminance.xxx, color, saturation);
  return max((color - 0.18f) * contrast + 0.18f, 0.0f);
}
