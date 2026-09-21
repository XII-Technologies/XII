/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Math.h>
#include <Foundation/Math/Vec4.h>

/// xiiColor represents an RGBA color in linear color space. Values are stored as float, allowing HDR values and full precision color
/// modifications.
///
/// xiiColor is the central class to handle colors throughout the engine. With floating point precision it can handle any value, including HDR colors.
/// Since it is stored in linear space, doing color transformations (e.g. adding colors or multiplying them) work as expected.
///
/// When you need to pass colors to the GPU you have multiple options.
///   * If you can spare the bandwidth, you should prefer to use floating point formats, e.g. the same as xiiColor on the CPU.
///   * If you need higher precision and HDR values, you can use xiiColorLinear16f as a storage format with only half the memory footprint.
///   * If you need to preserve memory and LDR values are sufficient, you should use xiiColorGammaUB. This format uses 8 Bit per pixel
///     but stores colors in Gamma space, resulting in higher precision in the range that the human eye can distinguish better.
///     However, when you store a color in Gamma space, you need to make sure to convert it back to linear space before doing ANY computations
///     with it. E.g. your shader needs to convert the color.
///   * You can also use 8 Bit per pixel with a linear color space by using xiiColorLinearUB, however this may give very noticeable precision loss.
///
/// When working with color in your code, be aware to always use the correct class to handle color conversions properly.
/// E.g. when you hardcode a color in source code, you might go to a Paint program, pick a nice color and then type that value into the
/// source code. Note that ALL colors that you see on screen are implicitly in sRGB / Gamma space. That means you should do the following cast:\n
///
/// \code
///   xiiColor linear = xiiColorGammaUB(100, 149, 237);
/// \endcode
///
/// This will automatically convert the color from Gamma to linear space. From there on all mathematical operations are possible.
///
/// The inverse has to be done when you want to present the value of a color in a UI:
///
/// \code
///   xiiColorGammaUB gamma = xiiColor(0.39f, 0.58f, 0.93f);
/// \endcode
///
/// Now the integer values in \a gamma can be used to e.g. populate a color picker and the color displayed on screen will show up the same, as
/// in a gamma correct 3D rendering.
///
///
///
/// The predefined colors can be seen at http://www.w3schools.com/colors/colors_names.asp
class XII_FOUNDATION_DLL xiiColor
{
public:
  XII_DECLARE_POD_TYPE();

