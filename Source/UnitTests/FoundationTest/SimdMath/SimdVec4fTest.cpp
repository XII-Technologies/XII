/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Vec4.h>
#include <Foundation/SimdMath/SimdVec4f.h>

namespace
{
  static bool AllCompSame(const xiiSimdFloat& a)
  {
    // Make sure all components are the same
    xiiSimdVec4f test;
    test.m_v = a.m_v;
    return test.x() == test.y() && test.x() == test.z() && test.x() == test.w();
  }

  template <xiiMathFloatBits::Enum acc>
  static void TestLength(const xiiSimdVec4f& a, float r[4], const xiiSimdFloat& fEps)
  {
    xiiSimdFloat l1 = a.GetLength<1, acc>();
    xiiSimdFloat l2 = a.GetLength<2, acc>();
    xiiSimdFloat l3 = a.GetLength<3, acc>();
    xiiSimdFloat l4 = a.GetLength<4, acc>();
    XII_TEST_FLOAT(l1, r[0], fEps);
    XII_TEST_FLOAT(l2, r[1], fEps);
    XII_TEST_FLOAT(l3, r[2], fEps);
    XII_TEST_FLOAT(l4, r[3], fEps);
    XII_TEST_BOOL(AllCompSame(l1));
    XII_TEST_BOOL(AllCompSame(l2));
    XII_TEST_BOOL(AllCompSame(l3));
    XII_TEST_BOOL(AllCompSame(l4));
  }

  template <xiiMathFloatBits::Enum acc>
  static void TestInvLength(const xiiSimdVec4f& a, float r[4], const xiiSimdFloat& fEps)
  {
    xiiSimdFloat l1 = a.GetInvLength<1, acc>();
    xiiSimdFloat l2 = a.GetInvLength<2, acc>();
    xiiSimdFloat l3 = a.GetInvLength<3, acc>();
    xiiSimdFloat l4 = a.GetInvLength<4, acc>();
    XII_TEST_FLOAT(l1, r[0], fEps);
    XII_TEST_FLOAT(l2, r[1], fEps);
    XII_TEST_FLOAT(l3, r[2], fEps);
    XII_TEST_FLOAT(l4, r[3], fEps);
    XII_TEST_BOOL(AllCompSame(l1));
    XII_TEST_BOOL(AllCompSame(l2));
    XII_TEST_BOOL(AllCompSame(l3));
    XII_TEST_BOOL(AllCompSame(l4));
  }

  template <xiiMathFloatBits::Enum acc>
  static void TestNormalize(const xiiSimdVec4f& a, xiiSimdVec4f n[4], xiiSimdFloat r[4], const xiiSimdFloat& fEps)
  {
    xiiSimdVec4f n1 = a.GetNormalized<1, acc>();
    xiiSimdVec4f n2 = a.GetNormalized<2, acc>();
    xiiSimdVec4f n3 = a.GetNormalized<3, acc>();
    xiiSimdVec4f n4 = a.GetNormalized<4, acc>();
    XII_TEST_BOOL(n1.IsEqual(n[0], fEps).AllSet());
    XII_TEST_BOOL(n2.IsEqual(n[1], fEps).AllSet());
    XII_TEST_BOOL(n3.IsEqual(n[2], fEps).AllSet());
    XII_TEST_BOOL(n4.IsEqual(n[3], fEps).AllSet());

    xiiSimdVec4f a1 = a;
    xiiSimdVec4f a2 = a;
    xiiSimdVec4f a3 = a;
    xiiSimdVec4f a4 = a;

    xiiSimdFloat l1 = a1.GetLengthAndNormalize<1, acc>();
    xiiSimdFloat l2 = a2.GetLengthAndNormalize<2, acc>();
    xiiSimdFloat l3 = a3.GetLengthAndNormalize<3, acc>();
    xiiSimdFloat l4 = a4.GetLengthAndNormalize<4, acc>();
    XII_TEST_FLOAT(l1, r[0], fEps);
    XII_TEST_FLOAT(l2, r[1], fEps);
    XII_TEST_FLOAT(l3, r[2], fEps);
    XII_TEST_FLOAT(l4, r[3], fEps);
    XII_TEST_BOOL(AllCompSame(l1));
    XII_TEST_BOOL(AllCompSame(l2));
    XII_TEST_BOOL(AllCompSame(l3));
    XII_TEST_BOOL(AllCompSame(l4));

    XII_TEST_BOOL(a1.IsEqual(n[0], fEps).AllSet());
    XII_TEST_BOOL(a2.IsEqual(n[1], fEps).AllSet());
    XII_TEST_BOOL(a3.IsEqual(n[2], fEps).AllSet());
    XII_TEST_BOOL(a4.IsEqual(n[3], fEps).AllSet());

    XII_TEST_BOOL(a1.IsNormalized<1>(fEps));
    XII_TEST_BOOL(a2.IsNormalized<2>(fEps));
    XII_TEST_BOOL(a3.IsNormalized<3>(fEps));
    XII_TEST_BOOL(a4.IsNormalized<4>(fEps));
    XII_TEST_BOOL(!a1.IsNormalized<2>(fEps));
    XII_TEST_BOOL(!a2.IsNormalized<3>(fEps));
    XII_TEST_BOOL(!a3.IsNormalized<4>(fEps));

    a1 = a;
    a1.Normalize<1, acc>();
    a2 = a;
    a2.Normalize<2, acc>();
    a3 = a;
    a3.Normalize<3, acc>();
    a4 = a;
    a4.Normalize<4, acc>();
    XII_TEST_BOOL(a1.IsEqual(n[0], fEps).AllSet());
    XII_TEST_BOOL(a2.IsEqual(n[1], fEps).AllSet());
    XII_TEST_BOOL(a3.IsEqual(n[2], fEps).AllSet());
    XII_TEST_BOOL(a4.IsEqual(n[3], fEps).AllSet());
  }

