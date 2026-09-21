/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Math.h>

/// A 8bit per channel color storage format with undefined encoding. It is up to the user to reinterpret as a gamma or linear space
/// color.
///
/// \see xiiColorLinearUB
/// \see xiiColorGammaUB
class XII_FOUNDATION_DLL xiiColorBaseUB
{
public:
  XII_DECLARE_POD_TYPE();

  xiiUInt8 r;
  xiiUInt8 g;
  xiiUInt8 b;
  xiiUInt8 a;

  /// Default-constructed color is uninitialized (for speed)
  xiiColorBaseUB() = default;

  /// Initializes the color with r, g, b, a
  xiiColorBaseUB(xiiUInt8 r, xiiUInt8 g, xiiUInt8 b, xiiUInt8 a = 255);

  /// Conversion to const xiiUInt8*.
  const xiiUInt8* GetData() const { return &r; }

  /// Conversion to xiiUInt8*
  xiiUInt8* GetData() { return &r; }

  /// Packs the 4 color values into a single uint32 with A in the least significant bits and R in the most significant ones.
  [[nodiscard]] xiiUInt32 ToRGBA8() const
  {
    // RGBA (A at lowest address, R at highest).
    return (static_cast<xiiUInt32>(r) << 24) + (static_cast<xiiUInt32>(g) << 16) + (static_cast<xiiUInt32>(b) << 8) + (static_cast<xiiUInt32>(a) << 0);
  }

  /// Packs the 4 color values into a single uint32 with R in the least significant bits and A in the most significant ones.
  [[nodiscard]] xiiUInt32 ToABGR8() const
  {
    // RGBA (A at highest address, R at lowest).
    return (static_cast<xiiUInt32>(a) << 24) + (static_cast<xiiUInt32>(b) << 16) + (static_cast<xiiUInt32>(g) << 8) + (static_cast<xiiUInt32>(r) << 0);
  }
};

static_assert(sizeof(xiiColorBaseUB) == 4);

/// A 8bit per channel unsigned normalized (values interpreted as 0-1) color storage format that represents colors in linear space.
///
/// For any calculations or conversions use xiiColor.
/// \see xiiColor
class XII_FOUNDATION_DLL xiiColorLinearUB : public xiiColorBaseUB
{
public:
  XII_DECLARE_POD_TYPE();

  /// Default-constructed color is uninitialized (for speed)
  xiiColorLinearUB() = default; // [tested]

  /// Initializes the color with r, g, b, a
  xiiColorLinearUB(xiiUInt8 r, xiiUInt8 g, xiiUInt8 b, xiiUInt8 a = 255); // [tested]

  /// Initializes the color with xiiColor.
  /// Assumes that the given color is normalized.
  /// \see xiiColor::IsNormalized
  xiiColorLinearUB(const xiiColor& color); // [tested]

  /// Initializes the color with xiiColor.
  void operator=(const xiiColor& color); // [tested]

  /// Converts this color to xiiColor.
  xiiColor ToLinearFloat() const; // [tested]
};

static_assert(sizeof(xiiColorLinearUB) == 4);

/// A 8bit per channel unsigned normalized (values interpreted as 0-1) color storage format that represents colors in gamma space.
///
/// For any calculations or conversions use xiiColor.
/// \see xiiColor
class XII_FOUNDATION_DLL xiiColorGammaUB : public xiiColorBaseUB
{
public:
  XII_DECLARE_POD_TYPE();

  /// Default-constructed color is uninitialized (for speed)
  xiiColorGammaUB() = default;

  /// Copies the color values. RGB are assumed to be in Gamma space.
  xiiColorGammaUB(xiiUInt8 uiGammaRed, xiiUInt8 uiGammaGreen, xiiUInt8 uiGammaBlue, xiiUInt8 uiLinearAlpha = 255); // [tested]

  /// Initializes the color with xiiColor. Converts the linear space color to gamma space.
  /// Assumes that the given color is normalized.
  /// \see xiiColor::IsNormalized
  xiiColorGammaUB(const xiiColor& color); // [tested]

  /// Initializes the color with xiiColor. Converts the linear space color to gamma space.
  void operator=(const xiiColor& color); // [tested]

  /// Converts this color to xiiColor.
  xiiColor ToLinearFloat() const;

  /// Extracts the values from a uint32 with R at the least significant bits, then G, then B and A at the most significant bits.
  static xiiColorLinearUB MakeFromABGR8(xiiUInt32 value)
  {
    return xiiColorLinearUB(static_cast<xiiUInt8>(value >> 0) & 0xFF,
                            static_cast<xiiUInt8>(value >> 8) & 0xFF,
                            static_cast<xiiUInt8>(value >> 16) & 0xFF,
                            static_cast<xiiUInt8>(value >> 24) & 0xFF);
  }
};

static_assert(sizeof(xiiColorGammaUB) == 4);

#include <Foundation/Math/Implementation/Color8UNorm_inl.h>
