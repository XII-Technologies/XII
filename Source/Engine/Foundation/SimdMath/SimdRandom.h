#pragma once

#include <Foundation/SimdMath/SimdVec4u.h>

/// \brief Noise based random number generator that generates 4 pseudo random values at once.
///
/// Does not keep any internal state but relies on the user to provide different positions for each call.
/// The seed parameter can be used to further alter the noise function.
struct xiiSimdRandom
{
  /// \brief Returns 4 random uint32 values at position, ie. ranging from 0 to (2 ^ 32) - 1
  static xiiSimdVec4u UInt(const xiiSimdVec4i& position, const xiiSimdVec4u& seed = xiiSimdVec4u::ZeroVector());

  /// \brief Returns 4 random float values in range [0.0 ; 1.0], ie. including zero and one
  static xiiSimdVec4f FloatZeroToOne(const xiiSimdVec4i& position, const xiiSimdVec4u& seed = xiiSimdVec4u::ZeroVector());

  /// \brief Returns 4 random float values in range [fMinValue ; fMaxValue]
  static xiiSimdVec4f FloatMinMax(const xiiSimdVec4i& position, const xiiSimdVec4f& minValue, const xiiSimdVec4f& maxValue, const xiiSimdVec4u& seed = xiiSimdVec4u::ZeroVector());
};

#include <Foundation/SimdMath/Implementation/SimdRandom_inl.h>
