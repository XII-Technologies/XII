#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Vec4.h>
#include <Foundation/SimdMath/SimdVec4d.h>

namespace
{
  static bool AllCompSame(const xiiSimdDouble& a)
  {
    // Make sure all components are the same
    xiiSimdVec4d test;
    test.m_v = a.m_v;
    return test.x() == test.y() && test.x() == test.z() && test.x() == test.w();
  }

  template <xiiMathDoubleBits::Enum acc>
  static void TestLength(const xiiSimdVec4d& a, double r[4], const xiiSimdDouble& fEps)
  {
    xiiSimdDouble l1 = a.GetLength<1, acc>();
    xiiSimdDouble l2 = a.GetLength<2, acc>();
    xiiSimdDouble l3 = a.GetLength<3, acc>();
    xiiSimdDouble l4 = a.GetLength<4, acc>();
    XII_TEST_DOUBLE(l1, r[0], fEps);
    XII_TEST_DOUBLE(l2, r[1], fEps);
    XII_TEST_DOUBLE(l3, r[2], fEps);
    XII_TEST_DOUBLE(l4, r[3], fEps);
    XII_TEST_BOOL(AllCompSame(l1));
    XII_TEST_BOOL(AllCompSame(l2));
    XII_TEST_BOOL(AllCompSame(l3));
    XII_TEST_BOOL(AllCompSame(l4));
  }

  template <xiiMathDoubleBits::Enum acc>
  static void TestInvLength(const xiiSimdVec4d& a, double r[4], const xiiSimdDouble& fEps)
  {
    xiiSimdDouble l1 = a.GetInvLength<1, acc>();
    xiiSimdDouble l2 = a.GetInvLength<2, acc>();
    xiiSimdDouble l3 = a.GetInvLength<3, acc>();
    xiiSimdDouble l4 = a.GetInvLength<4, acc>();
    XII_TEST_DOUBLE(l1, r[0], fEps);
    XII_TEST_DOUBLE(l2, r[1], fEps);
    XII_TEST_DOUBLE(l3, r[2], fEps);
    XII_TEST_DOUBLE(l4, r[3], fEps);
    XII_TEST_BOOL(AllCompSame(l1));
    XII_TEST_BOOL(AllCompSame(l2));
    XII_TEST_BOOL(AllCompSame(l3));
    XII_TEST_BOOL(AllCompSame(l4));
  }

  template <xiiMathDoubleBits::Enum acc>
  static void TestNormalize(const xiiSimdVec4d& a, xiiSimdVec4d n[4], xiiSimdDouble r[4], const xiiSimdDouble& fEps)
  {
    xiiSimdVec4d n1 = a.GetNormalized<1, acc>();
    xiiSimdVec4d n2 = a.GetNormalized<2, acc>();
    xiiSimdVec4d n3 = a.GetNormalized<3, acc>();
    xiiSimdVec4d n4 = a.GetNormalized<4, acc>();
    XII_TEST_BOOL(n1.IsEqual(n[0], fEps).AllSet());
    XII_TEST_BOOL(n2.IsEqual(n[1], fEps).AllSet());
    XII_TEST_BOOL(n3.IsEqual(n[2], fEps).AllSet());
    XII_TEST_BOOL(n4.IsEqual(n[3], fEps).AllSet());

    xiiSimdVec4d a1 = a;
    xiiSimdVec4d a2 = a;
    xiiSimdVec4d a3 = a;
    xiiSimdVec4d a4 = a;

    xiiSimdDouble l1 = a1.GetLengthAndNormalize<1, acc>();
    xiiSimdDouble l2 = a2.GetLengthAndNormalize<2, acc>();
    xiiSimdDouble l3 = a3.GetLengthAndNormalize<3, acc>();
    xiiSimdDouble l4 = a4.GetLengthAndNormalize<4, acc>();
    XII_TEST_DOUBLE(l1, r[0], fEps);
    XII_TEST_DOUBLE(l2, r[1], fEps);
    XII_TEST_DOUBLE(l3, r[2], fEps);
    XII_TEST_DOUBLE(l4, r[3], fEps);
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

  template <xiiMathDoubleBits::Enum acc>
  static void TestNormalizeIfNotZero(const xiiSimdVec4d& a, xiiSimdVec4d n[4], const xiiSimdDouble& fEps)
  {
    xiiSimdVec4d a1 = a;
    a1.NormalizeIfNotZero<1>(fEps);
    xiiSimdVec4d a2 = a;
    a2.NormalizeIfNotZero<2>(fEps);
    xiiSimdVec4d a3 = a;
    a3.NormalizeIfNotZero<3>(fEps);
    xiiSimdVec4d a4 = a;
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

    xiiSimdVec4d b(fEps);
    b.NormalizeIfNotZero<4>(fEps);
    XII_TEST_BOOL(b.IsZero<4>());
  }
} // namespace

XII_CREATE_SIMPLE_TEST(SimdMath, SimdVec4d)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    // In debug the default constructor initializes everything with NaN.
    xiiSimdVec4d vDefCtor;
    XII_TEST_BOOL(vDefCtor.IsNaN<4>());
#else
// GCC assumes that the contents of the memory prior to the placement constructor doesn't matter
// So it optimizes away the initialization.
#  if XII_DISABLED(XII_COMPILER_GCC)
    // Placement new of the default constructor should not have any effect on the previous data.
    alignas(32) double testBlock[4] = {1, 2, 3, 4};
    xiiSimdVec4d*      pDefCtor     = ::new ((void*)&testBlock[0]) xiiSimdVec4d;
    XII_TEST_BOOL(pDefCtor->x() == 1.0 && pDefCtor->y() == 2.0 && pDefCtor->z() == 3.0 && pDefCtor->w() == 4.0);
#  endif
#endif

