#pragma once

#include <Foundation/Math/Math.h>

struct xiiMathFloatBits
{
  enum Enum
  {
    FULL,
    BITS_23,
    BITS_12
  };
};

struct xiiMathDoubleBits
{
  enum Enum
  {
    FULL,
    BITS_27,
    BITS_14
  };
};

#if XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX
#  include <Foundation/SimdMath/Implementation/AVX/AVXTypes_inl.h>
#elif XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSETypes_inl.h>
#elif XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONTypes_inl.h>
#elif XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUTypes_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
