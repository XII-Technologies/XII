/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <float.h>

namespace xiiMath
{
  //////////////////////////////////////////////////////////////////////////

  template <>
  constexpr float Pi()
  {
    return static_cast<float>(3.1415926535897932384626433832795f);
  }

  template <>
  constexpr double Pi()
  {
    return static_cast<double>(3.1415926535897932384626433832795);
  }

  //////////////////////////////////////////////////////////////////////////

  template <>
  constexpr float Phi()
  {
    return static_cast<float>(1.6180339887498948482045868343656f);
  }

  template <>
  constexpr double Phi()
  {
    return static_cast<double>(1.6180339887498948482045868343656);
  }

  //////////////////////////////////////////////////////////////////////////

  template <>
  constexpr float e()
  {
    return static_cast<float>(2.71828182845904);
  }

  template <>
  constexpr double e()
  {
    return static_cast<double>(2.71828182845904);
  }

  //////////////////////////////////////////////////////////////////////////

  template <typename TYPE>
  constexpr bool SupportsNaN()
  {
    return false;
  }

  template <>
  constexpr bool SupportsNaN<float>()
  {
    return true;
  }

  template <>
  constexpr bool SupportsNaN<double>()
  {
    return true;
  }

  //////////////////////////////////////////////////////////////////////////

  template <typename TYPE>
  constexpr TYPE NaN()
  {
    return static_cast<TYPE>(0);
  }

  template <>
  constexpr float NaN()
  {
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    // NaN = 0111 1111 1000 0000 0000 0000 0000 0001

    xiiIntFloatUnion i2f(0x7f800042u);
    return i2f.f;
  }

  template <>
  constexpr double NaN()
  {
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    // NaN = 0111 1111 1111 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001

    xiiInt64DoubleUnion i2f(0x7FF0000000000042ull);
    return i2f.f;
  }

  //////////////////////////////////////////////////////////////////////////

  template <typename TYPE>
  constexpr bool SupportsInfinity()
  {
    return false;
  }

  template <>
  constexpr bool SupportsInfinity<float>()
  {
    return true;
  }

  template <>
  constexpr bool SupportsInfinity<double>()
  {
    return true;
  }

  //////////////////////////////////////////////////////////////////////////

  template <typename TYPE>
  constexpr TYPE Infinity()
  {
    return static_cast<TYPE>(0);
  }

  template <>
  constexpr float Infinity()
  {
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    // INF = 0111 1111 1000 0000 0000 0000 0000 0000

    // bitwise representation of float infinity (positive)
    xiiIntFloatUnion i2f(0x7f800000u);
    return i2f.f;
  }

  template <>
  constexpr double Infinity()
  {
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    // INF = 0111 1111 1111 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000

    // bitwise representation of double infinity (positive)
    xiiInt64DoubleUnion i2f(0x7FF0000000000000ull);

    return i2f.f;
  }

  //////////////////////////////////////////////////////////////////////////

  template <>
  constexpr xiiUInt8 MaxValue()
  {
    return 0xFF;
  }

  template <>
  constexpr xiiUInt16 MaxValue()
  {
    return 0xFFFF;
  }

  template <>
  constexpr xiiUInt32 MaxValue()
  {
    return 0xFFFFFFFFu;
  }

#if XII_ENABLED(XII_COMPILER_CLANG)
#  if XII_ENABLED(XII_PLATFORM_32BIT)
  template <>
  constexpr size_t MaxValue()
  {
    return 0xFFFFFFFFu;
  }
#  endif
#endif

  template <>
  constexpr xiiUInt64 MaxValue()
  {
    return 0xFFFFFFFFFFFFFFFFull;
  }

  template <>
  constexpr xiiInt8 MaxValue()
  {
    return 0x7F;
  }

  template <>
  constexpr xiiInt16 MaxValue()
  {
    return 0x7FFF;
  }

  template <>
  constexpr xiiInt32 MaxValue()
  {
    return 0x7FFFFFFF;
  }

  template <>
  constexpr xiiInt64 MaxValue()
  {
    return 0x7FFFFFFFFFFFFFFFll;
  }

  template <>
  constexpr float MaxValue()
  {
    return 3.402823465e+38F;
  }

  template <>
  constexpr double MaxValue()
  {
    return 1.7976931348623158e+307;
  }

  //////////////////////////////////////////////////////////////////////////

  template <>
  constexpr xiiUInt8 MinValue()
  {
    return 0;
  }

  template <>
  constexpr xiiUInt16 MinValue()
  {
    return 0;
  }

  template <>
  constexpr xiiUInt32 MinValue()
  {
    return 0;
  }

  template <>
  constexpr xiiUInt64 MinValue()
  {
    return 0;
  }

  template <>
  constexpr xiiInt8 MinValue()
  {
    return -MaxValue<xiiInt8>() - 1;
  }

  template <>
  constexpr xiiInt16 MinValue()
  {
    return -MaxValue<xiiInt16>() - 1;
  }

  template <>
  constexpr xiiInt32 MinValue()
  {
    return -MaxValue<xiiInt32>() - 1;
  }