    // Make sure the class didn't accidentally change in size.
#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX)
    XII_CHECK_AT_COMPILETIME(sizeof(xiiSimdVec4d) == 32);
    XII_CHECK_AT_COMPILETIME(XII_ALIGNMENT_OF(xiiSimdVec4d) == 32);
#endif

    xiiSimdVec4d vInit1F(2.0);
    XII_TEST_BOOL(vInit1F.x() == 2.0 && vInit1F.y() == 2.0 && vInit1F.z() == 2.0 && vInit1F.w() == 2.0);

    xiiSimdDouble a(3.0);
    xiiSimdVec4d  vInit1SF(a);
    XII_TEST_BOOL(vInit1SF.x() == 3.0 && vInit1SF.y() == 3.0 && vInit1SF.z() == 3.0 && vInit1SF.w() == 3.0);

    xiiSimdVec4d vInit4F(1.0, 2.0, 3.0, 4.0);
    XII_TEST_BOOL(vInit4F.x() == 1.0 && vInit4F.y() == 2.0 && vInit4F.z() == 3.0 && vInit4F.w() == 4.0);

    // Make sure all components have the correct values
#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) && XII_ENABLED(XII_COMPILER_MSVC)
    XII_TEST_BOOL(vInit4F.m_v.m256d_f64[0] == 1.0 && vInit4F.m_v.m256d_f64[1] == 2.0 && vInit4F.m_v.m256d_f64[2] == 3.0 && vInit4F.m_v.m256d_f64[3] == 4.0);
#endif

    xiiSimdVec4d vCopy(vInit4F);
    XII_TEST_BOOL(vCopy.x() == 1.0 && vCopy.y() == 2.0 && vCopy.z() == 3.0 && vCopy.w() == 4.0);

