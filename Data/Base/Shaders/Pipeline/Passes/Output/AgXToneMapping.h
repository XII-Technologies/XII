/// Copyright (c) Theophilus Eriata. All Rights Reserved.
//
// AgX analytical approximation adapted from Benjamin Wrensch's minimal implementation:
// https://iolite-engine.com/blog_posts/minimal_agx_implementation
// Color transforms and exposure range: https://github.com/sobotka/AgX
//
// MIT License
//
// Copyright (c) 2024 Missing Deadlines (Benjamin Wrensch)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

float3 AgXDefaultContrastApprox(float3 x)
{
  const float3 x2 = x * x;
  const float3 x4 = x2 * x2;
  return 15.5f * x4 * x2 - 40.14f * x4 * x + 31.96f * x4 - 6.868f * x2 * x + 0.4298f * x2 + 0.1191f * x - 0.00232f;
}

// Exposed scene-linear BT.709/sRGB input to display-linear BT.709 output.
float3 AgX(float3 color)
{
  // Rows are output channels; use matrix-vector multiplication for both transforms.
  const float3x3 inset = float3x3(
    0.842479062253094f, 0.0784335999999992f, 0.0792237451477643f,
    0.0423282422610123f, 0.878468636469772f, 0.0791661274605434f,
    0.0423756549057051f, 0.0784336f, 0.879142973793104f);
  const float3x3 outset = float3x3(
    1.19687900512017f, -0.0980208811401368f, -0.0990297440797205f,
    -0.0528968517574562f, 1.15190312990417f, -0.0989611768448433f,
    -0.0529716355144438f, -0.0980434501171241f, 1.15107367264116f);
  const float minEV = -12.47393f;
  const float maxEV = 4.026069f;

  color = mul(inset, max(color, 0.0f));
  // The log floor lies below the encoded exposure range and keeps black finite.
  color = saturate((log2(max(color, 1.0e-6f)) - minEV) / (maxEV - minEV));
  color = AgXDefaultContrastApprox(color);
  color = mul(outset, color);

  // AgX's 2.2 EOTF restores linear light; presentation applies the display transfer.
  return pow(saturate(color), 2.2f);
}
