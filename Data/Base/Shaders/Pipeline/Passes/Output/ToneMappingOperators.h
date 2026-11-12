/// Copyright (c) Theophilus Eriata. All Rights Reserved.
//
// Reference implementations adapted from Baking Lab and shaders-tone-map:
// https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
// https://github.com/dmnsgn/shaders-tone-map/tree/main/packages/glsl-tone-map
//
// MIT License
// Copyright (c) 2016 MJP
// Copyright (C) 2019 Damien Seguin
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

// Stephen Hill's ACES RRT/ODT fit, including its AP1 input/output transforms.
float3 ACESFitted(float3 color)
{
  const float3x3 inputTransform = float3x3(
    0.59719f, 0.35458f, 0.04823f,
    0.07600f, 0.90834f, 0.01566f,
    0.02840f, 0.13383f, 0.83777f);
  const float3x3 outputTransform = float3x3(
    1.60475f, -0.53108f, -0.07367f,
    -0.10208f, 1.10813f, -0.00605f,
    -0.00327f, -0.07276f, 1.07602f);
  color                    = mul(inputTransform, max(color, 0.0f));
  const float3 numerator   = color * (color + 0.0245786f) - 0.000090537f;
  const float3 denominator = color * (0.983729f * color + 0.4329510f) + 0.238081f;
  return saturate(mul(outputTransform, numerator / denominator));
}

// Hejl-Burgess-Dawson filmic curve. Its fitted display encoding is decoded here
// so grading and presentation continue to operate on display-linear values.
float3 Filmic(float3 color)
{
  color                = max(color - 0.004f, 0.0f);
  const float3 encoded = color * (6.2f * color + 0.5f) / (color * (6.2f * color + 1.7f) + 0.06f);
  return pow(saturate(encoded), 2.2f);
}