    xiiSimdVec4d vZero = xiiSimdVec4d::MakeZero();
    XII_TEST_BOOL(vZero.x() == 0.0 && vZero.y() == 0.0 && vZero.z() == 0.0 && vZero.w() == 0.0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Setter")
  {
    xiiSimdVec4d a;
    a.Set(2.0);
    XII_TEST_BOOL(a.x() == 2.0 && a.y() == 2.0 && a.z() == 2.0 && a.w() == 2.0);

    xiiSimdVec4d b;
    b.Set(1.0, 2.0, 3.0, 4.0);
    XII_TEST_BOOL(b.x() == 1.0 && b.y() == 2.0 && b.z() == 3.0 && b.w() == 4.0);

    b.SetX(5.0);
    XII_TEST_BOOL(b.x() == 5.0 && b.y() == 2.0 && b.z() == 3.0 && b.w() == 4.0);

    b.SetY(6.0);
    XII_TEST_BOOL(b.x() == 5.0 && b.y() == 6.0 && b.z() == 3.0 && b.w() == 4.0);

    b.SetZ(7.0);
    XII_TEST_BOOL(b.x() == 5.0 && b.y() == 6.0 && b.z() == 7.0 && b.w() == 4.0);

    b.SetW(8.0);
    XII_TEST_BOOL(b.x() == 5.0 && b.y() == 6.0 && b.z() == 7.0 && b.w() == 8.0);

    xiiSimdVec4d c;
    c.SetZero();
    XII_TEST_BOOL(c.x() == 0.0 && c.y() == 0.0 && c.z() == 0.0 && c.w() == 0.0);

    {
      xiiSimdVec4d z = xiiSimdVec4d::MakeNaN();
      XII_TEST_BOOL(xiiMath::IsNaN((double)z.x()));
      XII_TEST_BOOL(xiiMath::IsNaN((double)z.y()));
      XII_TEST_BOOL(xiiMath::IsNaN((double)z.z()));
      XII_TEST_BOOL(xiiMath::IsNaN((double)z.w()));
    }

    {
      double       testBlock[4] = {1, 2, 3, 4};
      xiiSimdVec4d x;
      x.Load<1>(testBlock);
      XII_TEST_BOOL(x.x() == 1.0 && x.y() == 0.0 && x.z() == 0.0 && x.w() == 0.0);

      xiiSimdVec4d xy;
      xy.Load<2>(testBlock);
      XII_TEST_BOOL(xy.x() == 1.0 && xy.y() == 2.0 && xy.z() == 0.0 && xy.w() == 0.0);

      xiiSimdVec4d xyz;
      xyz.Load<3>(testBlock);
      XII_TEST_BOOL(xyz.x() == 1.0 && xyz.y() == 2.0 && xyz.z() == 3.0 && xyz.w() == 0.0);

      xiiSimdVec4d xyzw;
      xyzw.Load<4>(testBlock);
      XII_TEST_BOOL(xyzw.x() == 1.0 && xyzw.y() == 2.0 && xyzw.z() == 3.0 && xyzw.w() == 4.0);

      XII_TEST_BOOL(xyzw.GetComponent(0) == 1.0);
      XII_TEST_BOOL(xyzw.GetComponent(1) == 2.0);
      XII_TEST_BOOL(xyzw.GetComponent(2) == 3.0);
      XII_TEST_BOOL(xyzw.GetComponent(3) == 4.0);
      XII_TEST_BOOL(xyzw.GetComponent(4) == 4.0);

      // Make sure all components have the correct values
#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) && XII_ENABLED(XII_COMPILER_MSVC)
      XII_TEST_BOOL(xyzw.m_v.m256d_f64[0] == 1.0 && xyzw.m_v.m256d_f64[1] == 2.0 && xyzw.m_v.m256d_f64[2] == 3.0 && xyzw.m_v.m256d_f64[3] == 4.0);
#endif
    }

    {
      double testBlock[4] = {7, 7, 7, 7};
      double mem[4]       = {};

      xiiSimdVec4d b2(1, 2, 3, 4);

      memcpy(mem, testBlock, 32);
      b2.Store<1>(mem);
      XII_TEST_BOOL(mem[0] == 1.0 && mem[1] == 7.0 && mem[2] == 7.0 && mem[3] == 7.0);

      memcpy(mem, testBlock, 32);
      b2.Store<2>(mem);
      XII_TEST_BOOL(mem[0] == 1.0 && mem[1] == 2.0 && mem[2] == 7.0 && mem[3] == 7.0);

      memcpy(mem, testBlock, 32);
      b2.Store<3>(mem);
      XII_TEST_BOOL(mem[0] == 1.0 && mem[1] == 2.0 && mem[2] == 3.0 && mem[3] == 7.0);

      memcpy(mem, testBlock, 32);
      b2.Store<4>(mem);
      XII_TEST_BOOL(mem[0] == 1.0 && mem[1] == 2.0 && mem[2] == 3.0 && mem[3] == 4.0);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Functions")
  {
    {
      xiiSimdVec4d a(1.0, 2.0, 4.0, 8.0);
      xiiSimdVec4d b(1.0, 0.5, 0.25, 0.125);

      XII_TEST_BOOL(a.GetReciprocal().IsEqual(b, xiiMath::SmallEpsilon<double>()).AllSet());
      XII_TEST_BOOL(a.GetReciprocal<xiiMathDoubleBits::FULL>().IsEqual(b, xiiMath::SmallEpsilon<double>()).AllSet());
#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) && (XII_SSE_LEVEL >= XII_AVX_512)
      XII_TEST_BOOL(a.GetReciprocal<xiiMathDoubleBits::BITS_27>().IsEqual(b, xiiMath::DefaultEpsilon<double>()).AllSet());
      XII_TEST_BOOL(a.GetReciprocal<xiiMathDoubleBits::BITS_14>().IsEqual(b, xiiMath::HugeEpsilon<double>()).AllSet());
#endif
    }

    {
      xiiSimdVec4d a(1.0, 2.0, 4.0, 8.0);
      xiiSimdVec4d b(1.0, xiiMath::Sqrt(2.0), xiiMath::Sqrt(4.0), xiiMath::Sqrt(8.0));

      XII_TEST_BOOL(a.GetSqrt().IsEqual(b, xiiMath::SmallEpsilon<double>()).AllSet());
      XII_TEST_BOOL(a.GetSqrt<xiiMathDoubleBits::FULL>().IsEqual(b, xiiMath::SmallEpsilon<double>()).AllSet());
#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) && (XII_SSE_LEVEL >= XII_AVX_512)
      XII_TEST_BOOL(a.GetSqrt<xiiMathDoubleBits::BITS_27>().IsEqual(b, xiiMath::DefaultEpsilon<double>()).AllSet());
      XII_TEST_BOOL(a.GetSqrt<xiiMathDoubleBits::BITS_14>().IsEqual(b, xiiMath::HugeEpsilon<double>()).AllSet());
#endif
    }

    {
      xiiSimdVec4d a(1.0, 2.0, 4.0, 8.0);
      xiiSimdVec4d b(1.0, 1.0 / xiiMath::Sqrt(2.0), 1.0 / xiiMath::Sqrt(4.0), 1.0 / xiiMath::Sqrt(8.0));

      XII_TEST_BOOL(a.GetInvSqrt().IsEqual(b, xiiMath::SmallEpsilon<double>()).AllSet());
      XII_TEST_BOOL(a.GetInvSqrt<xiiMathDoubleBits::FULL>().IsEqual(b, xiiMath::SmallEpsilon<double>()).AllSet());
#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) && (XII_SSE_LEVEL >= XII_AVX_512)
      XII_TEST_BOOL(a.GetInvSqrt<xiiMathDoubleBits::BITS_27>().IsEqual(b, xiiMath::DefaultEpsilon<double>()).AllSet());
      XII_TEST_BOOL(a.GetInvSqrt<xiiMathDoubleBits::BITS_14>().IsEqual(b, xiiMath::HugeEpsilon<double>()).AllSet());
#endif
    }

    {
      xiiSimdVec4d a(2.0, -2.0, 4.0, -8.0);
      double       r[4];
      r[0] = 2.0;
      r[1] = xiiVec2d(a.x(), a.y()).GetLength();
      r[2] = xiiVec3d(a.x(), a.y(), a.z()).GetLength();
      r[3] = xiiVec4d(a.x(), a.y(), a.z(), a.w()).GetLength();

      XII_TEST_DOUBLE(a.GetLength<1>(), r[0], xiiMath::SmallEpsilon<double>());
      XII_TEST_DOUBLE(a.GetLength<2>(), r[1], xiiMath::SmallEpsilon<double>());
      XII_TEST_DOUBLE(a.GetLength<3>(), r[2], xiiMath::SmallEpsilon<double>());
      XII_TEST_DOUBLE(a.GetLength<4>(), r[3], xiiMath::SmallEpsilon<double>());

      TestLength<xiiMathDoubleBits::FULL>(a, r, xiiMath::SmallEpsilon<double>());
#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) && (XII_SSE_LEVEL >= XII_AVX_512)
      TestLength<xiiMathDoubleBits::BITS_27>(a, r, xiiMath::DefaultEpsilon<double>());
      TestLength<xiiMathDoubleBits::BITS_14>(a, r, 0.01f);
#endif
    }

    {
      xiiSimdVec4d a(2.0, -2.0, 4.0, -8.0);
      double       r[4];
      r[0] = 0.5f;
      r[1] = 1.0 / xiiVec2d(a.x(), a.y()).GetLength();
      r[2] = 1.0 / xiiVec3d(a.x(), a.y(), a.z()).GetLength();
      r[3] = 1.0 / xiiVec4d(a.x(), a.y(), a.z(), a.w()).GetLength();

      XII_TEST_DOUBLE(a.GetInvLength<1>(), r[0], xiiMath::SmallEpsilon<double>());
      XII_TEST_DOUBLE(a.GetInvLength<2>(), r[1], xiiMath::SmallEpsilon<double>());
      XII_TEST_DOUBLE(a.GetInvLength<3>(), r[2], xiiMath::SmallEpsilon<double>());
      XII_TEST_DOUBLE(a.GetInvLength<4>(), r[3], xiiMath::SmallEpsilon<double>());

      TestInvLength<xiiMathDoubleBits::FULL>(a, r, xiiMath::SmallEpsilon<double>());
#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) && (XII_SSE_LEVEL >= XII_AVX_512)
      TestInvLength<xiiMathDoubleBits::BITS_27>(a, r, xiiMath::DefaultEpsilon<double>());
      TestInvLength<xiiMathDoubleBits::BITS_14>(a, r, xiiMath::HugeEpsilon<double>());
#endif
    }

    {
      xiiSimdVec4d a(2.0, -2.0, 4.0, -8.0);
      double       r[4];
      r[0] = 2.0 * 2.0;
      r[1] = xiiVec2d(a.x(), a.y()).GetLengthSquared();
      r[2] = xiiVec3d(a.x(), a.y(), a.z()).GetLengthSquared();
      r[3] = xiiVec4d(a.x(), a.y(), a.z(), a.w()).GetLengthSquared();

      XII_TEST_DOUBLE(a.GetLengthSquared<1>(), r[0], xiiMath::SmallEpsilon<double>());
      XII_TEST_DOUBLE(a.GetLengthSquared<2>(), r[1], xiiMath::SmallEpsilon<double>());
      XII_TEST_DOUBLE(a.GetLengthSquared<3>(), r[2], xiiMath::SmallEpsilon<double>());
      XII_TEST_DOUBLE(a.GetLengthSquared<4>(), r[3], xiiMath::SmallEpsilon<double>());
    }

    {
      xiiSimdVec4d  a(2.0, -2.0, 4.0, -8.0);
      xiiSimdDouble r[4];
      r[0] = 2.0;
      r[1] = xiiVec2d(a.x(), a.y()).GetLength();
      r[2] = xiiVec3d(a.x(), a.y(), a.z()).GetLength();
      r[3] = xiiVec4d(a.x(), a.y(), a.z(), a.w()).GetLength();

      xiiSimdVec4d n[4];
      n[0] = a / r[0];
      n[1] = a / r[1];
      n[2] = a / r[2];
      n[3] = a / r[3];

      TestNormalize<xiiMathDoubleBits::FULL>(a, n, r, xiiMath::SmallEpsilon<double>());
#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) && (XII_SSE_LEVEL >= XII_AVX_512)
      TestNormalize<xiiMathDoubleBits::BITS_27>(a, n, r, xiiMath::DefaultEpsilon<double>());
      TestNormalize<xiiMathDoubleBits::BITS_14>(a, n, r, 0.01f);
#endif
    }

    {
      xiiSimdVec4d a(2.0, -2.0, 4.0, -8.0);
      xiiSimdVec4d n[4];
      n[0] = a / 2.0;
      n[1] = a / xiiVec2d(a.x(), a.y()).GetLength();
      n[2] = a / xiiVec3d(a.x(), a.y(), a.z()).GetLength();
      n[3] = a / xiiVec4d(a.x(), a.y(), a.z(), a.w()).GetLength();

      TestNormalizeIfNotZero<xiiMathDoubleBits::FULL>(a, n, xiiMath::SmallEpsilon<double>());
#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) && (XII_SSE_LEVEL >= XII_AVX_512)
      TestNormalizeIfNotZero<xiiMathDoubleBits::BITS_27>(a, n, xiiMath::DefaultEpsilon<double>());
      TestNormalizeIfNotZero<xiiMathDoubleBits::BITS_14>(a, n, xiiMath::HugeEpsilon<double>());
#endif
    }

    {
      xiiSimdVec4d a;

      a.Set(0.0, 2.0, 0.0, 0.0);
      XII_TEST_BOOL(a.IsZero<1>());
      XII_TEST_BOOL(!a.IsZero<2>());

      a.Set(0.0, 0.0, 3.0, 0.0);
      XII_TEST_BOOL(a.IsZero<2>());
      XII_TEST_BOOL(!a.IsZero<3>());

      a.Set(0.0, 0.0, 0.0, 4.0);
      XII_TEST_BOOL(a.IsZero<3>());
      XII_TEST_BOOL(!a.IsZero<4>());

      double smallEps = xiiMath::SmallEpsilon<double>();
      a.Set(smallEps, 2.0, smallEps, smallEps);
      XII_TEST_BOOL(a.IsZero<1>(xiiMath::DefaultEpsilon<double>()));
      XII_TEST_BOOL(!a.IsZero<2>(xiiMath::DefaultEpsilon<double>()));

      a.Set(smallEps, smallEps, 3.0, smallEps);
      XII_TEST_BOOL(a.IsZero<2>(xiiMath::DefaultEpsilon<double>()));
      XII_TEST_BOOL(!a.IsZero<3>(xiiMath::DefaultEpsilon<double>()));

      a.Set(smallEps, smallEps, smallEps, 4.0);
      XII_TEST_BOOL(a.IsZero<3>(xiiMath::DefaultEpsilon<double>()));
      XII_TEST_BOOL(!a.IsZero<4>(xiiMath::DefaultEpsilon<double>()));
    }

    {
      xiiSimdVec4d a;

      double NaN = xiiMath::NaN<double>();
      double Inf = xiiMath::Infinity<double>();

      a.Set(NaN, 1.0, NaN, NaN);
      XII_TEST_BOOL(a.IsNaN<1>());
      XII_TEST_BOOL(a.IsNaN<2>());
      XII_TEST_BOOL(!a.IsValid<2>());

      a.Set(Inf, 1.0, NaN, NaN);
      XII_TEST_BOOL(!a.IsNaN<1>());
      XII_TEST_BOOL(!a.IsNaN<2>());
      XII_TEST_BOOL(!a.IsValid<2>());

      a.Set(1.0, 2.0, Inf, NaN);
      XII_TEST_BOOL(a.IsNaN<4>());
      XII_TEST_BOOL(!a.IsNaN<3>());
      XII_TEST_BOOL(a.IsValid<2>());
      XII_TEST_BOOL(!a.IsValid<3>());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Swizzle")
  {
    xiiSimdVec4d a(3.0, 5.0, 7.0, 9.0);

    xiiSimdVec4d b = a.Get<xiiSwizzle::XXXX>();
    XII_TEST_BOOL(b.x() == 3.0 && b.y() == 3.0 && b.z() == 3.0 && b.w() == 3.0);

    b = a.Get<xiiSwizzle::YYYX>();
    XII_TEST_BOOL(b.x() == 5.0 && b.y() == 5.0 && b.z() == 5.0 && b.w() == 3.0);

    b = a.Get<xiiSwizzle::ZZZX>();
    XII_TEST_BOOL(b.x() == 7.0 && b.y() == 7.0 && b.z() == 7.0 && b.w() == 3.0);

    b = a.Get<xiiSwizzle::WWWX>();
    XII_TEST_BOOL(b.x() == 9.0 && b.y() == 9.0 && b.z() == 9.0 && b.w() == 3.0);

    b = a.Get<xiiSwizzle::WZYX>();
    XII_TEST_BOOL(b.x() == 9.0 && b.y() == 7.0 && b.z() == 5.0 && b.w() == 3.0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Operators")
  {
    {
      xiiSimdVec4d a(-3.0, 5.0, -7.0, 9.0);

      xiiSimdVec4d b = -a;
      XII_TEST_BOOL(b.x() == 3.0 && b.y() == -5.0 && b.z() == 7.0 && b.w() == -9.0);

      b.Set(8.0, 6.0, 4.0, 2.0);
      xiiSimdVec4d c;
      c = a + b;
      XII_TEST_BOOL(c.x() == 5.0 && c.y() == 11.0 && c.z() == -3.0 && c.w() == 11.0);

      c = a - b;
      XII_TEST_BOOL(c.x() == -11.0 && c.y() == -1.0 && c.z() == -11.0 && c.w() == 7.0);

      c = a * xiiSimdDouble(3.0);
      XII_TEST_BOOL(c.x() == -9.0 && c.y() == 15.0 && c.z() == -21.0 && c.w() == 27.0);

      c = a / xiiSimdDouble(2.0);
      XII_TEST_BOOL(c.x() == -1.5 && c.y() == 2.5 && c.z() == -3.5 && c.w() == 4.5);

      c = a.CompMul(b);
      XII_TEST_BOOL(c.x() == -24.0 && c.y() == 30.0 && c.z() == -28.0 && c.w() == 18.0);

      xiiSimdVec4d divRes(-0.375, 5.0 / 6.0, -1.75, 4.5);
      xiiSimdVec4d d1 = a.CompDiv(b);
      xiiSimdVec4d d2 = a.CompDiv<xiiMathDoubleBits::FULL>(b);
#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) && (XII_SSE_LEVEL >= XII_AVX_512)
      xiiSimdVec4d d3 = a.CompDiv<xiiMathDoubleBits::BITS_27>(b);
      xiiSimdVec4d d4 = a.CompDiv<xiiMathDoubleBits::BITS_14>(b);
#endif

      XII_TEST_BOOL(d1.IsEqual(divRes, xiiMath::SmallEpsilon<double>()).AllSet());
      XII_TEST_BOOL(d2.IsEqual(divRes, xiiMath::SmallEpsilon<double>()).AllSet());
#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) && (XII_SSE_LEVEL >= XII_AVX_512)
      XII_TEST_BOOL(d3.IsEqual(divRes, xiiMath::DefaultEpsilon<double>()).AllSet());
      XII_TEST_BOOL(d4.IsEqual(divRes, 0.01f).AllSet());
#endif
    }

    {
      xiiSimdVec4d a(-3.4, 5.4, -7.6, 9.6);
      xiiSimdVec4d b(8.0, 6.0, 4.0, 2.0);
      xiiSimdVec4d c;

      c = a.CompMin(b);
      XII_TEST_BOOL(c.x() == -3.4 && c.y() == 5.4 && c.z() == -7.6 && c.w() == 2.0);

      c = a.CompMax(b);
      XII_TEST_BOOL(c.x() == 8.0 && c.y() == 6.0 && c.z() == 4.0 && c.w() == 9.6);

      c = a.Abs();
      XII_TEST_BOOL(c.x() == 3.4 && c.y() == 5.4 && c.z() == 7.6 && c.w() == 9.6);

      c = a.Round();
      XII_TEST_BOOL(c.x() == -3.0 && c.y() == 5.0 && c.z() == -8.0 && c.w() == 10.0);

      c = a.Floor();
      XII_TEST_BOOL(c.x() == -4.0 && c.y() == 5.0 && c.z() == -8.0 && c.w() == 9.0);

      c = a.Ceil();
      XII_TEST_BOOL(c.x() == -3.0 && c.y() == 6.0 && c.z() == -7.0 && c.w() == 10.0);

      c = a.Trunc();
      XII_TEST_BOOL(c.x() == -3.0 && c.y() == 5.0 && c.z() == -7.0 && c.w() == 9.0);

      c = a.Fraction();
      XII_TEST_BOOL(c.IsEqual(xiiSimdVec4d(-0.4, 0.4, -0.6, 0.6), xiiMath::SmallEpsilon<double>()).AllSet());
    }

    {
      xiiSimdVec4d a(-3.0, 5.0, -7.0, 9.0);
      xiiSimdVec4d b(8.0, 6.0, 4.0, 2.0);

      xiiSimdVec4b cmp(true, false, false, true);
      xiiSimdVec4d c;

      c = a.FlipSign(cmp);
      XII_TEST_BOOL(c.x() == 3.0 && c.y() == 5.0 && c.z() == -7.0 && c.w() == -9.0);

      c = xiiSimdVec4d::Select(cmp, b, a);
      XII_TEST_BOOL(c.x() == 8.0 && c.y() == 5.0 && c.z() == -7.0 && c.w() == 2.0);

      c = xiiSimdVec4d::Select(cmp, a, b);
      XII_TEST_BOOL(c.x() == -3.0 && c.y() == 6.0 && c.z() == 4.0 && c.w() == 9.0);
    }

    {
      xiiSimdVec4d a(-3.0, 5.0, -7.0, 9.0);
      xiiSimdVec4d b(8.0, 6.0, 4.0, 2.0);

      xiiSimdVec4d c = a;
      c += b;
      XII_TEST_BOOL(c.x() == 5.0 && c.y() == 11.0 && c.z() == -3.0 && c.w() == 11.0);

      c = a;
      c -= b;
      XII_TEST_BOOL(c.x() == -11.0 && c.y() == -1.0 && c.z() == -11.0 && c.w() == 7.0);

      c = a;
      c *= xiiSimdDouble(3.0);
      XII_TEST_BOOL(c.x() == -9.0 && c.y() == 15.0 && c.z() == -21.0 && c.w() == 27.0);

      c = a;
      c /= xiiSimdDouble(2.0);
      XII_TEST_BOOL(c.x() == -1.5 && c.y() == 2.5 && c.z() == -3.5 && c.w() == 4.5);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Comparison")
  {
    xiiSimdVec4d a(7.0, 5.0, 4.0, 3.0);
    xiiSimdVec4d b(8.0, 6.0, 4.0, 2.0);
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
      xiiSimdVec4d a(-3.0, 5.0, -7.0, 9.0);

      XII_TEST_DOUBLE(a.HorizontalSum<1>(), -3.0, 0.0);
      XII_TEST_DOUBLE(a.HorizontalSum<2>(), 2.0, 0.0);
      XII_TEST_DOUBLE(a.HorizontalSum<3>(), -5.0, 0.0);
      XII_TEST_DOUBLE(a.HorizontalSum<4>(), 4.0, 0.0);
      XII_TEST_BOOL(AllCompSame(a.HorizontalSum<1>()));
      XII_TEST_BOOL(AllCompSame(a.HorizontalSum<2>()));
      XII_TEST_BOOL(AllCompSame(a.HorizontalSum<3>()));
      XII_TEST_BOOL(AllCompSame(a.HorizontalSum<4>()));

      XII_TEST_DOUBLE(a.HorizontalMin<1>(), -3.0, 0.0);
      XII_TEST_DOUBLE(a.HorizontalMin<2>(), -3.0, 0.0);
      XII_TEST_DOUBLE(a.HorizontalMin<3>(), -7.0, 0.0);
      XII_TEST_DOUBLE(a.HorizontalMin<4>(), -7.0, 0.0);
      XII_TEST_BOOL(AllCompSame(a.HorizontalMin<1>()));
      XII_TEST_BOOL(AllCompSame(a.HorizontalMin<2>()));
      XII_TEST_BOOL(AllCompSame(a.HorizontalMin<3>()));
      XII_TEST_BOOL(AllCompSame(a.HorizontalMin<4>()));

      XII_TEST_DOUBLE(a.HorizontalMax<1>(), -3.0, 0.0);
      XII_TEST_DOUBLE(a.HorizontalMax<2>(), 5.0, 0.0);
      XII_TEST_DOUBLE(a.HorizontalMax<3>(), 5.0, 0.0);
      XII_TEST_DOUBLE(a.HorizontalMax<4>(), 9.0, 0.0);
      XII_TEST_BOOL(AllCompSame(a.HorizontalMax<1>()));
      XII_TEST_BOOL(AllCompSame(a.HorizontalMax<2>()));
      XII_TEST_BOOL(AllCompSame(a.HorizontalMax<3>()));
      XII_TEST_BOOL(AllCompSame(a.HorizontalMax<4>()));
    }

    {
      xiiSimdVec4d a(-3.0, 5.0, -7.0, 9.0);
      xiiSimdVec4d b(8.0, 6.0, 4.0, 2.0);

      XII_TEST_DOUBLE(a.Dot<1>(b), -24.0, 0.0);
      XII_TEST_DOUBLE(a.Dot<2>(b), 6.0, 0.0);
      XII_TEST_DOUBLE(a.Dot<3>(b), -22.0, 0.0);
      XII_TEST_DOUBLE(a.Dot<4>(b), -4.0, 0.0);
      XII_TEST_BOOL(AllCompSame(a.Dot<1>(b)));
      XII_TEST_BOOL(AllCompSame(a.Dot<2>(b)));
      XII_TEST_BOOL(AllCompSame(a.Dot<3>(b)));
      XII_TEST_BOOL(AllCompSame(a.Dot<4>(b)));
    }

    {
      xiiSimdVec4d a(1.0, 2.0, 3.0, 0.0);
      xiiSimdVec4d b(2.0, -4.0, 6.0, 8.0);

      xiiVec3d res = xiiVec3d(a.x(), a.y(), a.z()).CrossRH(xiiVec3d(b.x(), b.y(), b.z()));

      xiiSimdVec4d c = a.CrossRH(b);
      XII_TEST_BOOL(c.x() == res.x);
      XII_TEST_BOOL(c.y() == res.y);
      XII_TEST_BOOL(c.z() == res.z);
    }

    {
      xiiSimdVec4d a(1.0, 2.0, 3.0, 0.0);
      xiiSimdVec4d b(2.0, -4.0, 6.0, 0.0);

      xiiVec3d res = xiiVec3d(a.x(), a.y(), a.z()).CrossRH(xiiVec3d(b.x(), b.y(), b.z()));

      xiiSimdVec4d c = a.CrossRH(b);
      XII_TEST_BOOL(c.x() == res.x);
      XII_TEST_BOOL(c.y() == res.y);
      XII_TEST_BOOL(c.z() == res.z);
    }

    {
      xiiSimdVec4d a(-3.0, 5.0, -7.0, 0.0);
      xiiSimdVec4d b = a.GetOrthogonalVector();

      XII_TEST_BOOL(!b.IsZero<3>());
      XII_TEST_DOUBLE(a.Dot<3>(b), 0.0, 0.0);
    }

    {
      xiiSimdVec4d a(-3.0, 5.0, -7.0, 9.0);
      xiiSimdVec4d b(8.0, 6.0, 4.0, 2.0);
      xiiSimdVec4d c(1.0, 2.0, 3.0, 4.0);
      xiiSimdVec4d d;

      d = xiiSimdVec4d::MulAdd(a, b, c);
      XII_TEST_BOOL(d.x() == -23.0 && d.y() == 32.0 && d.z() == -25.0 && d.w() == 22.0);

      d = xiiSimdVec4d::MulAdd(a, xiiSimdDouble(3.0), c);
      XII_TEST_BOOL(d.x() == -8.0 && d.y() == 17.0 && d.z() == -18.0 && d.w() == 31.0);

      d = xiiSimdVec4d::MulSub(a, b, c);
      XII_TEST_BOOL(d.x() == -25.0 && d.y() == 28.0 && d.z() == -31.0 && d.w() == 14.0);

      d = xiiSimdVec4d::MulSub(a, xiiSimdDouble(3.0), c);
      XII_TEST_BOOL(d.x() == -10.0 && d.y() == 13.0 && d.z() == -24.0 && d.w() == 23.0);

      d = xiiSimdVec4d::CopySign(b, a);
      XII_TEST_BOOL(d.x() == -8.0 && d.y() == 6.0 && d.z() == -4.0 && d.w() == 2.0);
    }
  }
}
