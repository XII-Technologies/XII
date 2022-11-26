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

  inline xiiUInt32 Log2i(xiiUInt32 val)
  {
    xiiInt32 ret = -1;
    while (val != 0)
    {
      val >>= 1;
      ret++;
    }

    return (xiiUInt32)ret;
  }

  constexpr XII_ALWAYS_INLINE int Pow2(int i)
  {
    //
    return (1 << i);
  }

  inline int Pow(int base, int exp)
  {
    int res = 1;
    while (exp > 0)
    {
      res *= base;
      --exp;
    }

    return res;
  }

} // namespace xiiMath
