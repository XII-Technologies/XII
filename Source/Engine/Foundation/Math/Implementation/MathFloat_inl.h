#pragma once

#include <algorithm>

namespace xiiMath
{
  template <>
  XII_ALWAYS_INLINE float Sin(xiiAngleTemplate<float> a) { return sinf(a.GetRadian()); }

  template <>
  XII_ALWAYS_INLINE float Cos(xiiAngleTemplate<float> a) { return cosf(a.GetRadian()); }

  template <>
  XII_ALWAYS_INLINE float Tan(xiiAngleTemplate<float> a) { return tanf(a.GetRadian()); }

  template <>
  XII_ALWAYS_INLINE xiiAngleTemplate<float> ASin(float f) { return xiiAngleTemplate<float>::Radian(asinf(f)); }

  template <>
  XII_ALWAYS_INLINE xiiAngleTemplate<float> ACos(float f) { return xiiAngleTemplate<float>::Radian(acosf(f)); }

  template <>
  XII_ALWAYS_INLINE xiiAngleTemplate<float> ATan(float f) { return xiiAngleTemplate<float>::Radian(atanf(f)); }

  template <>
  XII_ALWAYS_INLINE xiiAngleTemplate<float> ATan2(float y, float x) { return xiiAngleTemplate<float>::Radian(atan2f(y, x)); }

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

  XII_ALWAYS_INLINE float RoundToMultiple(float f, float fMultiple) { return Round(f / fMultiple) * fMultiple; }


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

  XII_ALWAYS_INLINE float Exp(float f) { return expf(f); }

  XII_ALWAYS_INLINE float Ln(float f) { return logf(f); }

  XII_ALWAYS_INLINE float Log2(float f) { return log2f(f); }

  XII_ALWAYS_INLINE float Log10(float f) { return log10f(f); }

  XII_ALWAYS_INLINE float Log(float fBase, float f) { return log10f(f) / log10f(fBase); }

  XII_ALWAYS_INLINE float Pow2(float f) { return exp2f(f); }

  XII_ALWAYS_INLINE float Pow(float fBase, float fExp) { return powf(fBase, fExp); }

  XII_ALWAYS_INLINE float Root(float f, float fNthRoot) { return powf(f, 1.0f / fNthRoot); }

  XII_ALWAYS_INLINE float Sqrt(float f) { return sqrtf(f); }

  XII_ALWAYS_INLINE float Mod(float f, float fDiv) { return fmodf(f, fDiv); }

  XII_ALWAYS_INLINE float Hypot(float x, float y) { return sqrtf(powf(x, 2.0f) + powf(y, 2.0f)); }

  XII_ALWAYS_INLINE float NormalizeToRange(float value, float fMin, float fMax) { return (value - fMin) / (fMax - fMin); }

} // namespace xiiMath
