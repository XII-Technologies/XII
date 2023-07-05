#pragma once

#define XII_SSE_20     0x20
#define XII_SSE_30     0x30
#define XII_SSE_31     0x31
#define XII_SSE_41     0x41
#define XII_SSE_42     0x42
#define XII_AVX_1      0x50
#define XII_AVX_2      0x51
#define XII_AVX_512    0x52

#define XII_SSE_LEVEL XII_AVX_2

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

#if XII_SSE_LEVEL >= XII_AVX_1
#  include <immintrin.h>
#endif

#if XII_ENABLED(XII_COMPILER_MSVC)
#  if XII_SSE_LEVEL >= XII_AVX_2
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
  using QuadDouble = __m256d;
  using QuadUInt64 = __m256i;
  using QuadInt64  = __m256i;
  using QuadFloat  = __m128;
  using QuadUInt   = __m128i;
  using QuadInt    = __m128i;
  using QuadBool   = __m128;
} // namespace xiiInternal

#include <Foundation/SimdMath/SimdSwizzle.h>

#define XII_SHUFFLE(a0, a1, b2, b3) ((a0) | ((a1) << 2) | ((b2) << 4) | ((b3) << 6))

#define XII_TO_SHUFFLE(s) ((((s) >> 12) & 0x03) | (((s) >> 6) & 0x0c) | ((s)&0x30) | (((s) << 6) & 0xc0))
