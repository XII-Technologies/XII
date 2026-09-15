/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

inline xiiColor::xiiColor()
{
#if XII_ENABLED(XII_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const float TypeNaN = xiiMath::NaN<float>();
  r                   = TypeNaN;
  g                   = TypeNaN;
  b                   = TypeNaN;
  a                   = TypeNaN;
#endif
}

XII_FORCE_INLINE constexpr xiiColor::xiiColor(float fLinearRed, float fLinearGreen, float fLinearBlue, float fLinearAlpha /* = 1.0f */) :
  r(fLinearRed), g(fLinearGreen), b(fLinearBlue), a(fLinearAlpha)
{
}

inline xiiColor::xiiColor(const xiiColorLinearUB& cc)
{
  *this = cc;
}

inline xiiColor::xiiColor(const xiiColorGammaUB& cc)
{
  *this = cc;
}

inline void xiiColor::SetRGB(float fLinearRed, float fLinearGreen, float fLinearBlue)
{
  r = fLinearRed;
  g = fLinearGreen;
  b = fLinearBlue;
}

inline void xiiColor::SetRGBA(float fLinearRed, float fLinearGreen, float fLinearBlue, float fLinearAlpha /* = 1.0f */)
{
  r = fLinearRed;
  g = fLinearGreen;
  b = fLinearBlue;
  a = fLinearAlpha;
}

inline xiiColor xiiColor::MakeFromKelvin(xiiUInt32 uiKelvin)
{
  xiiColor finalColor;
  float    fKelvin        = xiiMath::Clamp(uiKelvin, 1000u, 40000u) / 1000.0f;
  float    fKelvinSquared = fKelvin * fKelvin;

  finalColor.r = fKelvin < 6.570f ? 1.0f : xiiMath::Saturate((1.35651f + 0.216422f * fKelvin + 0.000633715f * fKelvinSquared) / (-3.24223f + 0.918711f * fKelvin));
  finalColor.g = fKelvin < 6.570f ? xiiMath::Saturate((-399.809f + 414.271f * fKelvin + 111.543f * fKelvinSquared) / (2779.24f + 164.143f * fKelvin + 84.7356f * fKelvinSquared)) : xiiMath::Saturate((1370.38f + 734.616f * fKelvin + 0.689955f * fKelvinSquared) / (-4625.69f + 1699.87f * fKelvin));
  finalColor.b = fKelvin > 6.570f ? 1.0f : xiiMath::Saturate((348.963f - 523.53f * fKelvin + 183.62f * fKelvinSquared) / (2848.82f - 214.52f * fKelvin + 78.8614f * fKelvinSquared));

  return finalColor;
}

// http://en.wikipedia.org/wiki/Luminance_%28relative%29
XII_FORCE_INLINE float xiiColor::GetLuminance() const
{
  return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

inline xiiColor xiiColor::GetInvertedColor() const
{
  XII_NAN_ASSERT(this);
  XII_ASSERT_DEBUG(IsNormalized(), "Cannot invert a color that has values outside the [0; 1] range");

  return xiiColor(1.0f - r, 1.0f - g, 1.0f - b, 1.0f - a);
}

inline bool xiiColor::IsNaN() const
{
  if (xiiMath::IsNaN(r))
    return true;
  if (xiiMath::IsNaN(g))
    return true;
  if (xiiMath::IsNaN(b))
    return true;
  if (xiiMath::IsNaN(a))
    return true;

  return false;
}

inline void xiiColor::operator+=(const xiiColor& rhs)
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  r += rhs.r;
  g += rhs.g;
  b += rhs.b;
  a += rhs.a;
}

inline void xiiColor::operator-=(const xiiColor& rhs)
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  r -= rhs.r;
  g -= rhs.g;
  b -= rhs.b;
  a -= rhs.a;
}

inline void xiiColor::operator*=(const xiiColor& rhs)
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  r *= rhs.r;
  g *= rhs.g;
  b *= rhs.b;
  a *= rhs.a;
}
inline void xiiColor::operator*=(float f)
{
  r *= f;
  g *= f;
  b *= f;
  a *= f;

  XII_NAN_ASSERT(this);
}

inline bool xiiColor::IsIdenticalRGB(const xiiColor& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return r == rhs.r && g == rhs.g && b == rhs.b;
}

inline bool xiiColor::IsIdenticalRGBA(const xiiColor& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
}

inline xiiColor xiiColor::WithAlpha(float fAlpha) const
{
  return xiiColor(r, g, b, fAlpha);
}

inline const xiiColor operator+(const xiiColor& c1, const xiiColor& c2)
{
  XII_NAN_ASSERT(&c1);
  XII_NAN_ASSERT(&c2);

  return xiiColor(c1.r + c2.r, c1.g + c2.g, c1.b + c2.b, c1.a + c2.a);
}

inline const xiiColor operator-(const xiiColor& c1, const xiiColor& c2)
{
  XII_NAN_ASSERT(&c1);
  XII_NAN_ASSERT(&c2);

  return xiiColor(c1.r - c2.r, c1.g - c2.g, c1.b - c2.b, c1.a - c2.a);
}

inline const xiiColor operator*(const xiiColor& c1, const xiiColor& c2)
{
  XII_NAN_ASSERT(&c1);
  XII_NAN_ASSERT(&c2);

  return xiiColor(c1.r * c2.r, c1.g * c2.g, c1.b * c2.b, c1.a * c2.a);
}

inline const xiiColor operator*(float f, const xiiColor& c)
{
  XII_NAN_ASSERT(&c);

  return xiiColor(c.r * f, c.g * f, c.b * f, c.a * f);
}

inline const xiiColor operator*(const xiiColor& c, float f)
{
  XII_NAN_ASSERT(&c);

  return xiiColor(c.r * f, c.g * f, c.b * f, c.a * f);
}

inline const xiiColor operator*(const xiiMat4& lhs, const xiiColor& rhs)
{
  xiiColor r = rhs;
  r *= lhs;
  return r;
}

inline const xiiColor operator/(const xiiColor& c, float f)
{
  XII_NAN_ASSERT(&c);

  float f_inv = 1.0f / f;
  return xiiColor(c.r * f_inv, c.g * f_inv, c.b * f_inv, c.a * f_inv);
}

XII_ALWAYS_INLINE bool operator==(const xiiColor& c1, const xiiColor& c2)
{
  return c1.IsIdenticalRGBA(c2);
}

XII_FORCE_INLINE bool operator<(const xiiColor& c1, const xiiColor& c2)
{
  if (c1.r < c2.r)
    return true;
  if (c1.r > c2.r)
    return false;
  if (c1.g < c2.g)
    return true;
  if (c1.g > c2.g)
    return false;
  if (c1.b < c2.b)
    return true;
  if (c1.b > c2.b)
    return false;

  return (c1.a < c2.a);
}