  template <>
  constexpr xiiInt64 MinValue()
  {
    return -MaxValue<xiiInt64>() - 1;
  }

  template <>
  constexpr float MinValue()
  {
    return -3.402823465e+38F;
  }

  template <>
  constexpr double MinValue()
  {
    return -1.7976931348623158e+307;
  }

  //////////////////////////////////////////////////////////////////////////

  template <>
  constexpr float HighValue()
  {
    return 1.8446726e+019f;
  }

  template <>
  constexpr double HighValue()
  {
    return 1.8446726e+150;
  }

  //////////////////////////////////////////////////////////////////////////

  template <>
  constexpr float FloatEpsilon()
  {
    return FLT_EPSILON;
  }

  template <>
  constexpr double FloatEpsilon()
  {
    return DBL_EPSILON;
  }

  template <typename TYPE>
  constexpr TYPE SqrtEpsilon()
  {
    if constexpr (std::is_same_v<TYPE, float>)
    {
      return (TYPE)0.000001;
    }
    else if constexpr (std::is_same_v<TYPE, double>)
    {
      return (TYPE)0.000001;
    }
    else
    {
      return (TYPE)0.000001;
    }
  }

  template <typename TYPE>
  constexpr TYPE VeryVerySmallEpsilon()
  {
    if constexpr (std::is_same_v<TYPE, float>)
    {
      return (TYPE)0.00000001;
    }
    else if constexpr (std::is_same_v<TYPE, double>)
    {
      return (TYPE)0.000000000001;
    }
    else
    {
      return (TYPE)0.00000001;
    }
  }

  template <typename TYPE>
  constexpr TYPE VerySmallEpsilon()
  {
    if constexpr (std::is_same_v<TYPE, float>)
    {
      return (TYPE)0.0000001;
    }
    else if constexpr (std::is_same_v<TYPE, double>)
    {
      return (TYPE)0.00000000001;
    }
    else
    {
      return (TYPE)0.0000001;
    }
  }

  template <typename TYPE>
  constexpr TYPE SmallEpsilon()
  {
    if constexpr (std::is_same_v<TYPE, float>)
    {
      return (TYPE)0.000001;
    }
    else if constexpr (std::is_same_v<TYPE, double>)
    {
      return (TYPE)0.0000000001;
    }
    else
    {
      return (TYPE)0.000001;
    }
  }

  template <typename TYPE>
  constexpr TYPE DefaultEpsilon()
  {
    if constexpr (std::is_same_v<TYPE, float>)
    {
      return (TYPE)0.00001;
    }
    else if constexpr (std::is_same_v<TYPE, double>)
    {
      return (TYPE)0.000000001;
    }
    else
    {
      return (TYPE)0.00001;
    }
  }

  template <typename TYPE>
  constexpr TYPE LargeEpsilon()
  {
    if constexpr (std::is_same_v<TYPE, float>)
    {
      return (TYPE)0.0001;
    }
    else if constexpr (std::is_same_v<TYPE, double>)
    {
      return (TYPE)0.00000001;
    }
    else
    {
      return (TYPE)0.0001;
    }
  }

  template <typename TYPE>
  constexpr TYPE HugeEpsilon()
  {
    if constexpr (std::is_same_v<TYPE, float>)
    {
      return (TYPE)0.001;
    }
    else if constexpr (std::is_same_v<TYPE, double>)
    {
      return (TYPE)0.0000001;
    }
    else
    {
      return (TYPE)0.001;
    }
  }

  template <typename TYPE>
  constexpr TYPE VeryHugeEpsilon()
  {
    if constexpr (std::is_same_v<TYPE, float>)
    {
      return (TYPE)0.01;
    }
    else if constexpr (std::is_same_v<TYPE, double>)
    {
      return (TYPE)0.000001;
    }
    else
    {
      return (TYPE)0.01;
    }
  }

  //////////////////////////////////////////////////////////////////////////

  template <>
  constexpr xiiUInt32 NumBits<xiiUInt8>()
  {
    return 8;
  }

  template <>
  constexpr xiiUInt32 NumBits<xiiUInt16>()
  {
    return 16;
  }

  template <>
  constexpr xiiUInt32 NumBits<xiiUInt32>()
  {
    return 32;
  }

  template <>
  constexpr xiiUInt32 NumBits<xiiUInt64>()
  {
    return 64;
  }

  template <>
  constexpr xiiUInt32 NumBits<xiiInt8>()
  {
    return 8;
  }

  template <>
  constexpr xiiUInt32 NumBits<xiiInt16>()
  {
    return 16;
  }

  template <>
  constexpr xiiUInt32 NumBits<xiiInt32>()
  {
    return 32;
  }

  template <>
  constexpr xiiUInt32 NumBits<xiiInt64>()
  {
    return 64;
  }

  template <>
  constexpr xiiUInt32 NumBits<float>()
  {
    return 32;
  }

  template <>
  constexpr xiiUInt32 NumBits<double>()
  {
    return 64;
  }

  //////////////////////////////////////////////////////////////////////////

} // namespace xiiMath
