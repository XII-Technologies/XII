/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

// static
XII_FORCE_INLINE xiiSimdVec4u xiiSimdRandom::UInt(const xiiSimdVec4i& vPosition, const xiiSimdVec4u& vSeed /*= xiiSimdVec4u::MakeZero()*/)
{
  // Based on Squirrel3 which was introduced by Squirrel Eiserloh at 'Math for Game Programmers: Noise-Based RNG', GDC17.
  const xiiSimdVec4u BIT_NOISE1 = xiiSimdVec4u(0xb5297a4d);
  const xiiSimdVec4u BIT_NOISE2 = xiiSimdVec4u(0x68e31da4);
  const xiiSimdVec4u BIT_NOISE3 = xiiSimdVec4u(0x1b56c4e9);

  xiiSimdVec4u mangled = xiiSimdVec4u(vPosition);
  mangled              = mangled.CompMul(BIT_NOISE1);
  mangled += vSeed;
  mangled ^= (mangled >> 8);
  mangled += BIT_NOISE2;
  mangled ^= (mangled << 8);
  mangled = mangled.CompMul(BIT_NOISE3);
  mangled ^= (mangled >> 8);

  return mangled;
}

// static
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdRandom::FloatZeroToOne(const xiiSimdVec4i& vPosition, const xiiSimdVec4u& vSeed /*= xiiSimdVec4u::MakeZero()*/)
{
  return UInt(vPosition, vSeed).ToFloat() * (1.0f / 4294967296.0f);
}

// static
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdRandom::FloatMinMax(const xiiSimdVec4i& vPosition, const xiiSimdVec4f& vMinValue, const xiiSimdVec4f& vMaxValue, const xiiSimdVec4u& vSeed /*= xiiSimdVec4u::MakeZero()*/)
{
  return xiiSimdVec4f::Lerp(vMinValue, vMaxValue, FloatZeroToOne(vPosition, vSeed));
}
