#pragma once

#include <Foundation/Math/Color.h>

/// \brief A color scheme based on https://github.com/yeun/open-color version 1.9.1
///
/// Open Color Goals:
/// All colors will be beautiful in itself and harmonious
/// At the same brightness level, the perceived brightness will be constant
class XII_FOUNDATION_DLL xiiColorScheme
{
public:
  enum Enum
  {
    Red,
    Pink,
    Grape,
    Violet,
    Indigo,
    Blue,
    Cyan,
    Teal,
    Green,
    Lime,
    Yellow,
    Orange,
    Gray,

    Count
  };

  /// \brief Normalization factor for getting colors by index. E.g. xiiColorScheme::Blue * s_fIndexNormalizer would get exactly Blue as color.
  constexpr static float s_fIndexNormalizer = 1.0f / (Count - 2);

  /// \brief Get the scheme color with the given brightness (0..9) and with optional saturation and alpha.
  XII_FORCE_INLINE static xiiColor GetColor(Enum schemeColor, xiiUInt8 uiBrightness, float fSaturation = 1.0f, float fAlpha = 1.0f)
  {
    XII_ASSERT_DEV(uiBrightness <= 9, "Brightness is too large");
    const xiiColor c = s_Colors[schemeColor][uiBrightness];
    const float    l = c.GetLuminance();
    return xiiMath::Lerp(xiiColor(l, l, l), c, fSaturation).WithAlpha(fAlpha);
  }

  /// \brief Get the scheme color using a floating point index instead of the enum. The resulting color will be interpolated between the predefined ones.
  /// Does not include gray.
  static xiiColor GetColor(float fIndex, xiiUInt8 uiBrightness, float fSaturation = 1.0f, float fAlpha = 1.0f);

  /// \brief Get a scheme color with predefined brightness and saturation to look good with the XII tools dark UI scheme.
  XII_ALWAYS_INLINE static xiiColor DarkUI(Enum schemeColor)
  {
    return s_DarkUIColors[schemeColor];
  }

  /// \brief Gets a scheme color by index with predefined brightness and saturation to look good with the XII tools dark UI scheme.
  XII_FORCE_INLINE static xiiColor DarkUI(float fIndex)
  {
    xiiUInt32 uiIndexA, uiIndexB;
    float     fFrac;
    GetInterpolation(fIndex, uiIndexA, uiIndexB, fFrac);

    return xiiMath::Lerp(s_DarkUIColors[uiIndexA], s_DarkUIColors[uiIndexB], fFrac);
  }

  /// \brief Get a scheme color with predefined brightness and saturation to look good as highlight color in XII tools. Can also be used in a 3D scene for e.g. visualizers etc.
  XII_ALWAYS_INLINE static xiiColor LightUI(Enum schemeColor)
  {
    return s_LightUIColors[schemeColor];
  }

  /// \brief Get a scheme color by index with predefined brightness and saturation to look good as highlight color in XII tools. Can also be used in a 3D scene for e.g. visualizers etc.
  XII_FORCE_INLINE static xiiColor LightUI(float fIndex)
  {
    xiiUInt32 uiIndexA, uiIndexB;
    float     fFrac;
    GetInterpolation(fIndex, uiIndexA, uiIndexB, fFrac);

    return xiiMath::Lerp(s_LightUIColors[uiIndexA], s_LightUIColors[uiIndexB], fFrac);
  }

private:
  XII_ALWAYS_INLINE constexpr static void GetInterpolation(float fIndex, xiiUInt32& out_uiIndexA, xiiUInt32& out_uiIndexB, float& out_fFrac)
  {
    fIndex = xiiMath::Saturate(fIndex);

    constexpr xiiUInt32 uiCountWithoutGray = Count - 1;
    constexpr xiiUInt32 uiMaxIndex         = uiCountWithoutGray - 1;
    out_uiIndexA                           = xiiUInt32(fIndex * uiMaxIndex);
    out_uiIndexB                           = (out_uiIndexA + 1) % uiCountWithoutGray;
    out_fFrac                              = (fIndex * uiMaxIndex) - out_uiIndexA;
  }

  static xiiColor s_Colors[Count][10];
  static xiiColor s_DarkUIColors[Count];
  static xiiColor s_LightUIColors[Count];
};
