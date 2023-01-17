#pragma once

#include <Foundation/Math/FixedPoint.h>

#if 0

namespace xiiMath
{
#  define FIXEDPOINT_OVERLOADS(Bits)                                                       \
    template <>                                                                            \
    XII_ALWAYS_INLINE xiiFixedPoint<Bits> BasicType<xiiFixedPoint<Bits>>::MaxValue()       \
    {                                                                                      \
      return (xiiFixedPoint<Bits>)((1 << (31 - Bits)) - 1);                                \
    }                                                                                      \
    template <>                                                                            \
    XII_ALWAYS_INLINE xiiFixedPoint<Bits> BasicType<xiiFixedPoint<Bits>>::SmallEpsilon()   \
    {                                                                                      \
      return (xiiFixedPoint<Bits>)0.0001;                                                  \
    }                                                                                      \
    template <>                                                                            \
    XII_ALWAYS_INLINE xiiFixedPoint<Bits> BasicType<xiiFixedPoint<Bits>>::DefaultEpsilon() \
    {                                                                                      \
      return (xiiFixedPoint<Bits>)0.001;                                                   \
    }                                                                                      \
    template <>                                                                            \
    XII_ALWAYS_INLINE xiiFixedPoint<Bits> BasicType<xiiFixedPoint<Bits>>::LargeEpsilon()   \
    {                                                                                      \
      return (xiiFixedPoint<Bits>)0.01;                                                    \
    }                                                                                      \
    template <>                                                                            \
    XII_ALWAYS_INLINE xiiFixedPoint<Bits> BasicType<xiiFixedPoint<Bits>>::HugeEpsilon()    \
    {                                                                                      \
      return (xiiFixedPoint<Bits>)0.1;                                                     \
    }

  FIXEDPOINT_OVERLOADS(1);
  FIXEDPOINT_OVERLOADS(2);
  FIXEDPOINT_OVERLOADS(3);
  FIXEDPOINT_OVERLOADS(4);
  FIXEDPOINT_OVERLOADS(5);
  FIXEDPOINT_OVERLOADS(6);
  FIXEDPOINT_OVERLOADS(7);
  FIXEDPOINT_OVERLOADS(8);
  FIXEDPOINT_OVERLOADS(9);
  FIXEDPOINT_OVERLOADS(10);
  FIXEDPOINT_OVERLOADS(11);
  FIXEDPOINT_OVERLOADS(12);
  FIXEDPOINT_OVERLOADS(13);
  FIXEDPOINT_OVERLOADS(14);
  FIXEDPOINT_OVERLOADS(15);
  FIXEDPOINT_OVERLOADS(16);
  FIXEDPOINT_OVERLOADS(17);
  FIXEDPOINT_OVERLOADS(18);
  FIXEDPOINT_OVERLOADS(19);
  FIXEDPOINT_OVERLOADS(20);
  FIXEDPOINT_OVERLOADS(21);
  FIXEDPOINT_OVERLOADS(22);
  FIXEDPOINT_OVERLOADS(23);
  FIXEDPOINT_OVERLOADS(24);
  FIXEDPOINT_OVERLOADS(25);
  FIXEDPOINT_OVERLOADS(26);
  FIXEDPOINT_OVERLOADS(27);
  FIXEDPOINT_OVERLOADS(28);
  FIXEDPOINT_OVERLOADS(29);
  FIXEDPOINT_OVERLOADS(30);
  //FIXEDPOINT_OVERLOADS(31);

  template <xiiUInt8 DecimalBits>
  XII_FORCE_INLINE xiiFixedPoint<DecimalBits> Floor(xiiFixedPoint<DecimalBits> f)
  {
    XII_REPORT_FAILURE("This function is not really implemented yet.");

    return (xiiFixedPoint<DecimalBits>)floor(f.ToDouble());
  }

  template <xiiUInt8 DecimalBits>
  XII_FORCE_INLINE xiiFixedPoint<DecimalBits> Ceil(xiiFixedPoint<DecimalBits> f)
  {
    XII_REPORT_FAILURE("This function is not really implemented yet.");

    return (xiiFixedPoint<DecimalBits>)ceil(f.ToDouble());
  }

  template <xiiUInt8 DecimalBits>
  inline xiiFixedPoint<DecimalBits> Floor(xiiFixedPoint<DecimalBits> f, xiiFixedPoint<DecimalBits> fMultiple)
  {
    XII_REPORT_FAILURE("This function is not really implemented yet.");

    xiiFixedPoint<DecimalBits> fDivides = f / fMultiple;
    xiiFixedPoint<DecimalBits> fFactor = Floor(fDivides);
    return fFactor * fMultiple;
  }

