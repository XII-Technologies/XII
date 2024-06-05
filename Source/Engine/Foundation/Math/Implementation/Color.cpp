#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Mat4.h>

// ****** xiiColor ******

xiiColor xiiColor::MakeNaN()
{
  return xiiColor(xiiMath::NaN<float>(), xiiMath::NaN<float>(), xiiMath::NaN<float>(), xiiMath::NaN<float>());
}

xiiColor xiiColor::MakeZero()
{
  return xiiColor(0.0f, 0.0f, 0.0f, 0.0f);
}

xiiColor xiiColor::MakeRGBA(float fLinearRed, float fLinearGreen, float fLinearBlue, float fLinearAlpha /*= 1.0f*/)
{
  return xiiColor(fLinearRed, fLinearGreen, fLinearBlue, fLinearAlpha);
}

void xiiColor::operator=(const xiiColorLinearUB& cc)
{
  *this = cc.ToLinearFloat();
}

void xiiColor::operator=(const xiiColorGammaUB& cc)
{
  *this = cc.ToLinearFloat();
}

bool xiiColor::IsNormalized() const
{
  XII_NAN_ASSERT(this);

  return r <= 1.0f && g <= 1.0f && b <= 1.0f && a <= 1.0f && r >= 0.0f && g >= 0.0f && b >= 0.0f && a >= 0.0f;
}


float xiiColor::CalcAverageRGB() const
{
  return (1.0f / 3.0f) * (r + g + b);
}

// http://en.literateprograms.org/RGB_to_HSV_color_space_conversion_%28C%29
void xiiColor::GetHSV(float& out_fHue, float& out_fSat, float& out_fValue) const
{
  // The formula below assumes values in gamma space
  const float r2 = LinearToGamma(r);
  const float g2 = LinearToGamma(g);
  const float b2 = LinearToGamma(b);

  out_fValue = xiiMath::Max(r2, g2, b2); // Value

  if (out_fValue < xiiMath::SmallEpsilon<float>())
  {
    out_fHue   = 0.0f;
    out_fSat   = 0.0f;
    out_fValue = 0.0f;
    return;
  }

  const float invV    = 1.0f / out_fValue;
  float       norm_r  = r2 * invV;
  float       norm_g  = g2 * invV;
  float       norm_b  = b2 * invV;
  float       rgb_min = xiiMath::Min(norm_r, norm_g, norm_b);
  float       rgb_max = xiiMath::Max(norm_r, norm_g, norm_b);

  out_fSat = rgb_max - rgb_min; // Saturation

  if (out_fSat == 0)
  {
    out_fHue = 0;
    return;
  }

  // Normalize saturation
  const float rgb_delta_inv = 1.0f / (rgb_max - rgb_min);
  norm_r                    = (norm_r - rgb_min) * rgb_delta_inv;
  norm_g                    = (norm_g - rgb_min) * rgb_delta_inv;
  norm_b                    = (norm_b - rgb_min) * rgb_delta_inv;
  rgb_max                   = xiiMath::Max(norm_r, norm_g, norm_b);

  // hue
  if (rgb_max == norm_r)
  {
    out_fHue = 60.0f * (norm_g - norm_b);

    if (out_fHue < 0.0f)
      out_fHue += 360.0f;
  }
  else if (rgb_max == norm_g)
    out_fHue = 120.0f + 60.0f * (norm_b - norm_r);
  else
    out_fHue = 240.0f + 60.0f * (norm_r - norm_g);
}