  template <xiiMathFloatBits::Enum acc>
  static void TestNormalizeIfNotZero(const xiiSimdVec4f& a, xiiSimdVec4f n[4], const xiiSimdFloat& fEps)
  {
    xiiSimdVec4f a1 = a;
    a1.NormalizeIfNotZero<1>(fEps);
    xiiSimdVec4f a2 = a;
    a2.NormalizeIfNotZero<2>(fEps);
    xiiSimdVec4f a3 = a;
    a3.NormalizeIfNotZero<3>(fEps);
    xiiSimdVec4f a4 = a;
    a4.NormalizeIfNotZero<4>(fEps);
    XII_TEST_BOOL(a1.IsEqual(n[0], fEps).AllSet());
    XII_TEST_BOOL(a2.IsEqual(n[1], fEps).AllSet());
    XII_TEST_BOOL(a3.IsEqual(n[2], fEps).AllSet());
    XII_TEST_BOOL(a4.IsEqual(n[3], fEps).AllSet());

    XII_TEST_BOOL(a1.IsNormalized<1>(fEps));
    XII_TEST_BOOL(a2.IsNormalized<2>(fEps));
    XII_TEST_BOOL(a3.IsNormalized<3>(fEps));
    XII_TEST_BOOL(a4.IsNormalized<4>(fEps));
    XII_TEST_BOOL(!a1.IsNormalized<2>(fEps));
    XII_TEST_BOOL(!a2.IsNormalized<3>(fEps));
    XII_TEST_BOOL(!a3.IsNormalized<4>(fEps));

    xiiSimdVec4f b(fEps);
    b.NormalizeIfNotZero<4>(fEps);
    XII_TEST_BOOL(b.IsZero<4>());
  }
} // namespace

XII_CREATE_SIMPLE_TEST(SimdMath, SimdVec4f)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    // In debug the default constructor initializes everything with NaN.
    xiiSimdVec4f vDefCtor;
    XII_TEST_BOOL(vDefCtor.IsNaN<4>());
#else
// GCC assumes that the contents of the memory prior to the placement constructor doesn't matter
// So it optimizes away the initialization.
#  if XII_DISABLED(XII_COMPILER_GCC)
    // Placement new of the default constructor should not have any effect on the previous data.
    alignas(16) float testBlock[4] = {1, 2, 3, 4};
    xiiSimdVec4f*     pDefCtor     = ::new ((void*)&testBlock[0]) xiiSimdVec4f;
    XII_TEST_BOOL(pDefCtor->x() == 1.0f && pDefCtor->y() == 2.0f && pDefCtor->z() == 3.0f && pDefCtor->w() == 4.0f);
#  endif
#endif

    // Make sure the class didn't accidentally change in size.
#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) || (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE)
    static_assert(sizeof(xiiSimdVec4f) == 16);
    static_assert(alignof(xiiSimdVec4f) == 16);
#endif

    xiiSimdVec4f vInit1F(2.0f);
    XII_TEST_BOOL(vInit1F.x() == 2.0f && vInit1F.y() == 2.0f && vInit1F.z() == 2.0f && vInit1F.w() == 2.0f);

    xiiSimdFloat a(3.0f);
    xiiSimdVec4f vInit1SF(a);
    XII_TEST_BOOL(vInit1SF.x() == 3.0f && vInit1SF.y() == 3.0f && vInit1SF.z() == 3.0f && vInit1SF.w() == 3.0f);

    xiiSimdVec4f vInit4F(1.0f, 2.0f, 3.0f, 4.0f);
    XII_TEST_BOOL(vInit4F.x() == 1.0f && vInit4F.y() == 2.0f && vInit4F.z() == 3.0f && vInit4F.w() == 4.0f);

    // Make sure all components have the correct values
#if ((XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) || (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE)) && XII_ENABLED(XII_COMPILER_MSVC)
    XII_TEST_BOOL(vInit4F.m_v.m128_f32[0] == 1.0f && vInit4F.m_v.m128_f32[1] == 2.0f && vInit4F.m_v.m128_f32[2] == 3.0f && vInit4F.m_v.m128_f32[3] == 4.0f);
