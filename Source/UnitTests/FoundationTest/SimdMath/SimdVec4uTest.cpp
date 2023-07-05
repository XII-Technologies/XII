#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdVec4u.h>

XII_CREATE_SIMPLE_TEST(SimdMath, SimdVec4u)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    // In debug the default constructor initializes everything with 0xCDCDCDCD.
    xiiSimdVec4u vDefCtor;
    XII_TEST_BOOL(vDefCtor.x() == 0xCDCDCDCD && vDefCtor.y() == 0xCDCDCDCD && vDefCtor.z() == 0xCDCDCDCD && vDefCtor.w() == 0xCDCDCDCD);
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    alignas(16) float testBlock[4] = {1, 2, 3, 4};
    xiiSimdVec4u*     pDefCtor     = ::new ((void*)&testBlock[0]) xiiSimdVec4u;
    XII_TEST_BOOL(testBlock[0] == 1 && testBlock[1] == 2 && testBlock[2] == 3 && testBlock[3] == 4);
#endif

    // Make sure the class didn't accidentally change in size.
#if XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX || XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE
    XII_CHECK_AT_COMPILETIME(sizeof(xiiSimdVec4u) == 16);
    XII_CHECK_AT_COMPILETIME(XII_ALIGNMENT_OF(xiiSimdVec4u) == 16);
#endif

    xiiSimdVec4u a(2);
    XII_TEST_BOOL(a.x() == 2 && a.y() == 2 && a.z() == 2 && a.w() == 2);

    xiiSimdVec4u b(1, 2, 3, 0xFFFFFFFFu);
    XII_TEST_BOOL(b.x() == 1 && b.y() == 2 && b.z() == 3 && b.w() == 0xFFFFFFFFu);

    // Make sure all components have the correct values
#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX || XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE) && XII_ENABLED(XII_COMPILER_MSVC)
    XII_TEST_BOOL(b.m_v.m128i_u32[0] == 1 && b.m_v.m128i_u32[1] == 2 && b.m_v.m128i_u32[2] == 3 && b.m_v.m128i_u32[3] == 0xFFFFFFFFu);