// http://www.rapidtables.com/convert/color/hsv-to-rgb.htm
xiiColor xiiColor::MakeHSV(float fHue, float fSat, float fVal)
{
  XII_ASSERT_DEBUG(fHue <= 360 && fHue >= 0, "HSV 'hue' is in invalid range.");
  XII_ASSERT_DEBUG(fSat <= 1 && fVal >= 0, "HSV 'saturation' is in invalid range.");
  XII_ASSERT_DEBUG(fVal >= 0, "HSV 'value' is in invalid range.");

  float c = fSat * fVal;
  float x = c * (1.0f - xiiMath::Abs(xiiMath::Mod(fHue / 60.0f, 2) - 1.0f));
  float m = fVal - c;

  xiiColor res;
  res.a = 1.0f;

  if (fHue < 60)
  {
    res.r = c + m;
    res.g = x + m;
    res.b = 0 + m;
  }
  else if (fHue < 120)
  {
    res.r = x + m;
    res.g = c + m;
    res.b = 0 + m;
  }
  else if (fHue < 180)
  {
    res.r = 0 + m;
    res.g = c + m;
    res.b = x + m;
  }
  else if (fHue < 240)
  {
    res.r = 0 + m;
    res.g = x + m;
    res.b = c + m;
  }
  else if (fHue < 300)
  {
    res.r = x + m;
    res.g = 0 + m;
    res.b = c + m;
  }
  else
  {
    res.r = c + m;
    res.g = 0 + m;
    res.b = x + m;
  }

  // The formula above produces value in gamma space
  res.r = GammaToLinear(res.r);
  res.g = GammaToLinear(res.g);
  res.b = GammaToLinear(res.b);

  return res;
}

float xiiColor::GetSaturation() const
{
  float hue, sat, val;
  GetHSV(hue, sat, val);

  return sat;
}

bool xiiColor::IsValid() const
{
  if (!xiiMath::IsFinite(r))
    return false;
  if (!xiiMath::IsFinite(g))
    return false;
  if (!xiiMath::IsFinite(b))
    return false;
  if (!xiiMath::IsFinite(a))
    return false;

  return true;
}

bool xiiColor::IsEqualRGB(const xiiColor& rhs, float fEpsilon) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return (xiiMath::IsEqual(r, rhs.r, fEpsilon) && xiiMath::IsEqual(g, rhs.g, fEpsilon) && xiiMath::IsEqual(b, rhs.b, fEpsilon));
}

bool xiiColor::IsEqualRGBA(const xiiColor& rhs, float fEpsilon) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return (xiiMath::IsEqual(r, rhs.r, fEpsilon) && xiiMath::IsEqual(g, rhs.g, fEpsilon) && xiiMath::IsEqual(b, rhs.b, fEpsilon) &&
          xiiMath::IsEqual(a, rhs.a, fEpsilon));
}

void xiiColor::operator/=(float f)
{
  float f_inv = 1.0f / f;
  r *= f_inv;
  g *= f_inv;
  b *= f_inv;
  a *= f_inv;

  XII_NAN_ASSERT(this);
}

void xiiColor::operator*=(const xiiMat4& rhs)
{
  xiiVec3 v(r, g, b);
  v = rhs.TransformPosition(v);

  r = v.x;
  g = v.y;
  b = v.z;
}


void xiiColor::ScaleRGB(float fFactor)
{
  r *= fFactor;
  g *= fFactor;
  b *= fFactor;
}

void xiiColor::ScaleRGBA(float fFactor)
{
  r *= fFactor;
  g *= fFactor;
  b *= fFactor;
  a *= fFactor;
}

float xiiColor::ComputeHdrMultiplier() const
{
  return xiiMath::Max(1.0f, r, g, b);
}

float xiiColor::ComputeHdrExposureValue() const
{
  return xiiMath::Log2(ComputeHdrMultiplier());
}

void xiiColor::ApplyHdrExposureValue(float fEv)
{
  const float factor = xiiMath::Pow2(fEv);
  r *= factor;
  g *= factor;
  b *= factor;
}


void xiiColor::NormalizeToLdrRange()
{
  ScaleRGB(1.0f / ComputeHdrMultiplier());
}

