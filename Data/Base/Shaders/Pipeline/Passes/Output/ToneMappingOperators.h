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

#include <Shaders/Common/Common.h>

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

// Timothy Lottes, "Advanced Techniques and Optimization of HDR Color Pipelines" (2016).
// Reference parameters: contrast 1.6, shoulder 0.977, HDR white 8, and 0.18 -> 0.267 mid-gray.
float3 Lottes(float3 color)
{
  const float contrast       = 1.6f;
  const float shoulder       = 0.977f;
  const float hdrMaximum     = 8.0f;
  const float middleInput    = 0.18f;
  const float middleOutput   = 0.267f;
  const float whitePower     = pow(hdrMaximum, contrast);
  const float middlePower    = pow(middleInput, contrast);
  const float whiteShoulder  = pow(hdrMaximum, contrast * shoulder);
  const float middleShoulder = pow(middleInput, contrast * shoulder);
  const float denominator    = (whiteShoulder - middleShoulder) * middleOutput;
  const float b              = (whitePower * middleOutput - middlePower) / denominator;
  const float c              = (whiteShoulder * middlePower - whitePower * middleShoulder * middleOutput) / denominator;
  color                      = max(color, 0.0f);
  return saturate(pow(color, contrast) / (pow(color, contrast * shoulder) * b + c));
}

// Hajime Uchimura, "HDR theory and practice" (2017), with the reference SDR parameters.
float3 Uchimura(float3 color)
{
  const float peak          = 1.0f;
  const float contrast      = 1.0f;
  const float linearStart   = 0.22f;
  const float linearLength  = 0.4f;
  const float toePower      = 1.33f;
  const float linearOffset  = (peak - linearStart) * linearLength / contrast;
  const float shoulderStart = linearStart + linearOffset;
  const float shoulderValue = linearStart + contrast * linearOffset;
  const float shoulderSlope = -contrast / (peak - shoulderValue);
  color                     = max(color, 0.0f);

  const float3 toeWeight      = 1.0f - smoothstep(0.0f, linearStart, color);
  const float3 shoulderWeight = step(shoulderStart, color);
  const float3 linearWeight   = 1.0f - toeWeight - shoulderWeight;
  const float3 toe            = linearStart * pow(color / linearStart, toePower);
  const float3 linearSegment  = linearStart + contrast * (color - linearStart);
  const float3 shoulder       = peak - (peak - shoulderValue) * exp(shoulderSlope * (color - shoulderStart));
  return saturate(toe * toeWeight + linearSegment * linearWeight + shoulder * shoulderWeight);
}

// Extended Reinhard with a scene-linear white point of 4.
float3 ReinhardExtended(float3 color)
{
  const float whitePoint = 4.0f;
  color                  = max(color, 0.0f);
  return saturate(color * (1.0f + color / (whitePoint * whitePoint)) / (1.0f + color));
}

// Apply Reinhard to BT.709 luminance rather than independently to each channel.
float3 ReinhardLuminance(float3 color)
{
  color = max(color, 0.0f);
  return saturate(color / (1.0f + GetLuminance(color)));
}

float3 Exponential(float3 color)
{
  return 1.0f - exp(-max(color, 0.0f));
}
