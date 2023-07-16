#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdFloat.h>

XII_CREATE_SIMPLE_TEST_GROUP(SimdMath);

XII_CREATE_SIMPLE_TEST(SimdMath, SimdFloat)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    // In debug the default constructor initializes everything with NaN.
    xiiSimdFloat vDefCtor;
    XII_TEST_BOOL(xiiMath::IsNaN((float)vDefCtor));
#else
// GCC assumes that the contents of the memory before calling the default constructor are irrelevant.
// So it optimizes away the 1,2,3,4 initializer completely.
#  if XII_DISABLED(XII_COMPILER_GCC)
    // Placement new of the default constructor should not have any effect on the previous data.
    alignas(16) float testBlock[4] = {1, 2, 3, 4};
    xiiSimdFloat*     pDefCtor     = ::new ((void*)&testBlock[0]) xiiSimdFloat;
    XII_TEST_BOOL_MSG((float)(*pDefCtor) == 1.0f, "Default constructed value is %f", (float)(*pDefCtor));
#  endif
#endif

    // Make sure the class didn't accidentally change in size.
#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) || (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE)
    XII_CHECK_AT_COMPILETIME(sizeof(xiiSimdFloat) == 16);
    XII_CHECK_AT_COMPILETIME(XII_ALIGNMENT_OF(xiiSimdFloat) == 16);
#endif

    xiiSimdFloat vInit1F(2.0f);
    XII_TEST_BOOL(vInit1F == 2.0f);

    // Make sure all components are set to the same value
#if ((XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) || (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE)) && XII_ENABLED(XII_COMPILER_MSVC)
    XII_TEST_BOOL(vInit1F.m_v.m128_f32[0] == 2.0f && vInit1F.m_v.m128_f32[1] == 2.0f && vInit1F.m_v.m128_f32[2] == 2.0f && vInit1F.m_v.m128_f32[3] == 2.0f);
#endif

    xiiSimdFloat vInit1I(1);
    XII_TEST_BOOL(vInit1I == 1.0f);

    // Make sure all components are set to the same value
#if ((XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) || (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE)) && XII_ENABLED(XII_COMPILER_MSVC)
    XII_TEST_BOOL(vInit1I.m_v.m128_f32[0] == 1.0f && vInit1I.m_v.m128_f32[1] == 1.0f && vInit1I.m_v.m128_f32[2] == 1.0f && vInit1I.m_v.m128_f32[3] == 1.0f);
#endif

    xiiSimdFloat vInit1U(4553u);
    XII_TEST_BOOL(vInit1U == 4553.0f);

    // Make sure all components are set to the same value
#if ((XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) || (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE)) && XII_ENABLED(XII_COMPILER_MSVC)
    XII_TEST_BOOL(vInit1U.m_v.m128_f32[0] == 4553.0f && vInit1U.m_v.m128_f32[1] == 4553.0f && vInit1U.m_v.m128_f32[2] == 4553.0f && vInit1U.m_v.m128_f32[3] == 4553.0f);
#endif

    xiiSimdFloat z = xiiSimdFloat::Zero();
    XII_TEST_BOOL(z == 0.0f);

    // Make sure all components are set to the same value
#if ((XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) || (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE)) && XII_ENABLED(XII_COMPILER_MSVC)
    XII_TEST_BOOL(z.m_v.m128_f32[0] == 0.0f && z.m_v.m128_f32[1] == 0.0f && z.m_v.m128_f32[2] == 0.0f && z.m_v.m128_f32[3] == 0.0f);
