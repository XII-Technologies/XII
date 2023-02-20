#pragma once

namespace xiiMath
{
  constexpr XII_ALWAYS_INLINE xiiInt32 RoundUp(xiiInt32 value, xiiUInt16 multiple)
  {
    //
    return (value >= 0) ? ((value + multiple - 1) / multiple) * multiple : (value / multiple) * multiple;
  }

  constexpr XII_ALWAYS_INLINE xiiInt32 RoundDown(xiiInt32 value, xiiUInt16 multiple)
  {
    //
    return (value <= 0) ? ((value - multiple + 1) / multiple) * multiple : (value / multiple) * multiple;
  }

  constexpr XII_ALWAYS_INLINE xiiUInt32 RoundUp(xiiUInt32 value, xiiUInt16 multiple)
  {
    //
    return ((value + multiple - 1) / multiple) * multiple;
  }

  constexpr XII_ALWAYS_INLINE xiiUInt32 RoundDown(xiiUInt32 value, xiiUInt16 multiple)
  {
    //
    return (value / multiple) * multiple;
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

  XII_ALWAYS_INLINE xiiUInt32 Log2i(xiiUInt32 val)
  {
    return (val != 0) ? FirstBitHigh(val) : -1;
  }

  constexpr XII_ALWAYS_INLINE xiiInt32 Pow2(xiiInt32 i)
  {
    //
    return (1 << i);
  }

  inline xiiInt32 Pow(xiiInt32 base, xiiInt32 exp)
  {
    xiiInt32 res = 1;
    while (exp > 0)
    {
      res *= base;
      --exp;
    }

    return res;
  }

} // namespace xiiMath
