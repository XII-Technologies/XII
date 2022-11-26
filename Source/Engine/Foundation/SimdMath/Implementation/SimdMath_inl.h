#pragma once

///\todo optimize these methods if needed

// static
XII_FORCE_INLINE xiiSimdVec4f xiiSimdMath::Sin(const xiiSimdVec4f& f)
{
  return xiiSimdVec4f(xiiMath::Sin(xiiAngle::Radian(f.x())), xiiMath::Sin(xiiAngle::Radian(f.y())), xiiMath::Sin(xiiAngle::Radian(f.z())),
                      xiiMath::Sin(xiiAngle::Radian(f.w())));
}

// static
XII_FORCE_INLINE xiiSimdVec4f xiiSimdMath::Cos(const xiiSimdVec4f& f)
{
  return xiiSimdVec4f(xiiMath::Cos(xiiAngle::Radian(f.x())), xiiMath::Cos(xiiAngle::Radian(f.y())), xiiMath::Cos(xiiAngle::Radian(f.z())),
                      xiiMath::Cos(xiiAngle::Radian(f.w())));
}

// static
XII_FORCE_INLINE xiiSimdVec4f xiiSimdMath::Tan(const xiiSimdVec4f& f)
{
  return xiiSimdVec4f(xiiMath::Tan(xiiAngle::Radian(f.x())), xiiMath::Tan(xiiAngle::Radian(f.y())), xiiMath::Tan(xiiAngle::Radian(f.z())),
                      xiiMath::Tan(xiiAngle::Radian(f.w())));
}

// static
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdMath::ASin(const xiiSimdVec4f& f)
{
  return xiiSimdVec4f(xiiMath::Pi<float>() * 0.5f) - ACos(f);
}

// 4th order polynomial approximation
// 7 * 10^-5 radians precision
// Reference : Handbook of Mathematical Functions (chapter : Elementary Transcendental Functions), M. Abramowitz and I.A. Stegun, Ed.
// static
XII_FORCE_INLINE xiiSimdVec4f xiiSimdMath::ACos(const xiiSimdVec4f& f)
{
  xiiSimdVec4f x1 = f.Abs();
  xiiSimdVec4f x2 = x1.CompMul(x1);
  xiiSimdVec4f x3 = x2.CompMul(x1);

  xiiSimdVec4f s = x1 * -0.2121144f + xiiSimdVec4f(1.5707288f);
  s += x2 * 0.0742610f;
  s += x3 * -0.0187293f;
  s = s.CompMul((xiiSimdVec4f(1.0f) - x1).GetSqrt());

  return xiiSimdVec4f::Select(f >= xiiSimdVec4f::ZeroVector(), s, xiiSimdVec4f(xiiMath::Pi<float>()) - s);
}

// Reference: https://seblagarde.wordpress.com/2014/12/01/inverse-trigonometric-functions-gpu-optimization-for-amd-gcn-architecture/
// static
XII_FORCE_INLINE xiiSimdVec4f xiiSimdMath::ATan(const xiiSimdVec4f& f)
{
  xiiSimdVec4f x    = f.Abs();
  xiiSimdVec4f t0   = xiiSimdVec4f::Select(x < xiiSimdVec4f(1.0f), x, x.GetReciprocal());
  xiiSimdVec4f t1   = t0.CompMul(t0);
  xiiSimdVec4f poly = xiiSimdVec4f(0.0872929f);
  poly              = xiiSimdVec4f(-0.301895f) + poly.CompMul(t1);
  poly              = xiiSimdVec4f(1.0f) + poly.CompMul(t1);
  poly              = poly.CompMul(t0);
  t0                = xiiSimdVec4f::Select(x < xiiSimdVec4f(1.0f), poly, xiiSimdVec4f(xiiMath::Pi<float>() * 0.5f) - poly);

  return xiiSimdVec4f::Select(f < xiiSimdVec4f::ZeroVector(), -t0, t0);
}