  // *** Predefined Colors ***
public:
  static const xiiColor AliceBlue;            ///< #F0F8FF
  static const xiiColor AntiqueWhite;         ///< #FAEBD7
  static const xiiColor Aqua;                 ///< #00FFFF
  static const xiiColor Aquamarine;           ///< #7FFFD4
  static const xiiColor Azure;                ///< #F0FFFF
  static const xiiColor Beige;                ///< #F5F5DC
  static const xiiColor Bisque;               ///< #FFE4C4
  static const xiiColor Black;                ///< #000000
  static const xiiColor BlanchedAlmond;       ///< #FFEBCD
  static const xiiColor Blue;                 ///< #0000FF
  static const xiiColor BlueViolet;           ///< #8A2BE2
  static const xiiColor Brown;                ///< #A52A2A
  static const xiiColor BurlyWood;            ///< #DEB887
  static const xiiColor CadetBlue;            ///< #5F9EA0
  static const xiiColor Chartreuse;           ///< #7FFF00
  static const xiiColor Chocolate;            ///< #D2691E
  static const xiiColor Coral;                ///< #FF7F50
  static const xiiColor CornflowerBlue;       ///< #6495ED  The original!
  static const xiiColor Cornsilk;             ///< #FFF8DC
  static const xiiColor Crimson;              ///< #DC143C
  static const xiiColor Cyan;                 ///< #00FFFF
  static const xiiColor DarkBlue;             ///< #00008B
  static const xiiColor DarkCyan;             ///< #008B8B
  static const xiiColor DarkGoldenRod;        ///< #B8860B
  static const xiiColor DarkGray;             ///< #A9A9A9
  static const xiiColor DarkGrey;             ///< #A9A9A9
  static const xiiColor DarkGreen;            ///< #006400
  static const xiiColor DarkKhaki;            ///< #BDB76B
  static const xiiColor DarkMagenta;          ///< #8B008B
  static const xiiColor DarkOliveGreen;       ///< #556B2F
  static const xiiColor DarkOrange;           ///< #FF8C00
  static const xiiColor DarkOrchid;           ///< #9932CC
  static const xiiColor DarkRed;              ///< #8B0000
  static const xiiColor DarkSalmon;           ///< #E9967A
  static const xiiColor DarkSeaGreen;         ///< #8FBC8F
  static const xiiColor DarkSlateBlue;        ///< #483D8B
  static const xiiColor DarkSlateGray;        ///< #2F4F4F
  static const xiiColor DarkSlateGrey;        ///< #2F4F4F
  static const xiiColor DarkTurquoise;        ///< #00CED1
  static const xiiColor DarkViolet;           ///< #9400D3
  static const xiiColor DeepPink;             ///< #FF1493
  static const xiiColor DeepSkyBlue;          ///< #00BFFF
  static const xiiColor DimGray;              ///< #696969
  static const xiiColor DimGrey;              ///< #696969
  static const xiiColor DodgerBlue;           ///< #1E90FF
  static const xiiColor FireBrick;            ///< #B22222
  static const xiiColor FloralWhite;          ///< #FFFAF0
  static const xiiColor ForestGreen;          ///< #228B22
  static const xiiColor Fuchsia;              ///< #FF00FF
  static const xiiColor Gainsboro;            ///< #DCDCDC
  static const xiiColor GhostWhite;           ///< #F8F8FF
  static const xiiColor Gold;                 ///< #FFD700
  static const xiiColor GoldenRod;            ///< #DAA520
  static const xiiColor Gray;                 ///< #808080
  static const xiiColor Grey;                 ///< #808080
  static const xiiColor Green;                ///< #008000
  static const xiiColor GreenYellow;          ///< #ADFF2F
  static const xiiColor HoneyDew;             ///< #F0FFF0
  static const xiiColor HotPink;              ///< #FF69B4
  static const xiiColor IndianRed;            ///< #CD5C5C
  static const xiiColor Indigo;               ///< #4B0082
  static const xiiColor Ivory;                ///< #FFFFF0
  static const xiiColor Khaki;                ///< #F0E68C
  static const xiiColor Lavender;             ///< #E6E6FA
  static const xiiColor LavenderBlush;        ///< #FFF0F5
  static const xiiColor LawnGreen;            ///< #7CFC00
  static const xiiColor LemonChiffon;         ///< #FFFACD
  static const xiiColor LightBlue;            ///< #ADD8E6
  static const xiiColor LightCoral;           ///< #F08080
  static const xiiColor LightCyan;            ///< #E0FFFF
  static const xiiColor LightGoldenRodYellow; ///< #FAFAD2
  static const xiiColor LightGray;            ///< #D3D3D3
  static const xiiColor LightGrey;            ///< #D3D3D3
  static const xiiColor LightGreen;           ///< #90EE90
  static const xiiColor LightPink;            ///< #FFB6C1
  static const xiiColor LightSalmon;          ///< #FFA07A
  static const xiiColor LightSeaGreen;        ///< #20B2AA
  static const xiiColor LightSkyBlue;         ///< #87CEFA
  static const xiiColor LightSlateGray;       ///< #778899
  static const xiiColor LightSlateGrey;       ///< #778899
  static const xiiColor LightSteelBlue;       ///< #B0C4DE
  static const xiiColor LightYellow;          ///< #FFFFE0
  static const xiiColor Lime;                 ///< #00FF00
  static const xiiColor LimeGreen;            ///< #32CD32
  static const xiiColor Linen;                ///< #FAF0E6
  static const xiiColor Magenta;              ///< #FF00FF
  static const xiiColor Maroon;               ///< #800000
  static const xiiColor MediumAquaMarine;     ///< #66CDAA
  static const xiiColor MediumBlue;           ///< #0000CD
  static const xiiColor MediumOrchid;         ///< #BA55D3
  static const xiiColor MediumPurple;         ///< #9370DB
  static const xiiColor MediumSeaGreen;       ///< #3CB371
  static const xiiColor MediumSlateBlue;      ///< #7B68EE
  static const xiiColor MediumSpringGreen;    ///< #00FA9A
  static const xiiColor MediumTurquoise;      ///< #48D1CC
  static const xiiColor MediumVioletRed;      ///< #C71585
  static const xiiColor MidnightBlue;         ///< #191970
  static const xiiColor MintCream;            ///< #F5FFFA
  static const xiiColor MistyRose;            ///< #FFE4E1
  static const xiiColor Moccasin;             ///< #FFE4B5
  static const xiiColor NavajoWhite;          ///< #FFDEAD
  static const xiiColor Navy;                 ///< #000080
  static const xiiColor OldLace;              ///< #FDF5E6
  static const xiiColor Olive;                ///< #808000
  static const xiiColor OliveDrab;            ///< #6B8E23
  static const xiiColor Orange;               ///< #FFA500
  static const xiiColor OrangeRed;            ///< #FF4500
  static const xiiColor Orchid;               ///< #DA70D6
  static const xiiColor PaleGoldenRod;        ///< #EEE8AA
  static const xiiColor PaleGreen;            ///< #98FB98
  static const xiiColor PaleTurquoise;        ///< #AFEEEE
  static const xiiColor PaleVioletRed;        ///< #DB7093
  static const xiiColor PapayaWhip;           ///< #FFEFD5
  static const xiiColor PeachPuff;            ///< #FFDAB9
  static const xiiColor Peru;                 ///< #CD853F
  static const xiiColor Pink;                 ///< #FFC0CB
  static const xiiColor Plum;                 ///< #DDA0DD
  static const xiiColor PowderBlue;           ///< #B0E0E6
  static const xiiColor Purple;               ///< #800080
  static const xiiColor RebeccaPurple;        ///< #663399
  static const xiiColor Red;                  ///< #FF0000
  static const xiiColor RosyBrown;            ///< #BC8F8F
  static const xiiColor RoyalBlue;            ///< #4169E1
  static const xiiColor SaddleBrown;          ///< #8B4513
  static const xiiColor Salmon;               ///< #FA8072
  static const xiiColor SandyBrown;           ///< #F4A460
  static const xiiColor SeaGreen;             ///< #2E8B57
  static const xiiColor SeaShell;             ///< #FFF5EE
  static const xiiColor Sienna;               ///< #A0522D
  static const xiiColor Silver;               ///< #C0C0C0
  static const xiiColor SkyBlue;              ///< #87CEEB
  static const xiiColor SlateBlue;            ///< #6A5ACD
  static const xiiColor SlateGray;            ///< #708090
  static const xiiColor SlateGrey;            ///< #708090
  static const xiiColor Snow;                 ///< #FFFAFA
  static const xiiColor SpringGreen;          ///< #00FF7F
  static const xiiColor SteelBlue;            ///< #4682B4
  static const xiiColor Tan;                  ///< #D2B48C
  static const xiiColor Teal;                 ///< #008080
  static const xiiColor Thistle;              ///< #D8BFD8
  static const xiiColor Tomato;               ///< #FF6347
  static const xiiColor Turquoise;            ///< #40E0D0
  static const xiiColor Violet;               ///< #EE82EE
  static const xiiColor Wheat;                ///< #F5DEB3
  static const xiiColor White;                ///< #FFFFFF
  static const xiiColor WhiteSmoke;           ///< #F5F5F5
  static const xiiColor Yellow;               ///< #FFFF00
  static const xiiColor YellowGreen;          ///< #9ACD32

