#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdVec4b.h>

XII_CREATE_SIMPLE_TEST(SimdMath, SimdVec4b)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
#if XII_DISABLED(XII_COMPILER_GCC) && XII_DISABLED(XII_COMPILE_FOR_DEBUG)
    // Placement new of the default constructor should not have any effect on the previous data.
    alignas(16) float testBlock[4] = {1, 2, 3, 4};
    xiiSimdVec4b*     pDefCtor     = ::new ((void*)&testBlock[0]) xiiSimdVec4b;
    XII_TEST_BOOL(testBlock[0] == 1.0f && testBlock[1] == 2.0f && testBlock[2] == 3.0f && testBlock[3] == 4.0f);
#endif

    // Make sure the class didn't accidentally change in size.
#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) || (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE)
    XII_CHECK_AT_COMPILETIME(sizeof(xiiSimdVec4b) == 16);
    XII_CHECK_AT_COMPILETIME(XII_ALIGNMENT_OF(xiiSimdVec4b) == 16);
#endif

    xiiSimdVec4b vInit1B(true);
    XII_TEST_BOOL(vInit1B.x() == true && vInit1B.y() == true && vInit1B.z() == true && vInit1B.w() == true);

    // Make sure all components have the correct value
#if ((XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) || (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE)) && XII_ENABLED(XII_COMPILER_MSVC)
    XII_TEST_BOOL(vInit1B.m_v.m128_u32[0] == 0xFFFFFFFF && vInit1B.m_v.m128_u32[1] == 0xFFFFFFFF && vInit1B.m_v.m128_u32[2] == 0xFFFFFFFF &&
                  vInit1B.m_v.m128_u32[3] == 0xFFFFFFFF);
#endif

    xiiSimdVec4b vInit4B(false, true, false, true);
    XII_TEST_BOOL(vInit4B.x() == false && vInit4B.y() == true && vInit4B.z() == false && vInit4B.w() == true);

    // Make sure all components have the correct value
#if ((XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) || (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE)) && XII_ENABLED(XII_COMPILER_MSVC)
    XII_TEST_BOOL(vInit4B.m_v.m128_u32[0] == 0 && vInit4B.m_v.m128_u32[1] == 0xFFFFFFFF && vInit4B.m_v.m128_u32[2] == 0 && vInit4B.m_v.m128_u32[3] == 0xFFFFFFFF);
#endif

    xiiSimdVec4b vCopy(vInit4B);
    XII_TEST_BOOL(vCopy.x() == false && vCopy.y() == true && vCopy.z() == false && vCopy.w() == true);

    XII_TEST_BOOL(vCopy.GetComponent<0>() == false && vCopy.GetComponent<1>() == true && vCopy.GetComponent<2>() == false && vCopy.GetComponent<3>() == true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Swizzle")
  {
    xiiSimdVec4b a(true, false, true, false);

    xiiSimdVec4b b = a.Get<xiiSwizzle::XXXX>();
    XII_TEST_BOOL(b.x() && b.y() && b.z() && b.w());

    b = a.Get<xiiSwizzle::YYYX>();
    XII_TEST_BOOL(!b.x() && !b.y() && !b.z() && b.w());

    b = a.Get<xiiSwizzle::ZZZX>();
    XII_TEST_BOOL(b.x() && b.y() && b.z() && b.w());

    b = a.Get<xiiSwizzle::WWWX>();
    XII_TEST_BOOL(!b.x() && !b.y() && !b.z() && b.w());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Operators")
  {
    xiiSimdVec4b a(true, false, true, false);
    xiiSimdVec4b b(false, true, true, false);

    xiiSimdVec4b c = a && b;
    XII_TEST_BOOL(!c.x() && !c.y() && c.z() && !c.w());

    c = a || b;
    XII_TEST_BOOL(c.x() && c.y() && c.z() && !c.w());

    c = !a;
    XII_TEST_BOOL(!c.x() && c.y() && !c.z() && c.w());
    XII_TEST_BOOL(c.AnySet<2>());
    XII_TEST_BOOL(!c.AllSet<4>());
    XII_TEST_BOOL(!c.NoneSet<4>());

    c = c || a;
    XII_TEST_BOOL(c.AnySet<4>());
    XII_TEST_BOOL(c.AllSet<4>());
    XII_TEST_BOOL(!c.NoneSet<4>());

    c = !c;
    XII_TEST_BOOL(!c.AnySet<4>());
    XII_TEST_BOOL(!c.AllSet<4>());
    XII_TEST_BOOL(c.NoneSet<4>());

    c = a == b;
    XII_TEST_BOOL(!c.x() && !c.y() && c.z() && c.w());

    c = a != b;
    XII_TEST_BOOL(c.x() && c.y() && !c.z() && !c.w());

    XII_TEST_BOOL(a.AllSet<1>());
    XII_TEST_BOOL(b.NoneSet<1>());

    xiiSimdVec4b cmp(false, true, false, true);
    c = xiiSimdVec4b::Select(cmp, a, b);
    XII_TEST_BOOL(!c.x() && !c.y() && c.z() && !c.w());
  }
}
