/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdVec4u.h>

XII_CREATE_SIMPLE_TEST(SimdMath, SimdVec4i)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
#if XII_ENABLED(XII_MATH_CHECK_FOR_NAN)
    // In debug the default constructor initializes everything with 0xCDCDCDCD.
    xiiSimdVec4i vDefCtor;
    XII_TEST_BOOL(vDefCtor.x() == 0xCDCDCDCD && vDefCtor.y() == 0xCDCDCDCD && vDefCtor.z() == 0xCDCDCDCD && vDefCtor.w() == 0xCDCDCDCD);
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    alignas(16) float testBlock[4] = {1, 2, 3, 4};
    xiiSimdVec4i*     pDefCtor     = ::new ((void*)&testBlock[0]) xiiSimdVec4i;
    XII_TEST_BOOL(testBlock[0] == 1 && testBlock[1] == 2 && testBlock[2] == 3 && testBlock[3] == 4);
#endif

    // Make sure the class didn't accidentally change in size.
#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) || (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE)
    static_assert(sizeof(xiiSimdVec4i) == 16);
    static_assert(alignof(xiiSimdVec4i) == 16);
#endif

    xiiSimdVec4i a(2);
    XII_TEST_BOOL(a.x() == 2 && a.y() == 2 && a.z() == 2 && a.w() == 2);

    xiiSimdVec4i b(1, 2, 3, 4);
    XII_TEST_BOOL(b.x() == 1 && b.y() == 2 && b.z() == 3 && b.w() == 4);

    // Make sure all components have the correct values
#if ((XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) || (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE)) && XII_ENABLED(XII_COMPILER_MSVC)
    XII_TEST_BOOL(b.m_v.m128i_i32[0] == 1 && b.m_v.m128i_i32[1] == 2 && b.m_v.m128i_i32[2] == 3 && b.m_v.m128i_i32[3] == 4);
