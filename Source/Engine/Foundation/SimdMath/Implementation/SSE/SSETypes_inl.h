#pragma once

#include <Foundation/Math/Vec4.h>

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

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
#  define XII_CHECK_SIMD_FLOAT_ALIGNMENT(x) XII_CHECK_ALIGNMENT(x, 16)
#else
#  define XII_CHECK_SIMD_FLOAT_ALIGNMENT(x)
#endif

namespace xiiInternal
{
  using QuadDouble = xiiVec4d;
  using QuadUInt64 = xiiVec4U64;
  using QuadInt64  = xiiVec4I64;
  using QuadFloat  = __m128;
  using QuadUInt   = __m128i;
  using QuadInt    = __m128i;
  using QuadBool   = __m128;
} // namespace xiiInternal

#include <Foundation/SimdMath/SimdSwizzle.h>

#define XII_SHUFFLE(a0, a1, b2, b3) ((a0) | ((a1) << 2) | ((b2) << 4) | ((b3) << 6))

#define XII_TO_SHUFFLE(s) ((((s) >> 12) & 0x03) | (((s) >> 6) & 0x0c) | ((s)&0x30) | (((s) << 6) & 0xc0))