  template <xiiUInt8 DecimalBits>
  inline xiiFixedPoint<DecimalBits> Ceil(xiiFixedPoint<DecimalBits> f, xiiFixedPoint<DecimalBits> fMultiple)
  {
    XII_REPORT_FAILURE("This function is not really implemented yet.");

    xiiFixedPoint<DecimalBits> fDivides = f / fMultiple;
    xiiFixedPoint<DecimalBits> fFactor = Ceil(fDivides);
    return fFactor * fMultiple;
  }

  template <xiiUInt8 DecimalBits>
  XII_FORCE_INLINE xiiFixedPoint<DecimalBits> Exp(xiiFixedPoint<DecimalBits> f)
  {
    XII_REPORT_FAILURE("This function is not really implemented yet.");

    return (xiiFixedPoint<DecimalBits>)exp(f.ToDouble());
  }

  template <xiiUInt8 DecimalBits>
  XII_FORCE_INLINE xiiFixedPoint<DecimalBits> Ln(xiiFixedPoint<DecimalBits> f)
  {
    XII_REPORT_FAILURE("This function is not really implemented yet.");

    return (xiiFixedPoint<DecimalBits>)log(f.ToDouble());
  }

  template <xiiUInt8 DecimalBits>
  XII_FORCE_INLINE xiiFixedPoint<DecimalBits> Log2(xiiFixedPoint<DecimalBits> f)
  {
    XII_REPORT_FAILURE("This function is not really implemented yet.");

    return (xiiFixedPoint<DecimalBits>)(log10(f.ToDouble()) / log10(2.0));
  }

  template <xiiUInt8 DecimalBits>
  XII_FORCE_INLINE xiiFixedPoint<DecimalBits> Log10(xiiFixedPoint<DecimalBits> f)
  {
    XII_REPORT_FAILURE("This function is not really implemented yet.");

    return (xiiFixedPoint<DecimalBits>)log10(f.ToDouble());
  }

  template <xiiUInt8 DecimalBits>
  XII_FORCE_INLINE xiiFixedPoint<DecimalBits> Log(xiiFixedPoint<DecimalBits> fBase, xiiFixedPoint<DecimalBits> f)
  {
    XII_REPORT_FAILURE("This function is not really implemented yet.");

    return (xiiFixedPoint<DecimalBits>)(log10(f.ToDouble()) / log10(fBase.ToDouble()));
  }

  template <xiiUInt8 DecimalBits>
  XII_FORCE_INLINE xiiFixedPoint<DecimalBits> Pow2(xiiFixedPoint<DecimalBits> f)
  {
    XII_REPORT_FAILURE("This function is not really implemented yet.");

    return (xiiFixedPoint<DecimalBits>)pow(2.0, f.ToDouble());
  }

  template <xiiUInt8 DecimalBits>
  XII_FORCE_INLINE xiiFixedPoint<DecimalBits> Pow(xiiFixedPoint<DecimalBits> base, xiiFixedPoint<DecimalBits> exp)
  {
    XII_REPORT_FAILURE("This function is not really implemented yet.");

    return (xiiFixedPoint<DecimalBits>)pow(base.ToDouble(), exp.ToDouble());
  }

  template <xiiUInt8 DecimalBits>
  XII_FORCE_INLINE xiiFixedPoint<DecimalBits> Root(xiiFixedPoint<DecimalBits> f, xiiFixedPoint<DecimalBits> NthRoot)
  {
    XII_REPORT_FAILURE("This function is not really implemented yet.");

    return (xiiFixedPoint<DecimalBits>)pow(f.ToDouble(), 1.0 / NthRoot.ToDouble());
  }

  template <xiiUInt8 DecimalBits>
  xiiFixedPoint<DecimalBits> Sqrt(xiiFixedPoint<DecimalBits> a)
  {
    return (xiiFixedPoint<DecimalBits>)sqrt(a.ToDouble());

#  if 0
    if (a <= xiiFixedPoint<DecimalBits>(0))
      return xiiFixedPoint<DecimalBits>(0);

    xiiFixedPoint<DecimalBits> x = a / 2;

    for (xiiUInt32 i = 0; i < 8; ++i)
    {
      xiiFixedPoint<DecimalBits> ax = a / x;
      xiiFixedPoint<DecimalBits> xpax = x + ax;
      x = xpax / 2;
    }

    return x;
#  endif
  }

  template <xiiUInt8 DecimalBits>
  XII_FORCE_INLINE xiiFixedPoint<DecimalBits> Mod(xiiFixedPoint<DecimalBits> f, xiiFixedPoint<DecimalBits> div)
  {
    XII_REPORT_FAILURE("This function is not really implemented yet.");

    return (xiiFixedPoint<DecimalBits>)fmod(f.ToDouble(), div);
  }
}

#endif