xiiColor xiiColor::GetDarker(float fFactor /*= 2.0f*/) const
{
  float h, s, v;
  GetHSV(h, s, v);

  return xiiColor::MakeHSV(h, s, v / fFactor);
}

xiiColor xiiColor::GetComplementaryColor() const
{
  float hue, sat, val;
  GetHSV(hue, sat, val);

  xiiColor Shifted = xiiColor::MakeHSV(xiiMath::Mod(hue + 180.0f, 360.0f), sat, val);
  Shifted.a        = a;

  return Shifted;
}

const xiiVec4 xiiColor::GetAsVec4() const
{
  return xiiVec4(r, g, b, a);
}

float xiiColor::GammaToLinear(float fGamma)
{
  return fGamma <= 0.04045f ? (fGamma / 12.92f) : (xiiMath::Pow((fGamma + 0.055f) / 1.055f, 2.4f));
}

float xiiColor::LinearToGamma(float fLinear)
{
  // assuming we have linear color (not CIE xyY or CIE XYZ)
  return fLinear <= 0.0031308f ? (12.92f * fLinear) : (1.055f * xiiMath::Pow(fLinear, 1.0f / 2.4f) - 0.055f);
}

xiiVec3 xiiColor::GammaToLinear(const xiiVec3& vGamma)
{
  return xiiVec3(GammaToLinear(vGamma.x), GammaToLinear(vGamma.y), GammaToLinear(vGamma.z));
}

xiiVec3 xiiColor::LinearToGamma(const xiiVec3& vLinear)
{
  // assuming we have linear color (not CIE xyY or CIE XYZ)
  return xiiVec3(LinearToGamma(vLinear.x), LinearToGamma(vLinear.y), LinearToGamma(vLinear.z));
}

