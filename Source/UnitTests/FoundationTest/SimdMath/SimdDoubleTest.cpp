/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdDouble.h>

XII_CREATE_SIMPLE_TEST(SimdMath, SimdDouble)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    // In debug the default constructor initializes everything with NaN.
    xiiSimdDouble vDefCtor;
    XII_TEST_BOOL(xiiMath::IsNaN((double)vDefCtor));
#else
// GCC assumes that the contents of the memory before calling the default constructor are irrelevant.
// So it optimizes away the 1,2,3,4 initializer completely.
#  if XII_DISABLED(XII_COMPILER_GCC)
    // Placement new of the default constructor should not have any effect on the previous data.
    alignas(32) double testBlock[4] = {1, 2, 3, 4};
    xiiSimdDouble*     pDefCtor     = ::new ((void*)&testBlock[0]) xiiSimdDouble;
    XII_TEST_BOOL_MSG((double)(*pDefCtor) == 1.0, "Default constructed value is %f", (double)(*pDefCtor));
#  endif
#endif

    // Make sure the class didn't accidentally change in size.
#if XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX
    static_assert(sizeof(xiiSimdDouble) == 32);
    static_assert(alignof(xiiSimdDouble) == 32);
#endif

    xiiSimdDouble vInit1D(2.0);
    XII_TEST_BOOL(vInit1D == 2.0);

    // Make sure all components are set to the same value.
#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) && XII_ENABLED(XII_COMPILER_MSVC)
    XII_TEST_BOOL(vInit1D.m_v.m256d_f64[0] == 2.0 && vInit1D.m_v.m256d_f64[1] == 2.0 && vInit1D.m_v.m256d_f64[2] == 2.0 && vInit1D.m_v.m256d_f64[3] == 2.0);
#endif

    xiiSimdDouble vInit1F(2.0f);
    XII_TEST_BOOL(vInit1F == 2.0);

    // Make sure all components are set to the same value.
#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) && XII_ENABLED(XII_COMPILER_MSVC)
    XII_TEST_BOOL(vInit1F.m_v.m256d_f64[0] == 2.0 && vInit1F.m_v.m256d_f64[1] == 2.0 && vInit1F.m_v.m256d_f64[2] == 2.0 && vInit1F.m_v.m256d_f64[3] == 2.0);
#endif

    xiiSimdDouble vInit1I(1);
    XII_TEST_BOOL(vInit1I == 1.0);

    // Make sure all components are set to the same value.
#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) && XII_ENABLED(XII_COMPILER_MSVC)
    XII_TEST_BOOL(vInit1I.m_v.m256d_f64[0] == 1.0 && vInit1I.m_v.m256d_f64[1] == 1.0 && vInit1I.m_v.m256d_f64[2] == 1.0 && vInit1I.m_v.m256d_f64[3] == 1.0);
#endif

    xiiSimdDouble vInit1U(4553u);
    XII_TEST_BOOL(vInit1U == 4553.0);

    // Make sure all components are set to the same value
#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) && XII_ENABLED(XII_COMPILER_MSVC)
    XII_TEST_BOOL(vInit1U.m_v.m256d_f64[0] == 4553.0 && vInit1U.m_v.m256d_f64[1] == 4553.0 && vInit1U.m_v.m256d_f64[2] == 4553.0 && vInit1U.m_v.m256d_f64[3] == 4553.0);