#endif

    xiiSimdVec4u copy(b);
    XII_TEST_BOOL(copy.x() == 1 && copy.y() == 2 && copy.z() == 3 && copy.w() == 0xFFFFFFFFu);

    XII_TEST_BOOL(copy.GetComponent<0>() == 1 && copy.GetComponent<1>() == 2 && copy.GetComponent<2>() == 3 && copy.GetComponent<3>() == 0xFFFFFFFFu);

    xiiSimdVec4u vZero = xiiSimdVec4u::ZeroVector();
    XII_TEST_BOOL(vZero.x() == 0 && vZero.y() == 0 && vZero.z() == 0 && vZero.w() == 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Setter")
  {
    xiiSimdVec4u a;
    a.Set(2);
    XII_TEST_BOOL(a.x() == 2 && a.y() == 2 && a.z() == 2 && a.w() == 2);

    xiiSimdVec4u b;
    b.Set(1, 2, 3, 4);
    XII_TEST_BOOL(b.x() == 1 && b.y() == 2 && b.z() == 3 && b.w() == 4);

    xiiSimdVec4u vSetZero;
    vSetZero.SetZero();
    XII_TEST_BOOL(vSetZero.x() == 0 && vSetZero.y() == 0 && vSetZero.z() == 0 && vSetZero.w() == 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Conversion")
  {
    xiiSimdVec4u ua(-10000, 5, -7, 11);

    xiiSimdVec4i ia(ua);
    XII_TEST_BOOL(ia.x() == -10000 && ia.y() == 5 && ia.z() == -7 && ia.w() == 11);

    xiiSimdVec4f fa = ua.ToFloat();
    XII_TEST_BOOL(fa.x() == 4294957296.0f && fa.y() == 5.0f && fa.z() == 4294967289.0f && fa.w() == 11.0f);

    fa             = xiiSimdVec4f(-2.3f, 5.7f, -4294967040.0f, 4294967040.0f);
    xiiSimdVec4u b = xiiSimdVec4u::Truncate(fa);
    XII_TEST_INT(b.x(), 0);
    XII_TEST_INT(b.y(), 5);
    XII_TEST_INT(b.z(), 0);
    XII_TEST_INT(b.w(), 4294967040);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Swizzle")
  {
    xiiSimdVec4u a(3, 5, 7, 9);

    xiiSimdVec4u b = a.Get<xiiSwizzle::XXXX>();
    XII_TEST_BOOL(b.x() == 3 && b.y() == 3 && b.z() == 3 && b.w() == 3);

    b = a.Get<xiiSwizzle::YYYX>();
    XII_TEST_BOOL(b.x() == 5 && b.y() == 5 && b.z() == 5 && b.w() == 3);

    b = a.Get<xiiSwizzle::ZZZX>();
    XII_TEST_BOOL(b.x() == 7 && b.y() == 7 && b.z() == 7 && b.w() == 3);

    b = a.Get<xiiSwizzle::WWWX>();
    XII_TEST_BOOL(b.x() == 9 && b.y() == 9 && b.z() == 9 && b.w() == 3);

    b = a.Get<xiiSwizzle::WZYX>();
    XII_TEST_BOOL(b.x() == 9 && b.y() == 7 && b.z() == 5 && b.w() == 3);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Operators")
  {
    {
      xiiSimdVec4u a(-3, 5, -7, 9);
      xiiSimdVec4u b(8, 6, 4, 2);
      xiiSimdVec4u c;
      c = a + b;
      XII_TEST_BOOL(c.x() == 5 && c.y() == 11 && c.z() == -3 && c.w() == 11);

      c = a - b;
      XII_TEST_BOOL(c.x() == -11 && c.y() == -1 && c.z() == -11 && c.w() == 7);

      a.Set(0xFFFFFFFF);
      c = a.CompMul(b);
      XII_TEST_BOOL(c.x() == 4294967288u && c.y() == 4294967290u && c.z() == 4294967292u && c.w() == 4294967294u);
    }

    {
      xiiSimdVec4u a(XII_BIT(1), XII_BIT(2), XII_BIT(3), XII_BIT(4));
      xiiSimdVec4u b(XII_BIT(4), XII_BIT(3), XII_BIT(3), XII_BIT(5) - 1);
      xiiSimdVec4u c;

      c = a | b;
      XII_TEST_BOOL(c.x() == (XII_BIT(1) | XII_BIT(4)) && c.y() == (XII_BIT(2) | XII_BIT(3)) && c.z() == XII_BIT(3) && c.w() == XII_BIT(5) - 1);

      c = a & b;
      XII_TEST_BOOL(c.x() == 0 && c.y() == 0 && c.z() == XII_BIT(3) && c.w() == XII_BIT(4));

      c = a ^ b;
      XII_TEST_BOOL(c.x() == (XII_BIT(1) | XII_BIT(4)) && c.y() == (XII_BIT(2) | XII_BIT(3)) && c.z() == 0 && c.w() == XII_BIT(4) - 1);

      c = ~a;
      XII_TEST_BOOL(c.x() == 0xFFFFFFFD && c.y() == 0xFFFFFFFB && c.z() == 0xFFFFFFF7 && c.w() == 0xFFFFFFEF);

      c = a << 3;
      XII_TEST_BOOL(c.x() == XII_BIT(4) && c.y() == XII_BIT(5) && c.z() == XII_BIT(6) && c.w() == XII_BIT(7));

      c = a >> 1;
      XII_TEST_BOOL(c.x() == XII_BIT(0) && c.y() == XII_BIT(1) && c.z() == XII_BIT(2) && c.w() == XII_BIT(3));
    }

    {
      xiiSimdVec4u a(-3, 5, -7, 9);
      xiiSimdVec4u b(8, 6, 4, 2);

      xiiSimdVec4u c = a;
      c += b;
      XII_TEST_BOOL(c.x() == 5 && c.y() == 11 && c.z() == -3 && c.w() == 11);

      c = a;
      c -= b;
      XII_TEST_BOOL(c.x() == -11 && c.y() == -1 && c.z() == -11 && c.w() == 7);
    }

    {
      xiiSimdVec4u a(XII_BIT(1), XII_BIT(2), XII_BIT(3), XII_BIT(4));
      xiiSimdVec4u b(XII_BIT(4), XII_BIT(3), XII_BIT(3), XII_BIT(5) - 1);

      xiiSimdVec4u c = a;
      c |= b;
      XII_TEST_BOOL(c.x() == (XII_BIT(1) | XII_BIT(4)) && c.y() == (XII_BIT(2) | XII_BIT(3)) && c.z() == XII_BIT(3) && c.w() == XII_BIT(5) - 1);

      c = a;
      c &= b;
      XII_TEST_BOOL(c.x() == 0 && c.y() == 0 && c.z() == XII_BIT(3) && c.w() == XII_BIT(4));

      c = a;
      c ^= b;
      XII_TEST_BOOL(c.x() == (XII_BIT(1) | XII_BIT(4)) && c.y() == (XII_BIT(2) | XII_BIT(3)) && c.z() == 0 && c.w() == XII_BIT(4) - 1);

      c = a;
      c <<= 3;
      XII_TEST_BOOL(c.x() == XII_BIT(4) && c.y() == XII_BIT(5) && c.z() == XII_BIT(6) && c.w() == XII_BIT(7));

      c = a;
      c >>= 1;
      XII_TEST_BOOL(c.x() == XII_BIT(0) && c.y() == XII_BIT(1) && c.z() == XII_BIT(2) && c.w() == XII_BIT(3));

      c = xiiSimdVec4u(-2, -4, -7, -8);
      XII_TEST_BOOL(c.x() == 0xFFFFFFFE && c.y() == 0xFFFFFFFC && c.z() == 0xFFFFFFF9 && c.w() == 0xFFFFFFF8);
      c >>= 1;
      XII_TEST_BOOL(c.x() == 0x7FFFFFFF && c.y() == 0x7FFFFFFE && c.z() == 0x7FFFFFFC && c.w() == 0x7FFFFFFC);
    }

    {
      xiiSimdVec4u a(-3, 5, -7, 9);
      xiiSimdVec4u b(8, 6, 4, 2);
      xiiSimdVec4u c;

      c = a.CompMin(b);
      XII_TEST_BOOL(c.x() == 8 && c.y() == 5 && c.z() == 4 && c.w() == 2);

      c = a.CompMax(b);
      XII_TEST_BOOL(c.x() == -3 && c.y() == 6 && c.z() == -7 && c.w() == 9);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Comparison")
  {
    xiiSimdVec4u a(-7, 5, 4, 3);
    xiiSimdVec4u b(8, 6, 4, -2);
    xiiSimdVec4b cmp;

    cmp = a == b;
    XII_TEST_BOOL(!cmp.x() && !cmp.y() && cmp.z() && !cmp.w());

    cmp = a != b;
    XII_TEST_BOOL(cmp.x() && cmp.y() && !cmp.z() && cmp.w());

    cmp = a <= b;
    XII_TEST_BOOL(!cmp.x() && cmp.y() && cmp.z() && cmp.w());

    cmp = a < b;
    XII_TEST_BOOL(!cmp.x() && cmp.y() && !cmp.z() && cmp.w());

    cmp = a >= b;
    XII_TEST_BOOL(cmp.x() && !cmp.y() && cmp.z() && !cmp.w());

    cmp = a > b;
    XII_TEST_BOOL(cmp.x() && !cmp.y() && !cmp.z() && !cmp.w());
  }
}