const xiiColor xiiColor::AliceBlue(xiiColorGammaUB(0xF0, 0xF8, 0xFF));
const xiiColor xiiColor::AntiqueWhite(xiiColorGammaUB(0xFA, 0xEB, 0xD7));
const xiiColor xiiColor::Aqua(xiiColorGammaUB(0x00, 0xFF, 0xFF));
const xiiColor xiiColor::Aquamarine(xiiColorGammaUB(0x7F, 0xFF, 0xD4));
const xiiColor xiiColor::Azure(xiiColorGammaUB(0xF0, 0xFF, 0xFF));
const xiiColor xiiColor::Beige(xiiColorGammaUB(0xF5, 0xF5, 0xDC));
const xiiColor xiiColor::Bisque(xiiColorGammaUB(0xFF, 0xE4, 0xC4));
const xiiColor xiiColor::Black(xiiColorGammaUB(0x00, 0x00, 0x00));
const xiiColor xiiColor::BlanchedAlmond(xiiColorGammaUB(0xFF, 0xEB, 0xCD));
const xiiColor xiiColor::Blue(xiiColorGammaUB(0x00, 0x00, 0xFF));
const xiiColor xiiColor::BlueViolet(xiiColorGammaUB(0x8A, 0x2B, 0xE2));
const xiiColor xiiColor::Brown(xiiColorGammaUB(0xA5, 0x2A, 0x2A));
const xiiColor xiiColor::BurlyWood(xiiColorGammaUB(0xDE, 0xB8, 0x87));
const xiiColor xiiColor::CadetBlue(xiiColorGammaUB(0x5F, 0x9E, 0xA0));
const xiiColor xiiColor::Chartreuse(xiiColorGammaUB(0x7F, 0xFF, 0x00));
const xiiColor xiiColor::Chocolate(xiiColorGammaUB(0xD2, 0x69, 0x1E));
const xiiColor xiiColor::Coral(xiiColorGammaUB(0xFF, 0x7F, 0x50));
const xiiColor xiiColor::CornflowerBlue(xiiColorGammaUB(0x64, 0x95, 0xED)); // The Original!
const xiiColor xiiColor::Cornsilk(xiiColorGammaUB(0xFF, 0xF8, 0xDC));
const xiiColor xiiColor::Crimson(xiiColorGammaUB(0xDC, 0x14, 0x3C));
const xiiColor xiiColor::Cyan(xiiColorGammaUB(0x00, 0xFF, 0xFF));
const xiiColor xiiColor::DarkBlue(xiiColorGammaUB(0x00, 0x00, 0x8B));
const xiiColor xiiColor::DarkCyan(xiiColorGammaUB(0x00, 0x8B, 0x8B));
const xiiColor xiiColor::DarkGoldenRod(xiiColorGammaUB(0xB8, 0x86, 0x0B));
const xiiColor xiiColor::DarkGray(xiiColorGammaUB(0xA9, 0xA9, 0xA9));
const xiiColor xiiColor::DarkGrey(xiiColorGammaUB(0xA9, 0xA9, 0xA9));
const xiiColor xiiColor::DarkGreen(xiiColorGammaUB(0x00, 0x64, 0x00));
const xiiColor xiiColor::DarkKhaki(xiiColorGammaUB(0xBD, 0xB7, 0x6B));
const xiiColor xiiColor::DarkMagenta(xiiColorGammaUB(0x8B, 0x00, 0x8B));
const xiiColor xiiColor::DarkOliveGreen(xiiColorGammaUB(0x55, 0x6B, 0x2F));
const xiiColor xiiColor::DarkOrange(xiiColorGammaUB(0xFF, 0x8C, 0x00));
const xiiColor xiiColor::DarkOrchid(xiiColorGammaUB(0x99, 0x32, 0xCC));
const xiiColor xiiColor::DarkRed(xiiColorGammaUB(0x8B, 0x00, 0x00));
const xiiColor xiiColor::DarkSalmon(xiiColorGammaUB(0xE9, 0x96, 0x7A));
const xiiColor xiiColor::DarkSeaGreen(xiiColorGammaUB(0x8F, 0xBC, 0x8F));
const xiiColor xiiColor::DarkSlateBlue(xiiColorGammaUB(0x48, 0x3D, 0x8B));
const xiiColor xiiColor::DarkSlateGray(xiiColorGammaUB(0x2F, 0x4F, 0x4F));
const xiiColor xiiColor::DarkSlateGrey(xiiColorGammaUB(0x2F, 0x4F, 0x4F));
const xiiColor xiiColor::DarkTurquoise(xiiColorGammaUB(0x00, 0xCE, 0xD1));
const xiiColor xiiColor::DarkViolet(xiiColorGammaUB(0x94, 0x00, 0xD3));
const xiiColor xiiColor::DeepPink(xiiColorGammaUB(0xFF, 0x14, 0x93));
const xiiColor xiiColor::DeepSkyBlue(xiiColorGammaUB(0x00, 0xBF, 0xFF));
const xiiColor xiiColor::DimGray(xiiColorGammaUB(0x69, 0x69, 0x69));
const xiiColor xiiColor::DimGrey(xiiColorGammaUB(0x69, 0x69, 0x69));
const xiiColor xiiColor::DodgerBlue(xiiColorGammaUB(0x1E, 0x90, 0xFF));
const xiiColor xiiColor::FireBrick(xiiColorGammaUB(0xB2, 0x22, 0x22));
const xiiColor xiiColor::FloralWhite(xiiColorGammaUB(0xFF, 0xFA, 0xF0));
const xiiColor xiiColor::ForestGreen(xiiColorGammaUB(0x22, 0x8B, 0x22));
const xiiColor xiiColor::Fuchsia(xiiColorGammaUB(0xFF, 0x00, 0xFF));
const xiiColor xiiColor::Gainsboro(xiiColorGammaUB(0xDC, 0xDC, 0xDC));
const xiiColor xiiColor::GhostWhite(xiiColorGammaUB(0xF8, 0xF8, 0xFF));
const xiiColor xiiColor::Gold(xiiColorGammaUB(0xFF, 0xD7, 0x00));
const xiiColor xiiColor::GoldenRod(xiiColorGammaUB(0xDA, 0xA5, 0x20));
const xiiColor xiiColor::Gray(xiiColorGammaUB(0x80, 0x80, 0x80));
const xiiColor xiiColor::Grey(xiiColorGammaUB(0x80, 0x80, 0x80));
const xiiColor xiiColor::Green(xiiColorGammaUB(0x00, 0x80, 0x00));
const xiiColor xiiColor::GreenYellow(xiiColorGammaUB(0xAD, 0xFF, 0x2F));
const xiiColor xiiColor::HoneyDew(xiiColorGammaUB(0xF0, 0xFF, 0xF0));
const xiiColor xiiColor::HotPink(xiiColorGammaUB(0xFF, 0x69, 0xB4));
const xiiColor xiiColor::IndianRed(xiiColorGammaUB(0xCD, 0x5C, 0x5C));
const xiiColor xiiColor::Indigo(xiiColorGammaUB(0x4B, 0x00, 0x82));
const xiiColor xiiColor::Ivory(xiiColorGammaUB(0xFF, 0xFF, 0xF0));
const xiiColor xiiColor::Khaki(xiiColorGammaUB(0xF0, 0xE6, 0x8C));
const xiiColor xiiColor::Lavender(xiiColorGammaUB(0xE6, 0xE6, 0xFA));
const xiiColor xiiColor::LavenderBlush(xiiColorGammaUB(0xFF, 0xF0, 0xF5));
const xiiColor xiiColor::LawnGreen(xiiColorGammaUB(0x7C, 0xFC, 0x00));
const xiiColor xiiColor::LemonChiffon(xiiColorGammaUB(0xFF, 0xFA, 0xCD));
const xiiColor xiiColor::LightBlue(xiiColorGammaUB(0xAD, 0xD8, 0xE6));
const xiiColor xiiColor::LightCoral(xiiColorGammaUB(0xF0, 0x80, 0x80));
const xiiColor xiiColor::LightCyan(xiiColorGammaUB(0xE0, 0xFF, 0xFF));
const xiiColor xiiColor::LightGoldenRodYellow(xiiColorGammaUB(0xFA, 0xFA, 0xD2));
const xiiColor xiiColor::LightGray(xiiColorGammaUB(0xD3, 0xD3, 0xD3));
const xiiColor xiiColor::LightGrey(xiiColorGammaUB(0xD3, 0xD3, 0xD3));
const xiiColor xiiColor::LightGreen(xiiColorGammaUB(0x90, 0xEE, 0x90));
const xiiColor xiiColor::LightPink(xiiColorGammaUB(0xFF, 0xB6, 0xC1));
const xiiColor xiiColor::LightSalmon(xiiColorGammaUB(0xFF, 0xA0, 0x7A));
const xiiColor xiiColor::LightSeaGreen(xiiColorGammaUB(0x20, 0xB2, 0xAA));
const xiiColor xiiColor::LightSkyBlue(xiiColorGammaUB(0x87, 0xCE, 0xFA));
const xiiColor xiiColor::LightSlateGray(xiiColorGammaUB(0x77, 0x88, 0x99));
const xiiColor xiiColor::LightSlateGrey(xiiColorGammaUB(0x77, 0x88, 0x99));
const xiiColor xiiColor::LightSteelBlue(xiiColorGammaUB(0xB0, 0xC4, 0xDE));
const xiiColor xiiColor::LightYellow(xiiColorGammaUB(0xFF, 0xFF, 0xE0));
const xiiColor xiiColor::Lime(xiiColorGammaUB(0x00, 0xFF, 0x00));
const xiiColor xiiColor::LimeGreen(xiiColorGammaUB(0x32, 0xCD, 0x32));
const xiiColor xiiColor::Linen(xiiColorGammaUB(0xFA, 0xF0, 0xE6));
const xiiColor xiiColor::Magenta(xiiColorGammaUB(0xFF, 0x00, 0xFF));
const xiiColor xiiColor::Maroon(xiiColorGammaUB(0x80, 0x00, 0x00));
const xiiColor xiiColor::MediumAquaMarine(xiiColorGammaUB(0x66, 0xCD, 0xAA));
const xiiColor xiiColor::MediumBlue(xiiColorGammaUB(0x00, 0x00, 0xCD));
const xiiColor xiiColor::MediumOrchid(xiiColorGammaUB(0xBA, 0x55, 0xD3));
const xiiColor xiiColor::MediumPurple(xiiColorGammaUB(0x93, 0x70, 0xDB));
const xiiColor xiiColor::MediumSeaGreen(xiiColorGammaUB(0x3C, 0xB3, 0x71));
const xiiColor xiiColor::MediumSlateBlue(xiiColorGammaUB(0x7B, 0x68, 0xEE));
const xiiColor xiiColor::MediumSpringGreen(xiiColorGammaUB(0x00, 0xFA, 0x9A));
const xiiColor xiiColor::MediumTurquoise(xiiColorGammaUB(0x48, 0xD1, 0xCC));
const xiiColor xiiColor::MediumVioletRed(xiiColorGammaUB(0xC7, 0x15, 0x85));
const xiiColor xiiColor::MidnightBlue(xiiColorGammaUB(0x19, 0x19, 0x70));
const xiiColor xiiColor::MintCream(xiiColorGammaUB(0xF5, 0xFF, 0xFA));
const xiiColor xiiColor::MistyRose(xiiColorGammaUB(0xFF, 0xE4, 0xE1));
const xiiColor xiiColor::Moccasin(xiiColorGammaUB(0xFF, 0xE4, 0xB5));
const xiiColor xiiColor::NavajoWhite(xiiColorGammaUB(0xFF, 0xDE, 0xAD));
const xiiColor xiiColor::Navy(xiiColorGammaUB(0x00, 0x00, 0x80));
const xiiColor xiiColor::OldLace(xiiColorGammaUB(0xFD, 0xF5, 0xE6));
const xiiColor xiiColor::Olive(xiiColorGammaUB(0x80, 0x80, 0x00));
const xiiColor xiiColor::OliveDrab(xiiColorGammaUB(0x6B, 0x8E, 0x23));
const xiiColor xiiColor::Orange(xiiColorGammaUB(0xFF, 0xA5, 0x00));
const xiiColor xiiColor::OrangeRed(xiiColorGammaUB(0xFF, 0x45, 0x00));
const xiiColor xiiColor::Orchid(xiiColorGammaUB(0xDA, 0x70, 0xD6));
const xiiColor xiiColor::PaleGoldenRod(xiiColorGammaUB(0xEE, 0xE8, 0xAA));
const xiiColor xiiColor::PaleGreen(xiiColorGammaUB(0x98, 0xFB, 0x98));
const xiiColor xiiColor::PaleTurquoise(xiiColorGammaUB(0xAF, 0xEE, 0xEE));
const xiiColor xiiColor::PaleVioletRed(xiiColorGammaUB(0xDB, 0x70, 0x93));
const xiiColor xiiColor::PapayaWhip(xiiColorGammaUB(0xFF, 0xEF, 0xD5));
const xiiColor xiiColor::PeachPuff(xiiColorGammaUB(0xFF, 0xDA, 0xB9));
const xiiColor xiiColor::Peru(xiiColorGammaUB(0xCD, 0x85, 0x3F));
const xiiColor xiiColor::Pink(xiiColorGammaUB(0xFF, 0xC0, 0xCB));
const xiiColor xiiColor::Plum(xiiColorGammaUB(0xDD, 0xA0, 0xDD));
const xiiColor xiiColor::PowderBlue(xiiColorGammaUB(0xB0, 0xE0, 0xE6));
const xiiColor xiiColor::Purple(xiiColorGammaUB(0x80, 0x00, 0x80));
const xiiColor xiiColor::RebeccaPurple(xiiColorGammaUB(0x66, 0x33, 0x99));
const xiiColor xiiColor::Red(xiiColorGammaUB(0xFF, 0x00, 0x00));
const xiiColor xiiColor::RosyBrown(xiiColorGammaUB(0xBC, 0x8F, 0x8F));
const xiiColor xiiColor::RoyalBlue(xiiColorGammaUB(0x41, 0x69, 0xE1));
const xiiColor xiiColor::SaddleBrown(xiiColorGammaUB(0x8B, 0x45, 0x13));
const xiiColor xiiColor::Salmon(xiiColorGammaUB(0xFA, 0x80, 0x72));
const xiiColor xiiColor::SandyBrown(xiiColorGammaUB(0xF4, 0xA4, 0x60));
const xiiColor xiiColor::SeaGreen(xiiColorGammaUB(0x2E, 0x8B, 0x57));
const xiiColor xiiColor::SeaShell(xiiColorGammaUB(0xFF, 0xF5, 0xEE));
const xiiColor xiiColor::Sienna(xiiColorGammaUB(0xA0, 0x52, 0x2D));
const xiiColor xiiColor::Silver(xiiColorGammaUB(0xC0, 0xC0, 0xC0));
const xiiColor xiiColor::SkyBlue(xiiColorGammaUB(0x87, 0xCE, 0xEB));
const xiiColor xiiColor::SlateBlue(xiiColorGammaUB(0x6A, 0x5A, 0xCD));
const xiiColor xiiColor::SlateGray(xiiColorGammaUB(0x70, 0x80, 0x90));
const xiiColor xiiColor::SlateGrey(xiiColorGammaUB(0x70, 0x80, 0x90));
const xiiColor xiiColor::Snow(xiiColorGammaUB(0xFF, 0xFA, 0xFA));
const xiiColor xiiColor::SpringGreen(xiiColorGammaUB(0x00, 0xFF, 0x7F));
const xiiColor xiiColor::SteelBlue(xiiColorGammaUB(0x46, 0x82, 0xB4));
const xiiColor xiiColor::Tan(xiiColorGammaUB(0xD2, 0xB4, 0x8C));
const xiiColor xiiColor::Teal(xiiColorGammaUB(0x00, 0x80, 0x80));
const xiiColor xiiColor::Thistle(xiiColorGammaUB(0xD8, 0xBF, 0xD8));
const xiiColor xiiColor::Tomato(xiiColorGammaUB(0xFF, 0x63, 0x47));
const xiiColor xiiColor::Turquoise(xiiColorGammaUB(0x40, 0xE0, 0xD0));
const xiiColor xiiColor::Violet(xiiColorGammaUB(0xEE, 0x82, 0xEE));
const xiiColor xiiColor::Wheat(xiiColorGammaUB(0xF5, 0xDE, 0xB3));
const xiiColor xiiColor::White(xiiColorGammaUB(0xFF, 0xFF, 0xFF));
const xiiColor xiiColor::WhiteSmoke(xiiColorGammaUB(0xF5, 0xF5, 0xF5));
const xiiColor xiiColor::Yellow(xiiColorGammaUB(0xFF, 0xFF, 0x00));
const xiiColor xiiColor::YellowGreen(xiiColorGammaUB(0x9A, 0xCD, 0x32));


xiiUInt32 xiiColor::ToRGBA8() const
{
  return xiiColorLinearUB(*this).ToRGBA8();
}

xiiUInt32 xiiColor::ToABGR8() const
{
  return xiiColorLinearUB(*this).ToABGR8();
}

XII_STATICLINK_FILE(Foundation, Foundation_Math_Implementation_Color);
