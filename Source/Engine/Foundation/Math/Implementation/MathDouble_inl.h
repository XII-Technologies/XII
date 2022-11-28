#pragma once

namespace xiiMath
{
  XII_ALWAYS_INLINE bool IsFinite(double value)
  {
    // Check the 11 exponent bits.
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    xiiInt64DoubleUnion i2f(value);
    return ((i2f.i & 0x7FF0000000000000ull) != 0x7FF0000000000000ull);
  }

  XII_ALWAYS_INLINE bool IsNaN(double value)
  {
    // Check the 11 exponent bits.
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    xiiInt64DoubleUnion i2f(value);
    return (((i2f.i & 0x7FF0000000000000ull) == 0x7FF0000000000000ull) && ((i2f.i & 0xFFFFFFFFFFFFFull) != 0));
  }

  XII_ALWAYS_INLINE double Floor(double f) { return floor(f); }

  XII_ALWAYS_INLINE double Ceil(double f) { return ceil(f); }

  XII_ALWAYS_INLINE double Round(double f) { return Floor(f + 0.5f); }

  inline double RoundDown(double f, double fMultiple)
  {
    double fDivides = f / fMultiple;
    double fFactor  = Floor(fDivides);
    return fFactor * fMultiple;
  }

  inline double RoundUp(double f, double fMultiple)
  {
    double fDivides = f / fMultiple;
    double fFactor  = Ceil(fDivides);
    return fFactor * fMultiple;
  }

  XII_ALWAYS_INLINE double Sin(double a) { return sin(a); }

  XII_ALWAYS_INLINE double Cos(double a) { return cos(a); }

  XII_ALWAYS_INLINE double Tan(double a) { return tan(a); }

  XII_ALWAYS_INLINE double ASin(double f) { return asin(f); }

  XII_ALWAYS_INLINE double ACos(double f) { return acos(f); }

  XII_ALWAYS_INLINE double ATan(double f) { return atan(f); }

  XII_ALWAYS_INLINE double ATan2(double y, double x) { return atan2(y, x); }

  XII_ALWAYS_INLINE double RoundToMultiple(double f, double multiple) { return Round(f / multiple) * multiple; }

  XII_ALWAYS_INLINE double Exp(double f) { return exp(f); }

  XII_ALWAYS_INLINE double Ln(double f) { return log(f); }

  XII_ALWAYS_INLINE double Log2(double f) { return log10(f) / log10(2.0); }

  XII_ALWAYS_INLINE double Log10(double f) { return log10(f); }

  XII_ALWAYS_INLINE double Log(double fBase, double f) { return log10(f) / log10(fBase); }

  XII_ALWAYS_INLINE double Pow2(double f) { return pow(2.0, f); }

  XII_ALWAYS_INLINE double Pow(double base, double exp) { return pow(base, exp); }

  XII_ALWAYS_INLINE double Root(double f, double NthRoot) { return pow(f, 1.0 / NthRoot); }

  XII_ALWAYS_INLINE double Sqrt(double f) { return sqrt(f); }

  XII_ALWAYS_INLINE double Mod(double f, double div) { return fmod(f, div); }

  XII_ALWAYS_INLINE double Hypot(double x, double y) { return sqrt(pow(x, 2.0) + pow(y, 2.0)); }

  XII_ALWAYS_INLINE double NormalizeToRange(double value, double min, double max) { return (value - min) / (max - min); }

} // namespace xiiMath