#endif

    xiiSimdVec4i copy(b);
    XII_TEST_BOOL(copy.x() == 1 && copy.y() == 2 && copy.z() == 3 && copy.w() == 4);

    XII_TEST_BOOL(copy.GetComponent<0>() == 1 && copy.GetComponent<1>() == 2 && copy.GetComponent<2>() == 3 && copy.GetComponent<3>() == 4);

    xiiSimdVec4i vZero = xiiSimdVec4i::MakeZero();
    XII_TEST_BOOL(vZero.x() == 0 && vZero.y() == 0 && vZero.z() == 0 && vZero.w() == 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Setter")
  {
    xiiSimdVec4i a;
    a.Set(2);
    XII_TEST_BOOL(a.x() == 2 && a.y() == 2 && a.z() == 2 && a.w() == 2);

    xiiSimdVec4i b;
    b.Set(1, 2, 3, 4);
    XII_TEST_BOOL(b.x() == 1 && b.y() == 2 && b.z() == 3 && b.w() == 4);

    xiiSimdVec4i vSetZero;
    vSetZero.SetZero();
    XII_TEST_BOOL(vSetZero.x() == 0 && vSetZero.y() == 0 && vSetZero.z() == 0 && vSetZero.w() == 0);

    {
      int          testBlock[4] = {1, 2, 3, 4};
      xiiSimdVec4i x;
      x.Load<1>(testBlock);
      XII_TEST_BOOL(x.x() == 1 && x.y() == 0 && x.z() == 0 && x.w() == 0);

      xiiSimdVec4i xy;
      xy.Load<2>(testBlock);
      XII_TEST_BOOL(xy.x() == 1 && xy.y() == 2 && xy.z() == 0 && xy.w() == 0);

      xiiSimdVec4i xyz;
      xyz.Load<3>(testBlock);
      XII_TEST_BOOL(xyz.x() == 1 && xyz.y() == 2 && xyz.z() == 3 && xyz.w() == 0);

      xiiSimdVec4i xyzw;
      xyzw.Load<4>(testBlock);
      XII_TEST_BOOL(xyzw.x() == 1 && xyzw.y() == 2 && xyzw.z() == 3 && xyzw.w() == 4);

      XII_TEST_INT(xyzw.GetComponent<0>(), 1);
      XII_TEST_INT(xyzw.GetComponent<1>(), 2);
      XII_TEST_INT(xyzw.GetComponent<2>(), 3);
      XII_TEST_INT(xyzw.GetComponent<3>(), 4);

      // Make sure all components have the correct values
#if ((XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) || (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE)) && XII_ENABLED(XII_COMPILER_MSVC)
      XII_TEST_BOOL(xyzw.m_v.m128i_i32[0] == 1 && xyzw.m_v.m128i_i32[1] == 2 && xyzw.m_v.m128i_i32[2] == 3 && xyzw.m_v.m128i_i32[3] == 4);
#endif
    }

    {
      int testBlock[4] = {7, 7, 7, 7};
      int mem[4]       = {};

      xiiSimdVec4i b2(1, 2, 3, 4);

      memcpy(mem, testBlock, 16);
      b2.Store<1>(mem);
      XII_TEST_BOOL(mem[0] == 1 && mem[1] == 7 && mem[2] == 7 && mem[3] == 7);

      memcpy(mem, testBlock, 16);
      b2.Store<2>(mem);
      XII_TEST_BOOL(mem[0] == 1 && mem[1] == 2 && mem[2] == 7 && mem[3] == 7);

      memcpy(mem, testBlock, 16);
      b2.Store<3>(mem);
      XII_TEST_BOOL(mem[0] == 1 && mem[1] == 2 && mem[2] == 3 && mem[3] == 7);

      memcpy(mem, testBlock, 16);
      b2.Store<4>(mem);
      XII_TEST_BOOL(mem[0] == 1 && mem[1] == 2 && mem[2] == 3 && mem[3] == 4);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Conversion")
  {
    xiiSimdVec4i ia(-3, 5, -7, 11);

    xiiSimdVec4u ua(ia);
    XII_TEST_BOOL(ua.x() == -3 && ua.y() == 5 && ua.z() == -7 && ua.w() == 11);

    xiiSimdVec4f fa = ia.ToFloat();
    XII_TEST_BOOL(fa.x() == -3.0f && fa.y() == 5.0f && fa.z() == -7.0f && fa.w() == 11.0f);

    fa             = xiiSimdVec4f(-2.3f, 5.7f, -2147483520.0f, 2147483520.0f);
    xiiSimdVec4i b = xiiSimdVec4i::Truncate(fa);
    XII_TEST_INT(b.x(), -2);
    XII_TEST_INT(b.y(), 5);
    XII_TEST_INT(b.z(), -2147483520);
    XII_TEST_INT(b.w(), 2147483520);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Swizzle")
  {
    xiiSimdVec4i a(3, 5, 7, 9);

    xiiSimdVec4i b = a.Get<xiiSwizzle::XXXX>();
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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetCombined")
  {
    xiiSimdVec4i a(2, 4, 6, 8);
    xiiSimdVec4i b(3, 5, 7, 9);

    xiiSimdVec4i c = a.GetCombined<xiiSwizzle::XXXX>(b);
    XII_TEST_BOOL(c.x() == a.x() && c.y() == a.x() && c.z() == b.x() && c.w() == b.x());

    c = a.GetCombined<xiiSwizzle::YYYX>(b);
    XII_TEST_BOOL(c.x() == a.y() && c.y() == a.y() && c.z() == b.y() && c.w() == b.x());

    c = a.GetCombined<xiiSwizzle::ZZZX>(b);
    XII_TEST_BOOL(c.x() == a.z() && c.y() == a.z() && c.z() == b.z() && c.w() == b.x());

    c = a.GetCombined<xiiSwizzle::WWWX>(b);
    XII_TEST_BOOL(c.x() == a.w() && c.y() == a.w() && c.z() == b.w() && c.w() == b.x());

    c = a.GetCombined<xiiSwizzle::WZYX>(b);
    XII_TEST_BOOL(c.x() == a.w() && c.y() == a.z() && c.z() == b.y() && c.w() == b.x());

    c = a.GetCombined<xiiSwizzle::XYZW>(b);
    XII_TEST_BOOL(c.x() == a.x() && c.y() == a.y() && c.z() == b.z() && c.w() == b.w());

    c = a.GetCombined<xiiSwizzle::WZYX>(b);
    XII_TEST_BOOL(c.x() == a.w() && c.y() == a.z() && c.z() == b.y() && c.w() == b.x());

    c = a.GetCombined<xiiSwizzle::YYYY>(b);
    XII_TEST_BOOL(c.x() == a.y() && c.y() == a.y() && c.z() == b.y() && c.w() == b.y());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Operators")
  {
    {
      xiiSimdVec4i a(-3, 5, -7, 9);

      xiiSimdVec4i b = -a;
      XII_TEST_BOOL(b.x() == 3 && b.y() == -5 && b.z() == 7 && b.w() == -9);

      b.Set(8, 6, 4, 2);
      xiiSimdVec4i c;
      c = a + b;
      XII_TEST_BOOL(c.x() == 5 && c.y() == 11 && c.z() == -3 && c.w() == 11);

      c = a - b;
      XII_TEST_BOOL(c.x() == -11 && c.y() == -1 && c.z() == -11 && c.w() == 7);

      c = a.CompMul(b);
      XII_TEST_BOOL(c.x() == -24 && c.y() == 30 && c.z() == -28 && c.w() == 18);

      c = a.CompDiv(b);
      XII_TEST_BOOL(c.x() == 0 && c.y() == 0 && c.z() == -1 && c.w() == 4);
    }

    {
      xiiSimdVec4i a(XII_BIT(1), XII_BIT(2), XII_BIT(3), XII_BIT(4));
      xiiSimdVec4i b(XII_BIT(4), XII_BIT(3), XII_BIT(3), XII_BIT(5) - 1);
      xiiSimdVec4i c;

      c = a | b;
      XII_TEST_BOOL(c.x() == (XII_BIT(1) | XII_BIT(4)) && c.y() == (XII_BIT(2) | XII_BIT(3)) && c.z() == XII_BIT(3) && c.w() == XII_BIT(5) - 1);

      c = a & b;
      XII_TEST_BOOL(c.x() == 0 && c.y() == 0 && c.z() == XII_BIT(3) && c.w() == XII_BIT(4));

      c = a ^ b;
      XII_TEST_BOOL(c.x() == (XII_BIT(1) | XII_BIT(4)) && c.y() == (XII_BIT(2) | XII_BIT(3)) && c.z() == 0 && c.w() == XII_BIT(4) - 1);

      c = ~a;
      XII_TEST_BOOL(c.x() == ~XII_BIT(1) && c.y() == ~XII_BIT(2) && c.z() == ~XII_BIT(3) && c.w() == ~XII_BIT(4));

      c = a << 3;
      XII_TEST_BOOL(c.x() == XII_BIT(4) && c.y() == XII_BIT(5) && c.z() == XII_BIT(6) && c.w() == XII_BIT(7));

      c = a >> 1;
      XII_TEST_BOOL(c.x() == XII_BIT(0) && c.y() == XII_BIT(1) && c.z() == XII_BIT(2) && c.w() == XII_BIT(3));

      xiiSimdVec4i s(1, 2, 3, 4);
      c = a << s;
      XII_TEST_BOOL(c.x() == XII_BIT(2) && c.y() == XII_BIT(4) && c.z() == XII_BIT(6) && c.w() == XII_BIT(8));

      c = b >> s;
      XII_TEST_BOOL(c.x() == XII_BIT(3) && c.y() == XII_BIT(1) && c.z() == XII_BIT(0) && c.w() == XII_BIT(0));
    }

    {
      xiiSimdVec4i a(-3, 5, -7, 9);
      xiiSimdVec4i b(8, 6, 4, 2);

      xiiSimdVec4i c = a;
      c += b;
      XII_TEST_BOOL(c.x() == 5 && c.y() == 11 && c.z() == -3 && c.w() == 11);

      c = a;
      c -= b;
      XII_TEST_BOOL(c.x() == -11 && c.y() == -1 && c.z() == -11 && c.w() == 7);
    }

    {
      xiiSimdVec4i a(XII_BIT(1), XII_BIT(2), XII_BIT(3), XII_BIT(4));
      xiiSimdVec4i b(XII_BIT(4), XII_BIT(3), XII_BIT(3), XII_BIT(5) - 1);

      xiiSimdVec4i c = a;
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

      c = xiiSimdVec4i(-2, -4, -7, -8);
      c >>= 1;
      XII_TEST_BOOL(c.x() == -1 && c.y() == -2 && c.z() == -4 && c.w() == -4);
    }

    {
      xiiSimdVec4i a(-3, 5, -7, 9);
      xiiSimdVec4i b(8, 6, 4, 2);
      xiiSimdVec4i c;

      c = a.CompMin(b);
      XII_TEST_BOOL(c.x() == -3 && c.y() == 5 && c.z() == -7 && c.w() == 2);

      c = a.CompMax(b);
      XII_TEST_BOOL(c.x() == 8 && c.y() == 6 && c.z() == 4 && c.w() == 9);

      c = a.Abs();
      XII_TEST_BOOL(c.x() == 3 && c.y() == 5 && c.z() == 7 && c.w() == 9);

      xiiSimdVec4b cmp(false, true, false, true);
      c = xiiSimdVec4i::Select(cmp, a, b);
      XII_TEST_BOOL(c.x() == 8 && c.y() == 5 && c.z() == 4 && c.w() == 9);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Comparison")
  {
    xiiSimdVec4i a(-7, 5, 4, 3);
    xiiSimdVec4i b(8, 6, 4, -2);
    xiiSimdVec4b cmp;

    cmp = a == b;
    XII_TEST_BOOL(!cmp.x() && !cmp.y() && cmp.z() && !cmp.w());

    cmp = a != b;
    XII_TEST_BOOL(cmp.x() && cmp.y() && !cmp.z() && cmp.w());

    cmp = a <= b;
    XII_TEST_BOOL(cmp.x() && cmp.y() && cmp.z() && !cmp.w());

    cmp = a < b;
    XII_TEST_BOOL(cmp.x() && cmp.y() && !cmp.z() && !cmp.w());

    cmp = a >= b;
    XII_TEST_BOOL(!cmp.x() && !cmp.y() && cmp.z() && cmp.w());

    cmp = a > b;
    XII_TEST_BOOL(!cmp.x() && !cmp.y() && !cmp.z() && cmp.w());
  }
}
