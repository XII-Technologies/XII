#pragma once

#include <Foundation/Math/Vec4.h>

#include <arm_neon.h>

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
#  define XII_CHECK_SIMD_FLOAT_ALIGNMENT(x)  XII_CHECK_ALIGNMENT(x, 16)
#else
#  define XII_CHECK_SIMD_FLOAT_ALIGNMENT(x)
#endif

namespace xiiInternal
{
  using QuadDouble = xiiVec4d;
  using QuadUInt64 = xiiVec4U64;
  using QuadInt64  = xiiVec4I64;
  using QuadFloat  = float32x4_t;
  using QuadBool   = uint32x4_t;
  using QuadInt    = int32x4_t;
  using QuadUInt   = uint32x4_t;

  // Neon equivalent of _mm_movemask_ps
  XII_ALWAYS_INLINE uint32_t NeonMoveMask(uint32x4_t x)
  {
    // Isolate the sign bit of each vector element and shift it into its position in the final mask, the horizontally add to combine each element mask.
    alignas(16) static const int32_t shift[4] = {0, 1, 2, 3};
    return vaddvq_u32(vshlq_u32(vshrq_n_u32(x, 31), vld1q_s32(shift)));
  }

} // namespace xiiInternal

// Converts a xiiSwizzle into the mask selection format of __builtin_shufflevector
#define XII_TO_SHUFFLE(s) (s >> 12) & 3, (s >> 8) & 3, ((s >> 4) & 0x3) + 4, (s & 3) + 4
