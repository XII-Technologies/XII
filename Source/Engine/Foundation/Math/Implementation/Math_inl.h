#pragma once

#include <algorithm>

namespace xiiMath
{
  template <typename Type>
  XII_ALWAYS_INLINE Type Sin(xiiAngleTemplate<Type> a) { return (Type)sin(a.GetRadian()); }

  template <typename Type>
  XII_ALWAYS_INLINE Type Cos(xiiAngleTemplate<Type> a) { return (Type)cos(a.GetRadian()); }

  template <typename Type>
  XII_ALWAYS_INLINE Type Tan(xiiAngleTemplate<Type> a) { return (Type)tan(a.GetRadian()); }

  template <typename Type>
  XII_ALWAYS_INLINE xiiAngleTemplate<Type> ASin(Type f) { return xiiAngleTemplate<Type>::Radian((Type)asin(f)); }

  template <typename Type>
  XII_ALWAYS_INLINE xiiAngleTemplate<Type> ACos(Type f) { return xiiAngleTemplate<Type>::Radian((Type)acos(f)); }

  template <typename Type>
  XII_ALWAYS_INLINE xiiAngleTemplate<Type> ATan(Type f) { return xiiAngleTemplate<Type>::Radian((Type)atan(f)); }

  template <typename Type>
  XII_ALWAYS_INLINE xiiAngleTemplate<Type> ATan2(Type y, Type x) { return xiiAngleTemplate<Type>::Radian((Type)atan2(y, x)); }

  template <typename T>
  constexpr XII_ALWAYS_INLINE T Square(T f)
  {
    return (f * f);
  }

  template <typename T>
  constexpr XII_ALWAYS_INLINE T Sign(T f)
  {
    return (f < 0 ? T(-1) : f > 0 ? T(1) : 0);
  }

  template <typename T>
  constexpr XII_ALWAYS_INLINE T Abs(T f)
  {
    return (f < 0 ? -f : f);
  }

  template <typename T>
  constexpr XII_ALWAYS_INLINE T Min(T f1, T f2)
  {
    return (f2 < f1 ? f2 : f1);
  }

  template <typename T, typename... ARGS>
  constexpr XII_ALWAYS_INLINE T Min(T f1, T f2, ARGS... f)
  {
    return Min(Min(f1, f2), f...);
  }

  template <typename T>
  constexpr XII_ALWAYS_INLINE T Max(T f1, T f2)
  {
    return (f1 < f2 ? f2 : f1);
  }

  template <typename T, typename... ARGS>
  constexpr XII_ALWAYS_INLINE T Max(T f1, T f2, ARGS... f)
  {
    return Max(Max(f1, f2), f...);
  }

  template <typename T>
  constexpr XII_ALWAYS_INLINE T Clamp(T value, T min_val, T max_val)
  {
    return value < min_val ? min_val : (max_val < value ? max_val : value);
  }

  template <typename T>
  constexpr XII_ALWAYS_INLINE T Saturate(T value)
  {
    return Clamp(value, T(0), T(1));
  }

  template <typename Type>
  constexpr Type Invert(Type f)
  {
    return ((Type)1) / f;
  }

  XII_ALWAYS_INLINE xiiUInt32 FirstBitLow(xiiUInt32 value)
  {
    XII_ASSERT_DEBUG(value != 0, "FirstBitLow is undefined for 0");

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    unsigned long uiIndex = 0;
    _BitScanForward(&uiIndex, value);
    return uiIndex;
#elif XII_ENABLED(XII_COMPILER_GCC) || XII_ENABLED(XII_COMPILER_CLANG)
    return __builtin_ctz(value);
#else
    XII_ASSERT_NOT_IMPLEMENTED;
    return 0;
#endif
  }