  // *** Data ***
public:
  float r;
  float g;
  float b;
  float a;

  // *** Static Functions ***
public:
  /// Returns a color with all four RGBA components set to Not-A-Number (NaN).
  [[nodiscard]] static xiiColor MakeNaN();

  /// Returns a color with all four RGBA components set to zero. This is different to xiiColor::Black, which has alpha still set to 1.0.
  [[nodiscard]] static xiiColor MakeZero();

  /// Returns a color with the given r, g, b, a values. The values must be given in a linear color space.
  [[nodiscard]] static xiiColor MakeRGBA(float fLinearRed, float fLinearGreen, float fLinearBlue, float fLinearAlpha = 1.0f);

  // *** Constructors ***
public:
  /// default-constructed color is uninitialized (for speed)
  xiiColor(); // [tested]

  /// Initializes the color with r, g, b, a. The color values must be given in a linear color space.
  ///
  /// To initialize the color from a Gamma color space, e.g. when using a color value that was determined with a color picker,
  /// use the constructor that takes a xiiColorGammaUB object for initialization.
  constexpr xiiColor(float fLinearRed, float fLinearGreen, float fLinearBlue, float fLinearAlpha = 1.0f); // [tested]

  /// Initializes this color from a xiiColorLinearUB object.
  ///
  /// Prefer to either use linear colors with floating point precision, or to use xiiColorGammaUB for 8 bit per pixel colors in gamma space.
  xiiColor(const xiiColorLinearUB& cc); // [tested]