#endif
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Operators")
  {
    xiiSimdFloat a = 5.0f;
    xiiSimdFloat b = 2.0f;

    XII_TEST_FLOAT(a + b, 7.0f, xiiMath::SmallEpsilon<float>());
    XII_TEST_FLOAT(a - b, 3.0f, xiiMath::SmallEpsilon<float>());
    XII_TEST_FLOAT(a * b, 10.0f, xiiMath::SmallEpsilon<float>());
    XII_TEST_FLOAT(a / b, 2.5f, xiiMath::SmallEpsilon<float>());

    xiiSimdFloat c = 1.0f;
    c += a;
    XII_TEST_FLOAT(c, 6.0f, xiiMath::SmallEpsilon<float>());

    c = 1.0f;
    c -= b;
    XII_TEST_FLOAT(c, -1.0f, xiiMath::SmallEpsilon<float>());

    c = 1.0f;
    c *= a;
    XII_TEST_FLOAT(c, 5.0f, xiiMath::SmallEpsilon<float>());

    c = 1.0f;
    c /= a;
    XII_TEST_FLOAT(c, 0.2f, xiiMath::SmallEpsilon<float>());

    XII_TEST_BOOL(c.IsEqual(0.201f, xiiMath::HugeEpsilon<float>()));
    XII_TEST_BOOL(c.IsEqual(0.199f, xiiMath::HugeEpsilon<float>()));
    XII_TEST_BOOL(!c.IsEqual(0.202f, xiiMath::HugeEpsilon<float>()));
    XII_TEST_BOOL(!c.IsEqual(0.198f, xiiMath::HugeEpsilon<float>()));

    c = b;
    XII_TEST_BOOL(c == b);
    XII_TEST_BOOL(c != a);
    XII_TEST_BOOL(a > b);
    XII_TEST_BOOL(c >= b);
    XII_TEST_BOOL(b < a);
    XII_TEST_BOOL(b <= c);

    XII_TEST_BOOL(c == 2.0f);
    XII_TEST_BOOL(c != 5.0f);
    XII_TEST_BOOL(a > 2.0f);
    XII_TEST_BOOL(c >= 2.0f);
    XII_TEST_BOOL(b < 5.0f);
    XII_TEST_BOOL(b <= 2.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Misc")
  {
    xiiSimdFloat a = 2.0f;

    XII_TEST_FLOAT(a.GetReciprocal(), 0.5f, xiiMath::SmallEpsilon<float>());
    XII_TEST_FLOAT(a.GetReciprocal<xiiMathFloatBits::FULL>(), 0.5f, xiiMath::SmallEpsilon<float>());
    XII_TEST_FLOAT(a.GetReciprocal<xiiMathFloatBits::BITS_23>(), 0.5f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(a.GetReciprocal<xiiMathFloatBits::BITS_12>(), 0.5f, xiiMath::HugeEpsilon<float>());

    XII_TEST_FLOAT(a.GetSqrt(), 1.41421356f, xiiMath::SmallEpsilon<float>());
    XII_TEST_FLOAT(a.GetSqrt<xiiMathFloatBits::FULL>(), 1.41421356f, xiiMath::SmallEpsilon<float>());
    XII_TEST_FLOAT(a.GetSqrt<xiiMathFloatBits::BITS_23>(), 1.41421356f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(a.GetSqrt<xiiMathFloatBits::BITS_12>(), 1.41421356f, xiiMath::HugeEpsilon<float>());

    XII_TEST_FLOAT(a.GetInvSqrt(), 0.70710678f, xiiMath::SmallEpsilon<float>());
    XII_TEST_FLOAT(a.GetInvSqrt<xiiMathFloatBits::FULL>(), 0.70710678f, xiiMath::SmallEpsilon<float>());
    XII_TEST_FLOAT(a.GetInvSqrt<xiiMathFloatBits::BITS_23>(), 0.70710678f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(a.GetInvSqrt<xiiMathFloatBits::BITS_12>(), 0.70710678f, xiiMath::HugeEpsilon<float>());

    xiiSimdFloat b = 5.0f;
    XII_TEST_BOOL(a.Max(b) == b);
    XII_TEST_BOOL(a.Min(b) == a);

    xiiSimdFloat c = -4.0f;
    XII_TEST_FLOAT(c.Abs(), 4.0f, xiiMath::SmallEpsilon<float>());
  }
}