#endif

    {
      xiiSimdDouble z = xiiSimdDouble::MakeZero();
      XII_TEST_BOOL(z == 0.0);

      // Make sure all components are set to the same value
#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) && XII_ENABLED(XII_COMPILER_MSVC)
      XII_TEST_BOOL(z.m_v.m256d_f64[0] == 0.0 && z.m_v.m256d_f64[1] == 0.0 && z.m_v.m256d_f64[2] == 0.0 && z.m_v.m256d_f64[3] == 0.0);
#endif
    }

    {
      xiiSimdDouble z = xiiSimdDouble::MakeNaN();

      // Make sure all components are set to the same value
#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) && XII_ENABLED(XII_COMPILER_MSVC)
      XII_TEST_BOOL(xiiMath::IsNaN(z.m_v.m256d_f64[0]));
      XII_TEST_BOOL(xiiMath::IsNaN(z.m_v.m256d_f64[1]));
      XII_TEST_BOOL(xiiMath::IsNaN(z.m_v.m256d_f64[2]));
      XII_TEST_BOOL(xiiMath::IsNaN(z.m_v.m256d_f64[3]));
#endif
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Operators")
  {
    xiiSimdDouble a = 5.0;
    xiiSimdDouble b = 2.0;

    XII_TEST_DOUBLE(a + b, 7.0, xiiMath::SmallEpsilon<double>());
    XII_TEST_DOUBLE(a - b, 3.0, xiiMath::SmallEpsilon<double>());
    XII_TEST_DOUBLE(a * b, 10.0, xiiMath::SmallEpsilon<double>());
    XII_TEST_DOUBLE(a / b, 2.5, xiiMath::SmallEpsilon<double>());

    xiiSimdDouble c = 1.0;

    c += a;
    XII_TEST_DOUBLE(c, 6.0, xiiMath::SmallEpsilon<double>());

    c = 1.0;
    c -= b;
    XII_TEST_DOUBLE(c, -1.0, xiiMath::SmallEpsilon<double>());

    c = 1.0;
    c *= a;
    XII_TEST_DOUBLE(c, 5.0, xiiMath::SmallEpsilon<double>());

    c = 1.0;
    c /= a;
    XII_TEST_DOUBLE(c, 0.2, xiiMath::SmallEpsilon<double>());

    XII_TEST_BOOL(c.IsEqual(0.20000005, xiiMath::HugeEpsilon<double>()));
    XII_TEST_BOOL(c.IsEqual(0.19999995, xiiMath::HugeEpsilon<double>()));
    XII_TEST_BOOL(!c.IsEqual(0.2000002, xiiMath::HugeEpsilon<double>()));
    XII_TEST_BOOL(!c.IsEqual(0.1999998, xiiMath::HugeEpsilon<double>()));

    c = b;
    XII_TEST_BOOL(c == b);
    XII_TEST_BOOL(c != a);
    XII_TEST_BOOL(a > b);
    XII_TEST_BOOL(c >= b);
    XII_TEST_BOOL(b < a);
    XII_TEST_BOOL(b <= c);

    XII_TEST_BOOL(c == 2.0);
    XII_TEST_BOOL(c != 5.0);
    XII_TEST_BOOL(a > 2.0);
    XII_TEST_BOOL(c >= 2.0);
    XII_TEST_BOOL(b < 5.0);
    XII_TEST_BOOL(b <= 2.0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Misc")
  {
    xiiSimdDouble a = 2.0;

    XII_TEST_DOUBLE(a.GetReciprocal(), 0.5, xiiMath::SmallEpsilon<double>());
    XII_TEST_DOUBLE(a.GetReciprocal<xiiMathDoubleBits::FULL>(), 0.5, xiiMath::SmallEpsilon<double>());
#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) && (XII_SSE_LEVEL >= XII_AVX_512)
    XII_TEST_DOUBLE(a.GetReciprocal<xiiMathDoubleBits::BITS_27>(), 0.5, xiiMath::DefaultEpsilon<double>());
    XII_TEST_DOUBLE(a.GetReciprocal<xiiMathDoubleBits::BITS_14>(), 0.5, xiiMath::HugeEpsilon<double>());
#endif

    XII_TEST_DOUBLE(a.GetSqrt(), 1.4142135623730951, xiiMath::SmallEpsilon<double>());
    XII_TEST_DOUBLE(a.GetSqrt<xiiMathDoubleBits::FULL>(), 1.4142135623730951, xiiMath::SmallEpsilon<double>());
#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) && (XII_SSE_LEVEL >= XII_AVX_512)
    XII_TEST_DOUBLE(a.GetSqrt<xiiMathDoubleBits::BITS_27>(), 1.4142135623730951, xiiMath::DefaultEpsilon<double>());
    XII_TEST_DOUBLE(a.GetSqrt<xiiMathDoubleBits::BITS_14>(), 1.4142135623730951, xiiMath::HugeEpsilon<double>());
#endif

    XII_TEST_DOUBLE(a.GetInvSqrt(), 0.70710678118654746, xiiMath::SmallEpsilon<double>());
    XII_TEST_DOUBLE(a.GetInvSqrt<xiiMathDoubleBits::FULL>(), 0.70710678118654746, xiiMath::SmallEpsilon<double>());
#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) && (XII_SSE_LEVEL >= XII_AVX_512)
    XII_TEST_DOUBLE(a.GetInvSqrt<xiiMathDoubleBits::BITS_27>(), 0.70710678118654746, xiiMath::DefaultEpsilon<double>());
    XII_TEST_DOUBLE(a.GetInvSqrt<xiiMathDoubleBits::BITS_14>(), 0.70710678118654746, xiiMath::HugeEpsilon<double>());
#endif

    xiiSimdDouble b = 5.0;
    XII_TEST_BOOL(a.Max(b) == b);
    XII_TEST_BOOL(a.Min(b) == a);

    xiiSimdDouble c = -4.0;
    XII_TEST_DOUBLE(c.Abs(), 4.0, xiiMath::SmallEpsilon<double>());
  }
}