  XII_ALWAYS_INLINE xiiUInt32 FirstBitLow(xiiUInt64 value)
  {
    XII_ASSERT_DEBUG(value != 0, "FirstBitLow is undefined for 0");

#if __castxml__
    return 0;
#elif XII_ENABLED(XII_PLATFORM_WINDOWS)
    unsigned long uiIndex = 0;
#  if XII_ENABLED(XII_PLATFORM_64BIT)

    _BitScanForward64(&uiIndex, value);
#  else
    uint32_t      lower      = static_cast<uint32_t>(value);
    unsigned char returnCode = _BitScanForward(&uiIndex, lower);
    if (returnCode == 0)
    {
      uint32_t upper = static_cast<uint32_t>(value >> 32);
      returnCode     = _BitScanForward(&uiIndex, upper);
      if (returnCode > 0) // Only can happen in Release build when XII_ASSERT_DEBUG(value != 0) would fail.
      {
        uiIndex += 32; // Add length of lower to index.
      }
    }
#  endif
    return uiIndex;
#elif XII_ENABLED(XII_COMPILER_GCC) || XII_ENABLED(XII_COMPILER_CLANG)
    return __builtin_ctzll(value);
#else
    XII_ASSERT_NOT_IMPLEMENTED;
    return 0;
#endif
  }

  XII_ALWAYS_INLINE xiiUInt32 FirstBitHigh(xiiUInt32 value)
  {
    XII_ASSERT_DEBUG(value != 0, "FirstBitHigh is undefined for 0");

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    unsigned long uiIndex = 0;
    _BitScanReverse(&uiIndex, value);
    return uiIndex;
#elif XII_ENABLED(XII_COMPILER_GCC) || XII_ENABLED(XII_COMPILER_CLANG)
    return 31 - __builtin_clz(value);
#else
    XII_ASSERT_NOT_IMPLEMENTED;
    return 0;
#endif
  }

  XII_ALWAYS_INLINE xiiUInt32 FirstBitHigh(xiiUInt64 value)
  {
    XII_ASSERT_DEBUG(value != 0, "FirstBitHigh is undefined for 0");

#if __castxml__
    return 0;
#elif XII_ENABLED(XII_PLATFORM_WINDOWS)
    unsigned long uiIndex = 0;
#  if XII_ENABLED(XII_PLATFORM_64BIT)
    _BitScanReverse64(&uiIndex, value);
#  else
    uint32_t      upper      = static_cast<uint32_t>(value >> 32);
    unsigned char returnCode = _BitScanReverse(&uiIndex, upper);
    if (returnCode == 0)
    {
      uint32_t lower = static_cast<uint32_t>(value);
      returnCode     = _BitScanReverse(&uiIndex, lower);
    }
    else
    {
      uiIndex += 32; // Add length of upper to index.
    }
#  endif
    return uiIndex;
#elif XII_ENABLED(XII_COMPILER_GCC) || XII_ENABLED(XII_COMPILER_CLANG)
    return 63 - __builtin_clzll(value);
#else
    XII_ASSERT_NOT_IMPLEMENTED;
    return 0;
#endif
  }

  XII_ALWAYS_INLINE xiiUInt32 CountTrailingZeros(xiiUInt32 uiBitmask) { return (uiBitmask == 0) ? 32 : FirstBitLow(uiBitmask); }

  XII_ALWAYS_INLINE xiiUInt32 CountTrailingZeros(xiiUInt64 uiBitmask)
  {
    const xiiUInt32 numLow  = CountTrailingZeros(static_cast<xiiUInt32>(uiBitmask & 0xFFFFFFFF));
    const xiiUInt32 numHigh = CountTrailingZeros(static_cast<xiiUInt32>((uiBitmask >> 32u) & 0xFFFFFFFF));

    return (numLow == 32) ? (32 + numHigh) : numLow;
  }

  XII_ALWAYS_INLINE xiiUInt32 CountLeadingZeros(xiiUInt32 uiBitmask) { return (uiBitmask == 0) ? 32 : (31u - FirstBitHigh(uiBitmask)); }


  XII_ALWAYS_INLINE xiiUInt32 CountBits(xiiUInt32 value)
  {
#if XII_ENABLED(XII_COMPILER_MSVC) && (XII_ENABLED(XII_PLATFORM_ARCH_X86) || (XII_ENABLED(XII_PLATFORM_ARCH_ARM) && XII_ENABLED(XII_PLATFORM_32BIT)))
#  if XII_ENABLED(XII_PLATFORM_ARCH_X86)
    return __popcnt(value);
#  else
    return _CountOneBits(value);
#  endif
#elif XII_ENABLED(XII_COMPILER_GCC) || XII_ENABLED(XII_COMPILER_CLANG)
    return __builtin_popcount(value);
#else
    value = value - ((value >> 1) & 0x55555555u);
    value = (value & 0x33333333u) + ((value >> 2) & 0x33333333u);
    return ((value + (value >> 4) & 0xF0F0F0Fu) * 0x1010101u) >> 24;
#endif
  }

