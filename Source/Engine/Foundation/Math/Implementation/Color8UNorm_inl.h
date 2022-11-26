#pragma once

XII_ALWAYS_INLINE xiiColorBaseUB::xiiColorBaseUB(xiiUInt8 R, xiiUInt8 G, xiiUInt8 B, xiiUInt8 A /* = 255*/)
{
  r = R;
  g = G;
  b = B;
  a = A;
}

XII_ALWAYS_INLINE xiiColorLinearUB::xiiColorLinearUB(xiiUInt8 R, xiiUInt8 G, xiiUInt8 B, xiiUInt8 A /* = 255*/) :
  xiiColorBaseUB(R, G, B, A)
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

XII_ALWAYS_INLINE xiiColorGammaUB::xiiColorGammaUB(xiiUInt8 R, xiiUInt8 G, xiiUInt8 B, xiiUInt8 A) :
  xiiColorBaseUB(R, G, B, A)
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