#endif

    xiiSimdVec4f vCopy(vInit4F);
    XII_TEST_BOOL(vCopy.x() == 1.0f && vCopy.y() == 2.0f && vCopy.z() == 3.0f && vCopy.w() == 4.0f);

    xiiSimdVec4f vZero = xiiSimdVec4f::MakeZero();
    XII_TEST_BOOL(vZero.x() == 0.0f && vZero.y() == 0.0f && vZero.z() == 0.0f && vZero.w() == 0.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Setter")
  {
    xiiSimdVec4f a;
    a.Set(2.0f);
    XII_TEST_BOOL(a.x() == 2.0f && a.y() == 2.0f && a.z() == 2.0f && a.w() == 2.0f);

    xiiSimdVec4f b;
    b.Set(1.0f, 2.0f, 3.0f, 4.0f);
    XII_TEST_BOOL(b.x() == 1.0f && b.y() == 2.0f && b.z() == 3.0f && b.w() == 4.0f);

    b.SetX(5.0f);
    XII_TEST_BOOL(b.x() == 5.0f && b.y() == 2.0f && b.z() == 3.0f && b.w() == 4.0f);

    b.SetY(6.0f);
    XII_TEST_BOOL(b.x() == 5.0f && b.y() == 6.0f && b.z() == 3.0f && b.w() == 4.0f);

    b.SetZ(7.0f);
    XII_TEST_BOOL(b.x() == 5.0f && b.y() == 6.0f && b.z() == 7.0f && b.w() == 4.0f);

    b.SetW(8.0f);
    XII_TEST_BOOL(b.x() == 5.0f && b.y() == 6.0f && b.z() == 7.0f && b.w() == 8.0f);

    xiiSimdVec4f c;
    c.SetZero();
    XII_TEST_BOOL(c.x() == 0.0f && c.y() == 0.0f && c.z() == 0.0f && c.w() == 0.0f);

    {
      xiiSimdVec4f z = xiiSimdVec4f::MakeNaN();
      XII_TEST_BOOL(xiiMath::IsNaN((float)z.x()));
      XII_TEST_BOOL(xiiMath::IsNaN((float)z.y()));
      XII_TEST_BOOL(xiiMath::IsNaN((float)z.z()));
      XII_TEST_BOOL(xiiMath::IsNaN((float)z.w()));
    }

    {
      float        testBlock[4] = {1, 2, 3, 4};
      xiiSimdVec4f x;
      x.Load<1>(testBlock);
      XII_TEST_BOOL(x.x() == 1.0f && x.y() == 0.0f && x.z() == 0.0f && x.w() == 0.0f);

      xiiSimdVec4f xy;
      xy.Load<2>(testBlock);
      XII_TEST_BOOL(xy.x() == 1.0f && xy.y() == 2.0f && xy.z() == 0.0f && xy.w() == 0.0f);

      xiiSimdVec4f xyz;
      xyz.Load<3>(testBlock);
      XII_TEST_BOOL(xyz.x() == 1.0f && xyz.y() == 2.0f && xyz.z() == 3.0f && xyz.w() == 0.0f);

      xiiSimdVec4f xyzw;
      xyzw.Load<4>(testBlock);
      XII_TEST_BOOL(xyzw.x() == 1.0f && xyzw.y() == 2.0f && xyzw.z() == 3.0f && xyzw.w() == 4.0f);

      XII_TEST_BOOL(xyzw.GetComponent(0) == 1.0f);
      XII_TEST_BOOL(xyzw.GetComponent(1) == 2.0f);
      XII_TEST_BOOL(xyzw.GetComponent(2) == 3.0f);
      XII_TEST_BOOL(xyzw.GetComponent(3) == 4.0f);
      XII_TEST_BOOL(xyzw.GetComponent(4) == 4.0f);

      // Make sure all components have the correct values
#if ((XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) || (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE)) && XII_ENABLED(XII_COMPILER_MSVC)
      XII_TEST_BOOL(xyzw.m_v.m128_f32[0] == 1.0f && xyzw.m_v.m128_f32[1] == 2.0f && xyzw.m_v.m128_f32[2] == 3.0f && xyzw.m_v.m128_f32[3] == 4.0f);
#endif
    }

    {
      float testBlock[4] = {7, 7, 7, 7};
      float mem[4]       = {};

      xiiSimdVec4f b2(1, 2, 3, 4);

      memcpy(mem, testBlock, 16);
      b2.Store<1>(mem);
      XII_TEST_BOOL(mem[0] == 1.0f && mem[1] == 7.0f && mem[2] == 7.0f && mem[3] == 7.0f);

      memcpy(mem, testBlock, 16);
      b2.Store<2>(mem);
      XII_TEST_BOOL(mem[0] == 1.0f && mem[1] == 2.0f && mem[2] == 7.0f && mem[3] == 7.0f);

      memcpy(mem, testBlock, 16);
      b2.Store<3>(mem);
      XII_TEST_BOOL(mem[0] == 1.0f && mem[1] == 2.0f && mem[2] == 3.0f && mem[3] == 7.0f);

      memcpy(mem, testBlock, 16);
      b2.Store<4>(mem);
      XII_TEST_BOOL(mem[0] == 1.0f && mem[1] == 2.0f && mem[2] == 3.0f && mem[3] == 4.0f);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Functions")
  {
    {
      xiiSimdVec4f a(1.0f, 2.0f, 4.0f, 8.0f);
      xiiSimdVec4f b(1.0f, 0.5f, 0.25f, 0.125f);

      XII_TEST_BOOL(a.GetReciprocal().IsEqual(b, xiiMath::SmallEpsilon<float>()).AllSet());
      XII_TEST_BOOL(a.GetReciprocal<xiiMathFloatBits::FULL>().IsEqual(b, xiiMath::SmallEpsilon<float>()).AllSet());
      XII_TEST_BOOL(a.GetReciprocal<xiiMathFloatBits::BITS_23>().IsEqual(b, xiiMath::DefaultEpsilon<float>()).AllSet());
      XII_TEST_BOOL(a.GetReciprocal<xiiMathFloatBits::BITS_12>().IsEqual(b, xiiMath::HugeEpsilon<float>()).AllSet());
    }

    {
      xiiSimdVec4f a(1.0f, 2.0f, 4.0f, 8.0f);
      xiiSimdVec4f b(1.0f, xiiMath::Sqrt(2.0f), xiiMath::Sqrt(4.0f), xiiMath::Sqrt(8.0f));

      XII_TEST_BOOL(a.GetSqrt().IsEqual(b, xiiMath::SmallEpsilon<float>()).AllSet());
      XII_TEST_BOOL(a.GetSqrt<xiiMathFloatBits::FULL>().IsEqual(b, xiiMath::SmallEpsilon<float>()).AllSet());
      XII_TEST_BOOL(a.GetSqrt<xiiMathFloatBits::BITS_23>().IsEqual(b, xiiMath::DefaultEpsilon<float>()).AllSet());
      XII_TEST_BOOL(a.GetSqrt<xiiMathFloatBits::BITS_12>().IsEqual(b, xiiMath::HugeEpsilon<float>()).AllSet());
    }

    {
      xiiSimdVec4f a(1.0f, 2.0f, 4.0f, 8.0f);
      xiiSimdVec4f b(1.0f, 1.0f / xiiMath::Sqrt(2.0f), 1.0f / xiiMath::Sqrt(4.0f), 1.0f / xiiMath::Sqrt(8.0f));

      XII_TEST_BOOL(a.GetInvSqrt().IsEqual(b, xiiMath::SmallEpsilon<float>()).AllSet());
      XII_TEST_BOOL(a.GetInvSqrt<xiiMathFloatBits::FULL>().IsEqual(b, xiiMath::SmallEpsilon<float>()).AllSet());
      XII_TEST_BOOL(a.GetInvSqrt<xiiMathFloatBits::BITS_23>().IsEqual(b, xiiMath::DefaultEpsilon<float>()).AllSet());
      XII_TEST_BOOL(a.GetInvSqrt<xiiMathFloatBits::BITS_12>().IsEqual(b, xiiMath::HugeEpsilon<float>()).AllSet());
    }

    {
      xiiSimdVec4f a(2.0f, -2.0f, 4.0f, -8.0f);
      float        r[4];
      r[0] = 2.0f;
      r[1] = xiiVec2(a.x(), a.y()).GetLength();
      r[2] = xiiVec3(a.x(), a.y(), a.z()).GetLength();
      r[3] = xiiVec4(a.x(), a.y(), a.z(), a.w()).GetLength();

      XII_TEST_FLOAT(a.GetLength<1>(), r[0], xiiMath::SmallEpsilon<float>());
      XII_TEST_FLOAT(a.GetLength<2>(), r[1], xiiMath::SmallEpsilon<float>());
      XII_TEST_FLOAT(a.GetLength<3>(), r[2], xiiMath::SmallEpsilon<float>());
      XII_TEST_FLOAT(a.GetLength<4>(), r[3], xiiMath::SmallEpsilon<float>());

      TestLength<xiiMathFloatBits::FULL>(a, r, xiiMath::SmallEpsilon<float>());
      TestLength<xiiMathFloatBits::BITS_23>(a, r, xiiMath::DefaultEpsilon<float>());
      TestLength<xiiMathFloatBits::BITS_12>(a, r, 0.01f);
    }

    {
      xiiSimdVec4f a(2.0f, -2.0f, 4.0f, -8.0f);
      float        r[4];
      r[0] = 0.5f;
      r[1] = 1.0f / xiiVec2(a.x(), a.y()).GetLength();
      r[2] = 1.0f / xiiVec3(a.x(), a.y(), a.z()).GetLength();
      r[3] = 1.0f / xiiVec4(a.x(), a.y(), a.z(), a.w()).GetLength();

      XII_TEST_FLOAT(a.GetInvLength<1>(), r[0], xiiMath::SmallEpsilon<float>());
      XII_TEST_FLOAT(a.GetInvLength<2>(), r[1], xiiMath::SmallEpsilon<float>());
      XII_TEST_FLOAT(a.GetInvLength<3>(), r[2], xiiMath::SmallEpsilon<float>());
      XII_TEST_FLOAT(a.GetInvLength<4>(), r[3], xiiMath::SmallEpsilon<float>());

      TestInvLength<xiiMathFloatBits::FULL>(a, r, xiiMath::SmallEpsilon<float>());
      TestInvLength<xiiMathFloatBits::BITS_23>(a, r, xiiMath::DefaultEpsilon<float>());
      TestInvLength<xiiMathFloatBits::BITS_12>(a, r, xiiMath::HugeEpsilon<float>());
    }

    {
      xiiSimdVec4f a(2.0f, -2.0f, 4.0f, -8.0f);
      float        r[4];
      r[0] = 2.0f * 2.0f;
      r[1] = xiiVec2(a.x(), a.y()).GetLengthSquared();
      r[2] = xiiVec3(a.x(), a.y(), a.z()).GetLengthSquared();
      r[3] = xiiVec4(a.x(), a.y(), a.z(), a.w()).GetLengthSquared();

      XII_TEST_FLOAT(a.GetLengthSquared<1>(), r[0], xiiMath::SmallEpsilon<float>());
      XII_TEST_FLOAT(a.GetLengthSquared<2>(), r[1], xiiMath::SmallEpsilon<float>());
      XII_TEST_FLOAT(a.GetLengthSquared<3>(), r[2], xiiMath::SmallEpsilon<float>());
      XII_TEST_FLOAT(a.GetLengthSquared<4>(), r[3], xiiMath::SmallEpsilon<float>());
    }

    {
      xiiSimdVec4f a(2.0f, -2.0f, 4.0f, -8.0f);
      xiiSimdFloat r[4];
      r[0] = 2.0f;
      r[1] = xiiVec2(a.x(), a.y()).GetLength();
      r[2] = xiiVec3(a.x(), a.y(), a.z()).GetLength();
      r[3] = xiiVec4(a.x(), a.y(), a.z(), a.w()).GetLength();

      xiiSimdVec4f n[4];
      n[0] = a / r[0];
      n[1] = a / r[1];
      n[2] = a / r[2];
      n[3] = a / r[3];

      TestNormalize<xiiMathFloatBits::FULL>(a, n, r, xiiMath::SmallEpsilon<float>());
      TestNormalize<xiiMathFloatBits::BITS_23>(a, n, r, xiiMath::DefaultEpsilon<float>());
      TestNormalize<xiiMathFloatBits::BITS_12>(a, n, r, 0.01f);
    }

    {
      xiiSimdVec4f a(2.0f, -2.0f, 4.0f, -8.0f);
      xiiSimdVec4f n[4];
      n[0] = a / 2.0f;
      n[1] = a / xiiVec2(a.x(), a.y()).GetLength();
      n[2] = a / xiiVec3(a.x(), a.y(), a.z()).GetLength();
      n[3] = a / xiiVec4(a.x(), a.y(), a.z(), a.w()).GetLength();

      TestNormalizeIfNotZero<xiiMathFloatBits::FULL>(a, n, xiiMath::SmallEpsilon<float>());
      TestNormalizeIfNotZero<xiiMathFloatBits::BITS_23>(a, n, xiiMath::DefaultEpsilon<float>());
      TestNormalizeIfNotZero<xiiMathFloatBits::BITS_12>(a, n, xiiMath::HugeEpsilon<float>());
    }

    {
      xiiSimdVec4f a;

      a.Set(0.0f, 2.0f, 0.0f, 0.0f);
      XII_TEST_BOOL(a.IsZero<1>());
      XII_TEST_BOOL(!a.IsZero<2>());

      a.Set(0.0f, 0.0f, 3.0f, 0.0f);
      XII_TEST_BOOL(a.IsZero<2>());
      XII_TEST_BOOL(!a.IsZero<3>());

      a.Set(0.0f, 0.0f, 0.0f, 4.0f);
      XII_TEST_BOOL(a.IsZero<3>());
      XII_TEST_BOOL(!a.IsZero<4>());

      float smallEps = xiiMath::SmallEpsilon<float>();
      a.Set(smallEps, 2.0f, smallEps, smallEps);
      XII_TEST_BOOL(a.IsZero<1>(xiiMath::DefaultEpsilon<float>()));
      XII_TEST_BOOL(!a.IsZero<2>(xiiMath::DefaultEpsilon<float>()));

      a.Set(smallEps, smallEps, 3.0f, smallEps);
      XII_TEST_BOOL(a.IsZero<2>(xiiMath::DefaultEpsilon<float>()));
      XII_TEST_BOOL(!a.IsZero<3>(xiiMath::DefaultEpsilon<float>()));

      a.Set(smallEps, smallEps, smallEps, 4.0f);
      XII_TEST_BOOL(a.IsZero<3>(xiiMath::DefaultEpsilon<float>()));
      XII_TEST_BOOL(!a.IsZero<4>(xiiMath::DefaultEpsilon<float>()));
    }

    {
      xiiSimdVec4f a;

      float NaN = xiiMath::NaN<float>();
      float Inf = xiiMath::Infinity<float>();

      a.Set(NaN, 1.0f, NaN, NaN);
      XII_TEST_BOOL(a.IsNaN<1>());
      XII_TEST_BOOL(a.IsNaN<2>());
      XII_TEST_BOOL(!a.IsValid<2>());

      a.Set(Inf, 1.0f, NaN, NaN);
      XII_TEST_BOOL(!a.IsNaN<1>());
      XII_TEST_BOOL(!a.IsNaN<2>());
      XII_TEST_BOOL(!a.IsValid<2>());

      a.Set(1.0f, 2.0f, Inf, NaN);
      XII_TEST_BOOL(a.IsNaN<4>());
      XII_TEST_BOOL(!a.IsNaN<3>());
      XII_TEST_BOOL(a.IsValid<2>());
      XII_TEST_BOOL(!a.IsValid<3>());

      a.Set(-1.0f, -2.0f, -3.0f, -4.0f);
      XII_TEST_BOOL(a.IsValid<1>());
      XII_TEST_BOOL(a.IsValid<2>());
      XII_TEST_BOOL(a.IsValid<3>());
      XII_TEST_BOOL(a.IsValid<4>());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Swizzle")
  {
    xiiSimdVec4f a(3.0f, 5.0f, 7.0f, 9.0f);

    xiiSimdVec4f b = a.Get<xiiSwizzle::XXXX>();
    XII_TEST_BOOL(b.x() == 3.0f && b.y() == 3.0f && b.z() == 3.0f && b.w() == 3.0f);

    b = a.Get<xiiSwizzle::YYYX>();
    XII_TEST_BOOL(b.x() == 5.0f && b.y() == 5.0f && b.z() == 5.0f && b.w() == 3.0f);

    b = a.Get<xiiSwizzle::ZZZX>();
    XII_TEST_BOOL(b.x() == 7.0f && b.y() == 7.0f && b.z() == 7.0f && b.w() == 3.0f);

    b = a.Get<xiiSwizzle::WWWX>();
    XII_TEST_BOOL(b.x() == 9.0f && b.y() == 9.0f && b.z() == 9.0f && b.w() == 3.0f);

    b = a.Get<xiiSwizzle::WZYX>();
    XII_TEST_BOOL(b.x() == 9.0f && b.y() == 7.0f && b.z() == 5.0f && b.w() == 3.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetCombined")
  {
    xiiSimdVec4f a(2.0f, 4.0f, 6.0f, 8.0f);
    xiiSimdVec4f b(3.0f, 5.0f, 7.0f, 9.0f);

    xiiSimdVec4f c = a.GetCombined<xiiSwizzle::XXXX>(b);
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
      xiiSimdVec4f a(-3.0f, 5.0f, -7.0f, 9.0f);

      xiiSimdVec4f b = -a;
      XII_TEST_BOOL(b.x() == 3.0f && b.y() == -5.0f && b.z() == 7.0f && b.w() == -9.0f);

      b.Set(8.0f, 6.0f, 4.0f, 2.0f);
      xiiSimdVec4f c;
      c = a + b;
      XII_TEST_BOOL(c.x() == 5.0f && c.y() == 11.0f && c.z() == -3.0f && c.w() == 11.0f);

      c = a - b;
      XII_TEST_BOOL(c.x() == -11.0f && c.y() == -1.0f && c.z() == -11.0f && c.w() == 7.0f);

      c = a * xiiSimdFloat(3.0f);
      XII_TEST_BOOL(c.x() == -9.0f && c.y() == 15.0f && c.z() == -21.0f && c.w() == 27.0f);

      c = a / xiiSimdFloat(2.0f);
      XII_TEST_BOOL(c.x() == -1.5f && c.y() == 2.5f && c.z() == -3.5f && c.w() == 4.5f);

      c = a.CompMul(b);
      XII_TEST_BOOL(c.x() == -24.0f && c.y() == 30.0f && c.z() == -28.0f && c.w() == 18.0f);

      xiiSimdVec4f divRes(-0.375f, 5.0f / 6.0f, -1.75f, 4.5f);
      xiiSimdVec4f d1 = a.CompDiv(b);
      xiiSimdVec4f d2 = a.CompDiv<xiiMathFloatBits::FULL>(b);
      xiiSimdVec4f d3 = a.CompDiv<xiiMathFloatBits::BITS_23>(b);
      xiiSimdVec4f d4 = a.CompDiv<xiiMathFloatBits::BITS_12>(b);

      XII_TEST_BOOL(d1.IsEqual(divRes, xiiMath::SmallEpsilon<float>()).AllSet());
      XII_TEST_BOOL(d2.IsEqual(divRes, xiiMath::SmallEpsilon<float>()).AllSet());
      XII_TEST_BOOL(d3.IsEqual(divRes, xiiMath::DefaultEpsilon<float>()).AllSet());
      XII_TEST_BOOL(d4.IsEqual(divRes, 0.01f).AllSet());
    }

    {
      xiiSimdVec4f a(-3.4f, 5.4f, -7.6f, 9.6f);
      xiiSimdVec4f b(8.0f, 6.0f, 4.0f, 2.0f);
      xiiSimdVec4f c;

      c = a.CompMin(b);
      XII_TEST_BOOL(c.x() == -3.4f && c.y() == 5.4f && c.z() == -7.6f && c.w() == 2.0f);

      c = a.CompMax(b);
      XII_TEST_BOOL(c.x() == 8.0f && c.y() == 6.0f && c.z() == 4.0f && c.w() == 9.6f);

      c = a.Abs();
      XII_TEST_BOOL(c.x() == 3.4f && c.y() == 5.4f && c.z() == 7.6f && c.w() == 9.6f);

      c = a.Round();
      XII_TEST_BOOL(c.x() == -3.0f && c.y() == 5.0f && c.z() == -8.0f && c.w() == 10.0f);

      c = a.Floor();
      XII_TEST_BOOL(c.x() == -4.0f && c.y() == 5.0f && c.z() == -8.0f && c.w() == 9.0f);

      c = a.Ceil();
      XII_TEST_BOOL(c.x() == -3.0f && c.y() == 6.0f && c.z() == -7.0f && c.w() == 10.0f);

      c = a.Trunc();
      XII_TEST_BOOL(c.x() == -3.0f && c.y() == 5.0f && c.z() == -7.0f && c.w() == 9.0f);

      c = a.Fraction();
      XII_TEST_BOOL(c.IsEqual(xiiSimdVec4f(-0.4f, 0.4f, -0.6f, 0.6f), xiiMath::SmallEpsilon<float>()).AllSet());
    }

    {
      xiiSimdVec4f a(-3.0f, 5.0f, -7.0f, 9.0f);
      xiiSimdVec4f b(8.0f, 6.0f, 4.0f, 2.0f);

      xiiSimdVec4b cmp(true, false, false, true);
      xiiSimdVec4f c;

      c = a.FlipSign(cmp);
      XII_TEST_BOOL(c.x() == 3.0f && c.y() == 5.0f && c.z() == -7.0f && c.w() == -9.0f);

      c = xiiSimdVec4f::Select(cmp, b, a);
      XII_TEST_BOOL(c.x() == 8.0f && c.y() == 5.0f && c.z() == -7.0f && c.w() == 2.0f);

      c = xiiSimdVec4f::Select(cmp, a, b);
      XII_TEST_BOOL(c.x() == -3.0f && c.y() == 6.0f && c.z() == 4.0f && c.w() == 9.0f);
    }

    {
      xiiSimdVec4f a(-3.0f, 5.0f, -7.0f, 9.0f);
      xiiSimdVec4f b(8.0f, 6.0f, 4.0f, 2.0f);

      xiiSimdVec4f c = a;
      c += b;
      XII_TEST_BOOL(c.x() == 5.0f && c.y() == 11.0f && c.z() == -3.0f && c.w() == 11.0f);

      c = a;
      c -= b;
      XII_TEST_BOOL(c.x() == -11.0f && c.y() == -1.0f && c.z() == -11.0f && c.w() == 7.0f);

      c = a;
      c *= xiiSimdFloat(3.0f);
      XII_TEST_BOOL(c.x() == -9.0f && c.y() == 15.0f && c.z() == -21.0f && c.w() == 27.0f);

      c = a;
      c /= xiiSimdFloat(2.0f);
      XII_TEST_BOOL(c.x() == -1.5f && c.y() == 2.5f && c.z() == -3.5f && c.w() == 4.5f);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Comparison")
  {
    xiiSimdVec4f a(7.0f, 5.0f, 4.0f, 3.0f);
    xiiSimdVec4f b(8.0f, 6.0f, 4.0f, 2.0f);
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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Advanced Operators")
  {
    {
      xiiSimdVec4f a(-3.0f, 5.0f, -7.0f, 9.0f);

      XII_TEST_FLOAT(a.HorizontalSum<1>(), -3.0f, 0.0f);
      XII_TEST_FLOAT(a.HorizontalSum<2>(), 2.0f, 0.0f);
      XII_TEST_FLOAT(a.HorizontalSum<3>(), -5.0f, 0.0f);
      XII_TEST_FLOAT(a.HorizontalSum<4>(), 4.0f, 0.0f);
      XII_TEST_BOOL(AllCompSame(a.HorizontalSum<1>()));
      XII_TEST_BOOL(AllCompSame(a.HorizontalSum<2>()));
      XII_TEST_BOOL(AllCompSame(a.HorizontalSum<3>()));
      XII_TEST_BOOL(AllCompSame(a.HorizontalSum<4>()));

      XII_TEST_FLOAT(a.HorizontalMin<1>(), -3.0f, 0.0f);
      XII_TEST_FLOAT(a.HorizontalMin<2>(), -3.0f, 0.0f);
      XII_TEST_FLOAT(a.HorizontalMin<3>(), -7.0f, 0.0f);
      XII_TEST_FLOAT(a.HorizontalMin<4>(), -7.0f, 0.0f);
      XII_TEST_BOOL(AllCompSame(a.HorizontalMin<1>()));
      XII_TEST_BOOL(AllCompSame(a.HorizontalMin<2>()));
      XII_TEST_BOOL(AllCompSame(a.HorizontalMin<3>()));
      XII_TEST_BOOL(AllCompSame(a.HorizontalMin<4>()));

      XII_TEST_FLOAT(a.HorizontalMax<1>(), -3.0f, 0.0f);
      XII_TEST_FLOAT(a.HorizontalMax<2>(), 5.0f, 0.0f);
      XII_TEST_FLOAT(a.HorizontalMax<3>(), 5.0f, 0.0f);
      XII_TEST_FLOAT(a.HorizontalMax<4>(), 9.0f, 0.0f);
      XII_TEST_BOOL(AllCompSame(a.HorizontalMax<1>()));
      XII_TEST_BOOL(AllCompSame(a.HorizontalMax<2>()));
      XII_TEST_BOOL(AllCompSame(a.HorizontalMax<3>()));
      XII_TEST_BOOL(AllCompSame(a.HorizontalMax<4>()));
    }

    {
      xiiSimdVec4f a(-3.0f, 5.0f, -7.0f, 9.0f);
      xiiSimdVec4f b(8.0f, 6.0f, 4.0f, 2.0f);

      XII_TEST_FLOAT(a.Dot<1>(b), -24.0f, 0.0f);
      XII_TEST_FLOAT(a.Dot<2>(b), 6.0f, 0.0f);
      XII_TEST_FLOAT(a.Dot<3>(b), -22.0f, 0.0f);
      XII_TEST_FLOAT(a.Dot<4>(b), -4.0f, 0.0f);
      XII_TEST_BOOL(AllCompSame(a.Dot<1>(b)));
      XII_TEST_BOOL(AllCompSame(a.Dot<2>(b)));
      XII_TEST_BOOL(AllCompSame(a.Dot<3>(b)));
      XII_TEST_BOOL(AllCompSame(a.Dot<4>(b)));
    }

    {
      xiiSimdVec4f a(1.0f, 2.0f, 3.0f, 0.0f);
      xiiSimdVec4f b(2.0f, -4.0f, 6.0f, 8.0f);

      xiiVec3 res = xiiVec3(a.x(), a.y(), a.z()).CrossRH(xiiVec3(b.x(), b.y(), b.z()));

      xiiSimdVec4f c = a.CrossRH(b);
      XII_TEST_BOOL(c.x() == res.x);
      XII_TEST_BOOL(c.y() == res.y);
      XII_TEST_BOOL(c.z() == res.z);
    }

    {
      xiiSimdVec4f a(1.0f, 2.0f, 3.0f, 0.0f);
      xiiSimdVec4f b(2.0f, -4.0f, 6.0f, 0.0f);

      xiiVec3 res = xiiVec3(a.x(), a.y(), a.z()).CrossRH(xiiVec3(b.x(), b.y(), b.z()));

      xiiSimdVec4f c = a.CrossRH(b);
      XII_TEST_BOOL(c.x() == res.x);
      XII_TEST_BOOL(c.y() == res.y);
      XII_TEST_BOOL(c.z() == res.z);
    }

    {
      xiiSimdVec4f a(-3.0f, 5.0f, -7.0f, 0.0f);
      xiiSimdVec4f b = a.GetOrthogonalVector();

      XII_TEST_BOOL(!b.IsZero<3>());
      XII_TEST_FLOAT(a.Dot<3>(b), 0.0f, 0.0f);
    }

    {
      xiiSimdVec4f a(-3.0f, 5.0f, -7.0f, 9.0f);
      xiiSimdVec4f b(8.0f, 6.0f, 4.0f, 2.0f);
      xiiSimdVec4f c(1.0f, 2.0f, 3.0f, 4.0f);
      xiiSimdVec4f d;

      d = xiiSimdVec4f::MulAdd(a, b, c);
      XII_TEST_BOOL(d.x() == -23.0f && d.y() == 32.0f && d.z() == -25.0f && d.w() == 22.0f);

      d = xiiSimdVec4f::MulAdd(a, xiiSimdFloat(3.0f), c);
      XII_TEST_BOOL(d.x() == -8.0f && d.y() == 17.0f && d.z() == -18.0f && d.w() == 31.0f);

      d = xiiSimdVec4f::MulSub(a, b, c);
      XII_TEST_BOOL(d.x() == -25.0f && d.y() == 28.0f && d.z() == -31.0f && d.w() == 14.0f);

      d = xiiSimdVec4f::MulSub(a, xiiSimdFloat(3.0f), c);
      XII_TEST_BOOL(d.x() == -10.0f && d.y() == 13.0f && d.z() == -24.0f && d.w() == 23.0f);

      d = xiiSimdVec4f::CopySign(b, a);
      XII_TEST_BOOL(d.x() == -8.0f && d.y() == 6.0f && d.z() == -4.0f && d.w() == 2.0f);
    }
  }
}
