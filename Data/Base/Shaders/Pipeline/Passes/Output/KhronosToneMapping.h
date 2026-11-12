/// Copyright (c) Theophilus Eriata. All Rights Reserved.

//
// Copyright 2024 The Khronos Group, Inc.
// SPDX-License-Identifier: Apache-2.0
// Adapted from https://github.com/KhronosGroup/ToneMapping/blob/main/PBR_Neutral/pbrNeutral.glsl
// Modified for HLSL and XII's display-linear tone-mapping pipeline.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License. A copy is in Licenses/Apache-2.0.txt.

#pragma once

// Non-negative scene-linear BT.709 input to display-linear BT.709 in [0, 1].
float3 KhronosPBRNeutral(float3 color)
{
  const float startCompression = 0.8f - 0.04f;
  const float desaturation     = 0.15f;
  color                        = max(color, 0.0f);
  const float minimum          = min(color.r, min(color.g, color.b));
  const float offset           = minimum < 0.08f ? minimum - 6.25f * minimum * minimum : 0.04f;
  color -= offset;

  const float peak = max(color.r, max(color.g, color.b));
  if (peak < startCompression)
    return color;

  const float distanceToWhite = 1.0f - startCompression;
  const float mappedPeak      = 1.0f - distanceToWhite * distanceToWhite / (peak + distanceToWhite - startCompression);
  color *= mappedPeak / peak;
  const float highlightDesaturation = 1.0f - 1.0f / (desaturation * (peak - mappedPeak) + 1.0f);
  return lerp(color, mappedPeak.xxx, highlightDesaturation);
}