  /// Initializes this color from a xiiColorGammaUB object.
  ///
  /// This should be the preferred method when hard-coding colors in source code.
  xiiColor(const xiiColorGammaUB& cc); // [tested]

#if XII_ENABLED(XII_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    XII_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please check that "
                                "all code-paths properly initialize this object.");
  }
#endif

  /// Sets the RGB components, ignores alpha.
  void SetRGB(float fLinearRed, float fLinearGreen, float fLinearBlue); // [tested]

  /// Sets all four RGBA components.
  void SetRGBA(float fLinearRed, float fLinearGreen, float fLinearBlue, float fLinearAlpha = 1.0f); // [tested]

  // *** Conversion Operators/Functions ***
public:
  /// Returns a color created from the kelvin temperature. https://wikipedia.org/wiki/Color_temperature
  /// Originally inspired from https://tannerhelland.com/2012/09/18/convert-temperature-rgb-algorithm-code.html
  /// But with heavy modification to better fit the mapping shown out in https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
  /// Physically accurate clipping points are 6580K for Red and 6560K for G and B. but approximated to 6570k for all to give a better mapping.
  [[nodiscard]] static xiiColor MakeFromKelvin(xiiUInt32 uiKelvin);

  /// Sets this color from a HSV (hue, saturation, value) format.
  ///
  /// \a hue is in range [0; 360], \a sat and \a val are in range [0; 1]
  [[nodiscard]] static xiiColor MakeHSV(float fHue, float fSat, float fVal); // [tested]

  /// Converts the color part to HSV format.
  ///
  /// \a hue is in range [0; 360], \a sat and \a val are in range [0; 1]
  void GetHSV(float& out_fHue, float& out_fSat, float& out_fValue) const; // [tested]

  /// Conversion to const float*
  const float* GetData() const { return &r; }

  /// Conversion to float*
  float* GetData() { return &r; }

  /// Returns the 4 color values packed in a xiiVec4
  const xiiVec4 GetAsVec4() const;

  /// Helper function to convert a float color value from gamma space to linear color space.
  static float GammaToLinear(float fGamma); // [tested]
  /// Helper function to convert a float color value from linear space to gamma color space.
  static float LinearToGamma(float fGamma); // [tested]

  /// Helper function to convert a float RGB color value from gamma space to linear color space.
  static xiiVec3 GammaToLinear(const xiiVec3& vGamma); // [tested]
  /// Helper function to convert a float RGB color value from linear space to gamma color space.
  static xiiVec3 LinearToGamma(const xiiVec3& vGamma); // [tested]

  // *** Color specific functions ***
public:
  /// Returns if the color is in the Range [0; 1] on all 4 channels.
  bool IsNormalized() const; // [tested]

  /// Calculates the average of the RGB channels.
  float CalcAverageRGB() const;

  /// Computes saturation.
  float GetSaturation() const; // [tested]

  /// Computes the perceived luminance. Assumes linear color space (http://en.wikipedia.org/wiki/Luminance_%28relative%29).
  float GetLuminance() const; /// [tested]

  /// Performs a simple (1.0 - color) inversion on all four channels.
  ///
  /// Using this function on non-normalized colors will lead to negative results.
  /// \see xiiColor IsNormalized
  xiiColor GetInvertedColor() const; // [tested]

  /// Calculates the complementary color for this color (hue shifted by 180 degrees). The complementary color will have the same alpha.
  xiiColor GetComplementaryColor() const; // [tested]

  /// Multiplies the given factor into red, green and blue, but not alpha.
  void ScaleRGB(float fFactor);

  /// Multiplies the given factor into red, green, blue and also alpha.
  void ScaleRGBA(float fFactor);

  /// Returns 1 for an LDR color (all ´RGB components < 1). Otherwise the value of the largest component. Ignores alpha.
  float ComputeHdrMultiplier() const;

  /// Returns the base-2 logarithm of ComputeHdrMultiplier().
  /// 0 for LDR colors, +1, +2, etc. for HDR colors.
  float ComputeHdrExposureValue() const;

  /// Raises 2 to the power \a ev and multiplies RGB with that factor.
  void ApplyHdrExposureValue(float fEv);

  /// If this is an HDR color, the largest component value is used to normalize RGB to LDR range. Alpha is unaffected.
  void NormalizeToLdrRange();

  /// Returns a darker color by converting the color to HSV, dividing the *value* by fFactor and converting it back.
  xiiColor GetDarker(float fFactor = 2.0f) const;

  // *** Numeric properties ***
