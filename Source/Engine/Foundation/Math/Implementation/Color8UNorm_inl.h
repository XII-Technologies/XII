#pragma once

XII_ALWAYS_INLINE xiiColorBaseUB::xiiColorBaseUB(xiiUInt8 r, xiiUInt8 g, xiiUInt8 b, xiiUInt8 a /* = 255*/)
{
  this->r = r;
  this->g = g;
  this->b = b;
  this->a = a;
}

XII_ALWAYS_INLINE xiiColorLinearUB::xiiColorLinearUB(xiiUInt8 r, xiiUInt8 g, xiiUInt8 b, xiiUInt8 a /* = 255*/) :
  xiiColorBaseUB(r, g, b, a)
{
}

inline xiiColorLinearUB::xiiColorLinearUB(const xiiColor& color)
{
  *this = color;
}

inline void xiiColorLinearUB::operator=(const xiiColor& color)
{
  r = xiiMath::ColorFloatToByte(color.r);
  g = xiiMath::ColorFloatToByte(color.g);
  b = xiiMath::ColorFloatToByte(color.b);
  a = xiiMath::ColorFloatToByte(color.a);
}

inline xiiColor xiiColorLinearUB::ToLinearFloat() const
{
  return xiiColor(xiiMath::ColorByteToFloat(r), xiiMath::ColorByteToFloat(g), xiiMath::ColorByteToFloat(b), xiiMath::ColorByteToFloat(a));
}

// *****************

XII_ALWAYS_INLINE xiiColorGammaUB::xiiColorGammaUB(xiiUInt8 r, xiiUInt8 g, xiiUInt8 b, xiiUInt8 a) :
  xiiColorBaseUB(r, g, b, a)
{
}

inline xiiColorGammaUB::xiiColorGammaUB(const xiiColor& color)
{
  *this = color;
}

inline void xiiColorGammaUB::operator=(const xiiColor& color)
{
  const xiiVec3 gamma = xiiColor::LinearToGamma(xiiVec3(color.r, color.g, color.b));

  r = xiiMath::ColorFloatToByte(gamma.x);
  g = xiiMath::ColorFloatToByte(gamma.y);
  b = xiiMath::ColorFloatToByte(gamma.z);
  a = xiiMath::ColorFloatToByte(color.a);
}

inline xiiColor xiiColorGammaUB::ToLinearFloat() const
{
  xiiVec3 gamma;
  gamma.x = xiiMath::ColorByteToFloat(r);
  gamma.y = xiiMath::ColorByteToFloat(g);
  gamma.z = xiiMath::ColorByteToFloat(b);

  const xiiVec3 linear = xiiColor::GammaToLinear(gamma);

  return xiiColor(linear.x, linear.y, linear.z, xiiMath::ColorByteToFloat(a));
}
