#pragma once

#define XII_SSE_20     0x20
#define XII_SSE_30     0x30
#define XII_SSE_31     0x31
#define XII_SSE_41     0x41
#define XII_SSE_42     0x42
#define XII_SSE_AVX    0x50
#define XII_SSE_AVX2   0x51
#define XII_SSE_AVX512 0x52

#define XII_SSE_LEVEL XII_SSE_AVX2

#if XII_SSE_LEVEL >= XII_SSE_20
#  include <emmintrin.h>
#endif

#if XII_SSE_LEVEL >= XII_SSE_30
#  include <pmmintrin.h>
#endif

#if XII_SSE_LEVEL >= XII_SSE_31
#  include <tmmintrin.h>
#endif

#if XII_SSE_LEVEL >= XII_SSE_41
#  include <smmintrin.h>
#endif

#if XII_SSE_LEVEL >= XII_SSE_42
#  include <nmmintrin.h>
#endif

#if XII_SSE_LEVEL >= XII_SSE_AVX
#  include <immintrin.h>
#endif

#if XII_DISABLED(XII_COMPILER_GCC) && XII_DISABLED(XII_COMPILER_CLANG)
#  if XII_SSE_LEVEL >= XII_SSE_AVX2
#    include <zmmintrin.h>
#  endif
#endif

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
#  define XII_CHECK_SIMD_FLOAT_ALIGNMENT  XII_CHECK_ALIGNMENT_16
#  define XII_CHECK_SIMD_DOUBLE_ALIGNMENT XII_CHECK_ALIGNMENT_32
#else
#  define XII_CHECK_SIMD_FLOAT_ALIGNMENT(x)
#  define XII_CHECK_SIMD_DOUBLE_ALIGNMENT(x)
#endif

namespace xiiInternal
{
  typedef __m256d QuadDouble;
  typedef __m128  QuadFloat;
  typedef __m128i QuadUInt;
  typedef __m128i QuadInt;
  typedef __m128  QuadBool;
} // namespace xiiInternal

#include <Foundation/SimdMath/SimdSwizzle.h>

#define XII_SHUFFLE(a0, a1, b2, b3) ((a0) | ((a1) << 2) | ((b2) << 4) | ((b3) << 6))

#define XII_TO_SHUFFLE(s) (((s >> 12) & 0x03) | ((s >> 6) & 0x0c) | (s & 0x30) | ((s << 6) & 0xc0))
