#pragma once

#include <algorithm>

namespace xiiMath
{
  XII_ALWAYS_INLINE bool IsFinite(float value)
  {
    // Check the 8 exponent bits.
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    xiiIntFloatUnion i2f(value);
    return ((i2f.i & 0x7f800000u) != 0x7f800000u);
  }

  XII_ALWAYS_INLINE bool IsNaN(float value)
  {
    // Check the 8 exponent bits.
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    xiiIntFloatUnion i2f(value);
    return (((i2f.i & 0x7f800000u) == 0x7f800000u) && ((i2f.i & 0x7FFFFFu) != 0));
  }

  XII_ALWAYS_INLINE float Floor(float f) { return floorf(f); }

  XII_ALWAYS_INLINE float Ceil(float f) { return ceilf(f); }

  XII_ALWAYS_INLINE float Round(float f) { return Floor(f + 0.5f); }

  XII_ALWAYS_INLINE float RoundToMultiple(float f, float multiple) { return Round(f / multiple) * multiple; }


  inline float RoundDown(float f, float fMultiple)
  {
    float fDivides = f / fMultiple;
    float fFactor  = Floor(fDivides);
    return fFactor * fMultiple;
  }

  inline float RoundUp(float f, float fMultiple)
  {
    float fDivides = f / fMultiple;
    float fFactor  = Ceil(fDivides);
    return fFactor * fMultiple;
  }

  XII_ALWAYS_INLINE float Sin(xiiAngle a) { return sinf(a.GetRadian()); }

  XII_ALWAYS_INLINE float Cos(xiiAngle a) { return cosf(a.GetRadian()); }

  XII_ALWAYS_INLINE float Tan(xiiAngle a) { return tanf(a.GetRadian()); }

  XII_ALWAYS_INLINE xiiAngle ASin(float f) { return xiiAngle::Radian(asinf(f)); }

  XII_ALWAYS_INLINE xiiAngle ACos(float f) { return xiiAngle::Radian(acosf(f)); }

  XII_ALWAYS_INLINE xiiAngle ATan(float f) { return xiiAngle::Radian(atanf(f)); }

  XII_ALWAYS_INLINE xiiAngle ATan2(float y, float x) { return xiiAngle::Radian(atan2f(y, x)); }

  XII_ALWAYS_INLINE float Exp(float f) { return expf(f); }

  XII_ALWAYS_INLINE float Ln(float f) { return logf(f); }

  XII_ALWAYS_INLINE float Log2(float f) { return log2f(f); }

  XII_ALWAYS_INLINE float Log10(float f) { return log10f(f); }

  XII_ALWAYS_INLINE float Log(float fBase, float f) { return log10f(f) / log10f(fBase); }

  XII_ALWAYS_INLINE float Pow2(float f) { return exp2f(f); }

  XII_ALWAYS_INLINE float Pow(float base, float exp) { return powf(base, exp); }

  XII_ALWAYS_INLINE float Root(float f, float NthRoot) { return powf(f, 1.0f / NthRoot); }

  XII_ALWAYS_INLINE float Sqrt(float f) { return sqrtf(f); }

  XII_ALWAYS_INLINE float Mod(float f, float div) { return fmodf(f, div); }

  XII_ALWAYS_INLINE float Hypot(float x, float y) { return sqrtf(powf(x, 2.0f) + powf(y, 2.0f)); }

  XII_ALWAYS_INLINE float NormalizeToRange(float value, float min, float max) { return (value - min) / (max - min); }

} // namespace xiiMath