public:
  /// Returns true, if any of \a r, \a g, \a b or \a a is NaN.
  bool IsNaN() const; // [tested]

  /// Checks that all components are finite numbers.
  bool IsValid() const; // [tested]

  // *** Operators ***
public:
  /// Converts the color from xiiColorLinearUB to linear float values.
  void operator=(const xiiColorLinearUB& cc); // [tested]

  /// Converts the color from xiiColorGammaUB to linear float values. Gamma is correctly converted to linear space.
  void operator=(const xiiColorGammaUB& cc); // [tested]

  /// Adds \a rhs component-wise to this color.
  void operator+=(const xiiColor& rhs); // [tested]

  /// Subtracts \a rhs component-wise from this vector.
  void operator-=(const xiiColor& rhs); // [tested]

  /// Multiplies \a rhs component-wise with this color.
  void operator*=(const xiiColor& rhs); // [tested]

  /// Multiplies all components of this color with f.
  void operator*=(float f); // [tested]

  /// Divides all components of this color by f.
  void operator/=(float f); // [tested]

  /// Transforms the RGB components by the matrix. Alpha has no influence on the computation and will stay unmodified. The fourth row of the
  /// matrix is ignored.
  ///
  /// This operation can be used to do basic color correction.
  void operator*=(const xiiMat4& rhs); // [tested]

  /// Equality Check (bitwise). Only compares RGB, ignores Alpha.
  bool IsIdenticalRGB(const xiiColor& rhs) const; // [tested]

  /// Equality Check (bitwise). Compares all four components.
  bool IsIdenticalRGBA(const xiiColor& rhs) const; // [tested]

  /// Equality Check with epsilon. Only compares RGB, ignores Alpha.
  bool IsEqualRGB(const xiiColor& rhs, float fEpsilon) const; // [tested]

  /// Equality Check with epsilon. Compares all four components.
  bool IsEqualRGBA(const xiiColor& rhs, float fEpsilon) const; // [tested]

  /// Returns the current color but with changes the alpha value to the given value.
  xiiColor WithAlpha(float fAlpha) const;

  /// Packs the 4 color values as uint8 into a single uint32 with A in the least significant bits and R in the most significant ones.
  [[nodiscard]] xiiUInt32 ToRGBA8() const;

  /// Packs the 4 color values as uint8 into a single uint32 with R in the least significant bits and A in the most significant ones.
  [[nodiscard]] xiiUInt32 ToABGR8() const;
};

// *** Operators ***

/// Component-wise addition.
const xiiColor operator+(const xiiColor& c1, const xiiColor& c2); // [tested]

/// Component-wise subtraction.
const xiiColor operator-(const xiiColor& c1, const xiiColor& c2); // [tested]

/// Component-wise multiplication.
const xiiColor operator*(const xiiColor& c1, const xiiColor& c2); // [tested]

/// Returns a scaled color.
const xiiColor operator*(float f, const xiiColor& c); // [tested]

/// Returns a scaled color. Will scale all components.
const xiiColor operator*(const xiiColor& c, float f); // [tested]

/// Returns a scaled color. Will scale all components.
const xiiColor operator/(const xiiColor& c, float f); // [tested]

/// Transforms the RGB components by the matrix. Alpha has no influence on the computation and will stay unmodified. The fourth row of the
/// matrix is ignored.
///
/// This operation can be used to do basic color correction.
const xiiColor operator*(const xiiMat4& lhs, const xiiColor& rhs); // [tested]

/// Returns true, if both colors are identical in all components.
bool operator==(const xiiColor& c1, const xiiColor& c2); // [tested]

/// Strict weak ordering. Useful for sorting colors into a map.
bool operator<(const xiiColor& c1, const xiiColor& c2); // [tested]

static_assert(sizeof(xiiColor) == 16);

#include <Foundation/Math/Implementation/Color_inl.h>
