#pragma once

// static
XII_FORCE_INLINE xiiSimdVec4u xiiSimdRandom::UInt(const xiiSimdVec4i& position, const xiiSimdVec4u& seed /*= xiiSimdVec4u::ZeroVector()*/)
{
  // Based on Squirrel3 which was introduced by Squirrel Eiserloh at 'Math for Game Programmers: Noise-Based RNG', GDC17.
  const xiiSimdVec4u BIT_NOISE1 = xiiSimdVec4u(0xb5297a4d);
  const xiiSimdVec4u BIT_NOISE2 = xiiSimdVec4u(0x68e31da4);
  const xiiSimdVec4u BIT_NOISE3 = xiiSimdVec4u(0x1b56c4e9);

  xiiSimdVec4u mangled = xiiSimdVec4u(position);
  mangled              = mangled.CompMul(BIT_NOISE1);
  mangled += seed;
  mangled ^= (mangled >> 8);
  mangled += BIT_NOISE2;
  mangled ^= (mangled << 8);
  mangled = mangled.CompMul(BIT_NOISE3);
  mangled ^= (mangled >> 8);

  return mangled;
}

// static
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdRandom::FloatZeroToOne(const xiiSimdVec4i& position, const xiiSimdVec4u& seed /*= xiiSimdVec4u::ZeroVector()*/)
{
  return UInt(position, seed).ToFloat() * (1.0f / 4294967296.0f);
}

// static
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdRandom::FloatMinMax(const xiiSimdVec4i& position, const xiiSimdVec4f& minValue, const xiiSimdVec4f& maxValue, const xiiSimdVec4u& seed /*= xiiSimdVec4u::ZeroVector()*/)
{
  return xiiSimdVec4f::Lerp(minValue, maxValue, FloatZeroToOne(position, seed));
}