  XII_ALWAYS_INLINE xiiUInt32 CountBits(xiiUInt64 value)
  {
    xiiUInt32 result = 0;
    result += CountBits(xiiUInt32(value));
    result += CountBits(xiiUInt32(value >> 32));
    return result;
  }

  template <typename T>
  XII_ALWAYS_INLINE void Swap(T& ref_f1, T& ref_f2)
  {
    std::swap(ref_f1, ref_f2);
  }

  template <typename T>
  XII_FORCE_INLINE T Lerp(T f1, T f2, float fFactor)
  {
    // value is not included in format string, to prevent requirement on FormatString.h, to break #include cycles
    XII_ASSERT_DEBUG((fFactor >= -0.00001f) && (fFactor <= 1.0f + 0.00001f), "lerp: factor is not in the range [0, 1]");

    return (T)(f1 + (fFactor * (f2 - f1)));
  }

  template <typename T>
  XII_FORCE_INLINE T Lerp(T f1, T f2, double fFactor)
  {
    // value is not included in format string, to prevent requirement on FormatString.h, to break #include cycles
    XII_ASSERT_DEBUG((fFactor >= -0.00001) && (fFactor <= 1.0 + 0.00001), "lerp: factor is not in the range [0, 1]");

    return (T)(f1 + (fFactor * (f2 - f1)));
  }

  ///  Returns 0, if value < edge, and 1, if value >= edge.
  template <typename T>
  constexpr XII_FORCE_INLINE T Step(T value, T edge)
  {
    return (value >= edge ? T(1) : T(0));
  }

  constexpr XII_FORCE_INLINE bool IsPowerOf2(xiiInt32 value) { return (value < 1) ? false : ((value & (value - 1)) == 0); }

  constexpr XII_FORCE_INLINE bool IsPowerOf2(xiiUInt32 value) { return (value < 1) ? false : ((value & (value - 1)) == 0); }

  template <typename Type>
  constexpr bool IsEqual(Type lhs, Type rhs, Type fEpsilon)
  {
    return ((rhs >= lhs - fEpsilon) && (rhs <= lhs + fEpsilon));
  }

  template <typename T>
  constexpr inline bool IsInRange(T value, T minVal, T maxVal)
  {
    return minVal < maxVal ? (value >= minVal) && (value <= maxVal) : (value <= minVal) && (value >= maxVal);
  }

  template <typename Type>
  bool IsZero(Type f, Type fEpsilon)
  {
    XII_ASSERT_DEBUG(fEpsilon >= 0, "Epsilon may not be negative.");

    return ((f >= -fEpsilon) && (f <= fEpsilon));
  }

  template <typename Type>
  XII_ALWAYS_INLINE Type Trunc(Type f)
  {
    if (f > 0)
      return Floor(f);

    return Ceil(f);
  }

  template <typename Type>
  XII_ALWAYS_INLINE Type Fraction(Type f)
  {
    return (f - Trunc(f));
  }

  template <typename Type>
  inline Type SmoothStep(Type x, Type edge1, Type edge2)
  {
    const Type divider = edge2 - edge1;

    if (divider == (Type)0)
    {
      if (x >= edge2)
        return (Type)1;
      return (Type)0;
    }

    x = (x - edge1) / divider;

    if (x <= (Type)0)
      return (Type)0;
    if (x >= (Type)1)
      return (Type)1;

    return (x * x * ((Type)3 - ((Type)2 * x)));
  }

  inline xiiUInt8 ColorFloatToByte(float value)
  {
    // Implemented according to
    // https://docs.microsoft.com/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    if (IsNaN(value))
    {
      return 0;
    }
    else
    {
      return static_cast<xiiUInt8>(Saturate(value) * 255.0f + 0.5f);
    }
  }

  inline xiiUInt16 ColorFloatToShort(float value)
  {
    // Implemented according to
    // https://docs.microsoft.com/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    if (IsNaN(value))
    {
      return 0;
    }
    else
    {
      return static_cast<xiiUInt16>(Saturate(value) * 65535.0f + 0.5f);
    }
  }

