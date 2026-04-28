/// Copyright (c) Theophilus Eriata. All Rights Reserved.

namespace xiiMath
{
  constexpr XII_ALWAYS_INLINE xiiInt32 RoundUp(xiiInt32 value, xiiUInt16 uiMultiple)
  {
    //
    return (value >= 0) ? ((value + uiMultiple - 1) / uiMultiple) * uiMultiple : (value / uiMultiple) * uiMultiple;
  }

  constexpr XII_ALWAYS_INLINE xiiInt32 RoundDown(xiiInt32 value, xiiUInt16 uiMultiple)
  {
    //
    return (value <= 0) ? ((value - uiMultiple + 1) / uiMultiple) * uiMultiple : (value / uiMultiple) * uiMultiple;
  }

  constexpr XII_ALWAYS_INLINE xiiUInt32 RoundUp(xiiUInt32 value, xiiUInt16 uiMultiple)
  {
    //
    return ((value + uiMultiple - 1) / uiMultiple) * uiMultiple;
  }

  constexpr XII_ALWAYS_INLINE xiiUInt32 RoundDown(xiiUInt32 value, xiiUInt16 uiMultiple)
  {
    //
    return (value / uiMultiple) * uiMultiple;
  }

  constexpr XII_ALWAYS_INLINE bool IsOdd(xiiInt32 i)
  {
    //
    return ((i & 1) != 0);
  }

  constexpr XII_ALWAYS_INLINE bool IsEven(xiiInt32 i)
  {
    //
    return ((i & 1) == 0);
  }

  XII_ALWAYS_INLINE xiiUInt32 Log2i(xiiUInt32 uiVal)
  {
    return (uiVal != 0) ? FirstBitHigh(uiVal) : -1;
  }

  constexpr XII_ALWAYS_INLINE xiiInt32 Pow2(xiiInt32 i)
  {
    //
    return (1 << i);
  }

  inline xiiInt32 Pow(xiiInt32 iBase, xiiInt32 iExp)
  {
    xiiInt32 res = 1;
    while (iExp > 0)
    {
      res *= iBase;
      --iExp;
    }

    return res;
  }

} // namespace xiiMath