  inline xiiInt8 ColorFloatToSignedByte(float value)
  {
    // Implemented according to
    // https://docs.microsoft.com/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    if (IsNaN(value))
    {
      return 0;
    }
    else
    {
      value = Clamp(value, -1.0f, 1.0f) * 127.0f;
      if (value >= 0.0f)
      {
        value += 0.5f;
      }
      else
      {
        value -= 0.5f;
      }
      return static_cast<xiiInt8>(value);
    }
  }

  inline xiiInt16 ColorFloatToSignedShort(float value)
  {
    // Implemented according to
    // https://docs.microsoft.com/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    if (IsNaN(value))
    {
      return 0;
    }
    else
    {
      value = Clamp(value, -1.0f, 1.0f) * 32767.0f;
      if (value >= 0.0f)
      {
        value += 0.5f;
      }
      else
      {
        value -= 0.5f;
      }
      return static_cast<xiiInt16>(value);
    }
  }

  constexpr inline float ColorByteToFloat(xiiUInt8 value)
  {
    // Implemented according to
    // https://docs.microsoft.com/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    return value * (1.0f / 255.0f);
  }

  constexpr inline float ColorShortToFloat(xiiUInt16 value)
  {
    // Implemented according to
    // https://docs.microsoft.com/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    return value * (1.0f / 65535.0f);
  }

  constexpr inline float ColorSignedByteToFloat(xiiInt8 value)
  {
    // Implemented according to
    // https://docs.microsoft.com/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    return (value == -128) ? -1.0f : value * (1.0f / 127.0f);
  }

  constexpr inline float ColorSignedShortToFloat(xiiInt16 value)
  {
    // Implemented according to
    // https://docs.microsoft.com/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    return (value == -32768) ? -1.0f : value * (1.0f / 32767.0f);
  }

  template <typename T, typename T2>
  T EvaluateBezierCurve(T2 t, const T& startPoint, const T& controlPoint1, const T& controlPoint2, const T& endPoint)
  {
    const T2 mt = 1 - t;

    const T2 f1 = mt * mt * mt;
    const T2 f2 = 3 * mt * mt * t;
    const T2 f3 = 3 * mt * t * t;
    const T2 f4 = t * t * t;

    return f1 * startPoint + f2 * controlPoint1 + f3 * controlPoint2 + f4 * endPoint;
  }
} // namespace xiiMath

template <typename Type>
constexpr XII_FORCE_INLINE xiiAngleTemplate<Type> xiiAngleTemplate<Type>::AngleBetween(xiiAngleTemplate a, xiiAngleTemplate b)
{
  // Derived from http://gamedev.stackexchange.com/questions/4467/comparing-angles-and-working-out-the-difference
  return xiiAngleTemplate<Type>(xiiAngleTemplate<Type>::Pi() - xiiMath::Abs(xiiMath::Abs(a.GetRadian() - b.GetRadian()) - xiiAngleTemplate<Type>::Pi()));
}

constexpr XII_FORCE_INLINE xiiInt32 xiiMath::FloatToInt(float value)
{
  return static_cast<xiiInt32>(value);
}

#if XII_DISABLED(XII_PLATFORM_ARCH_X86) || (_MSC_VER <= 1916)
constexpr XII_FORCE_INLINE xiiInt64 xiiMath::FloatToInt(double value)
{
  return static_cast<xiiInt64>(value);
}
#endif

XII_ALWAYS_INLINE xiiResult xiiMath::TryConvertToSizeT(size_t& out_uiResult, xiiUInt64 uiValue)
{
#if XII_ENABLED(XII_PLATFORM_32BIT)
  if (uiValue <= MaxValue<size_t>())
  {
    out_Result = static_cast<size_t>(uiValue);
    return XII_SUCCESS;
  }

  return XII_FAILURE;
#else
  out_uiResult = static_cast<size_t>(uiValue);
  return XII_SUCCESS;
#endif
}

#if XII_ENABLED(XII_PLATFORM_64BIT)
XII_ALWAYS_INLINE size_t xiiMath::SafeConvertToSizeT(xiiUInt64 uiValue)
{
  return uiValue;
}
#endif
