#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdMath.h>

namespace
{
  xiiSimdVec4f SimdDegree(float fDegree)
  {
    return xiiSimdVec4f(xiiAngle::MakeFromDegree(fDegree));
  }

  xiiSimdVec4d SimdDegree(double fDegree)
  {
    return xiiSimdVec4d(xiiAngled::MakeFromDegree(fDegree));
  }
} // namespace

XII_CREATE_SIMPLE_TEST(SimdMath, SimdMath)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Exp (float)")
  {
    float testVals[] = {0.0f, 1.0f, 2.0f};
    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(testVals); ++i)
    {
      const float v = testVals[i];
      const float r = xiiMath::Exp(v);
      XII_TEST_BOOL(xiiSimdMath::Exp(xiiSimdVec4f(v)).IsEqual(xiiSimdVec4f(r), 0.000001f).AllSet());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Exp (double)")
  {
    double testVals[] = {0.0, 1.0, 2.0};
    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(testVals); ++i)
    {
      const double v = testVals[i];
      const double r = xiiMath::Exp(v);
      XII_TEST_BOOL(xiiSimdMath::Exp(xiiSimdVec4d(v)).IsEqual(xiiSimdVec4d(r), 0.000001).AllSet());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Ln (float)")
  {
    float testVals[] = {1.0f, 2.7182818284f, 7.3890560989f};
    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(testVals); ++i)
    {
      const float v = testVals[i];
      const float r = xiiMath::Ln(v);
      XII_TEST_BOOL(xiiSimdMath::Ln(xiiSimdVec4f(v)).IsEqual(xiiSimdVec4f(r), 0.000001f).AllSet());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Ln (double)")
  {
    double testVals[] = {1.0, 2.7182818284, 7.3890560989};
    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(testVals); ++i)
    {
      const double v = testVals[i];
      const double r = xiiMath::Ln(v);
      XII_TEST_BOOL(xiiSimdMath::Ln(xiiSimdVec4d(v)).IsEqual(xiiSimdVec4d(r), 0.000001).AllSet());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Log2 (float)")
  {
    float testVals[] = {1.0f, 2.0f, 4.0f};
    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(testVals); ++i)
    {
      const float v = testVals[i];
      const float r = xiiMath::Log2(v);
      XII_TEST_BOOL(xiiSimdMath::Log2(xiiSimdVec4f(v)).IsEqual(xiiSimdVec4f(r), 0.000001f).AllSet());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Log2 (double)")
  {
    double testVals[] = {1.0, 2.0, 4.0};
    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(testVals); ++i)
    {
      const double v = testVals[i];
      const double r = xiiMath::Log2(v);
      XII_TEST_BOOL(xiiSimdMath::Log2(xiiSimdVec4d(v)).IsEqual(xiiSimdVec4d(r), 0.000001).AllSet());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Log2i")
  {
    int testVals[] = {0, 1, 2, 3, 4, 6, 7, 8};
    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(testVals); ++i)
    {
      const int v = testVals[i];
      const int r = xiiMath::Log2i(v);
      XII_TEST_BOOL((xiiSimdMath::Log2i(xiiSimdVec4i(v)) == xiiSimdVec4i(r)).AllSet());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Log10 (float)")
  {
    float testVals[] = {1.0f, 10.0f, 100.0f};
    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(testVals); ++i)
    {
      const float v = testVals[i];
      const float r = xiiMath::Log10(v);
      XII_TEST_BOOL(xiiSimdMath::Log10(xiiSimdVec4f(v)).IsEqual(xiiSimdVec4f(r), 0.000001f).AllSet());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Log10 (double)")
  {
    double testVals[] = {1.0, 10.0, 100.0};
    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(testVals); ++i)
    {
      const double v = testVals[i];
      const double r = xiiMath::Log10(v);
      XII_TEST_BOOL(xiiSimdMath::Log10(xiiSimdVec4d(v)).IsEqual(xiiSimdVec4d(r), 0.000001).AllSet());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Pow2 (float)")
  {
    float testVals[] = {0.0f, 1.0f, 2.0f};
    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(testVals); ++i)
    {
      const float v = testVals[i];
      const float r = xiiMath::Pow2(v);
      XII_TEST_BOOL(xiiSimdMath::Pow2(xiiSimdVec4f(v)).IsEqual(xiiSimdVec4f(r), 0.000001f).AllSet());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Pow2 (double)")
  {
    double testVals[] = {0.0, 1.0, 2.0};
    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(testVals); ++i)
    {
      const double v = testVals[i];
      const double r = xiiMath::Pow2(v);
      XII_TEST_BOOL(xiiSimdMath::Pow2(xiiSimdVec4d(v)).IsEqual(xiiSimdVec4d(r), 0.000001).AllSet());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Sin (float)")
  {
    XII_TEST_BOOL(xiiSimdMath::Sin(SimdDegree(0.0f)).IsEqual(xiiSimdVec4f(0.0f), 0.000001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Sin(SimdDegree(90.0f)).IsEqual(xiiSimdVec4f(1.0f), 0.000001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Sin(SimdDegree(180.0f)).IsEqual(xiiSimdVec4f(0.0f), 0.000001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Sin(SimdDegree(270.0f)).IsEqual(xiiSimdVec4f(-1.0f), 0.000001f).AllSet());

    XII_TEST_BOOL(xiiSimdMath::Sin(SimdDegree(45.0f)).IsEqual(xiiSimdVec4f(0.7071067f), 0.000001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Sin(SimdDegree(135.0f)).IsEqual(xiiSimdVec4f(0.7071067f), 0.000001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Sin(SimdDegree(225.0f)).IsEqual(xiiSimdVec4f(-0.7071067f), 0.000001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Sin(SimdDegree(315.0f)).IsEqual(xiiSimdVec4f(-0.7071067f), 0.000001f).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Sin (double)")
  {
    XII_TEST_BOOL(xiiSimdMath::Sin(SimdDegree(0.0)).IsEqual(xiiSimdVec4d(0.0), 0.000001).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Sin(SimdDegree(90.0)).IsEqual(xiiSimdVec4d(1.0), 0.000001).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Sin(SimdDegree(180.0)).IsEqual(xiiSimdVec4d(0.0), 0.000001).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Sin(SimdDegree(270.0)).IsEqual(xiiSimdVec4d(-1.0), 0.000001).AllSet());

    XII_TEST_BOOL(xiiSimdMath::Sin(SimdDegree(45.0)).IsEqual(xiiSimdVec4d(0.7071067), 0.000001).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Sin(SimdDegree(135.0)).IsEqual(xiiSimdVec4d(0.7071067), 0.000001).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Sin(SimdDegree(225.0)).IsEqual(xiiSimdVec4d(-0.7071067), 0.000001).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Sin(SimdDegree(315.0)).IsEqual(xiiSimdVec4d(-0.7071067), 0.000001).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Cos (float)")
  {
    XII_TEST_BOOL(xiiSimdMath::Cos(SimdDegree(0.0f)).IsEqual(xiiSimdVec4f(1.0f), 0.000001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Cos(SimdDegree(90.0f)).IsEqual(xiiSimdVec4f(0.0f), 0.000001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Cos(SimdDegree(180.0f)).IsEqual(xiiSimdVec4f(-1.0f), 0.000001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Cos(SimdDegree(270.0f)).IsEqual(xiiSimdVec4f(0.0f), 0.000001f).AllSet());

    XII_TEST_BOOL(xiiSimdMath::Cos(SimdDegree(45.0f)).IsEqual(xiiSimdVec4f(0.7071067f), 0.000001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Cos(SimdDegree(135.0f)).IsEqual(xiiSimdVec4f(-0.7071067f), 0.000001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Cos(SimdDegree(225.0f)).IsEqual(xiiSimdVec4f(-0.7071067f), 0.000001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Cos(SimdDegree(315.0f)).IsEqual(xiiSimdVec4f(0.7071067f), 0.000001f).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Cos (double)")
  {
    XII_TEST_BOOL(xiiSimdMath::Cos(SimdDegree(0.0)).IsEqual(xiiSimdVec4d(1.0), 0.000001).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Cos(SimdDegree(90.0)).IsEqual(xiiSimdVec4d(0.0), 0.000001).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Cos(SimdDegree(180.0)).IsEqual(xiiSimdVec4d(-1.0), 0.000001).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Cos(SimdDegree(270.0)).IsEqual(xiiSimdVec4d(0.0), 0.000001).AllSet());

    XII_TEST_BOOL(xiiSimdMath::Cos(SimdDegree(45.0)).IsEqual(xiiSimdVec4d(0.7071067), 0.000001).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Cos(SimdDegree(135.0)).IsEqual(xiiSimdVec4d(-0.7071067), 0.000001).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Cos(SimdDegree(225.0)).IsEqual(xiiSimdVec4d(-0.7071067), 0.000001).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Cos(SimdDegree(315.0)).IsEqual(xiiSimdVec4d(0.7071067), 0.000001).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Tan (float)")
  {
    XII_TEST_BOOL(xiiSimdMath::Tan(SimdDegree(0.0f)).IsEqual(xiiSimdVec4f(0.0f), 0.000001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Tan(SimdDegree(45.0f)).IsEqual(xiiSimdVec4f(1.0f), 0.000001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Tan(SimdDegree(-45.0f)).IsEqual(xiiSimdVec4f(-1.0f), 0.000001f).AllSet());
    XII_TEST_BOOL((xiiSimdMath::Tan(SimdDegree(90.00001f)) < xiiSimdVec4f(1000000.0f)).AllSet());
    XII_TEST_BOOL((xiiSimdMath::Tan(SimdDegree(89.9999f)) > xiiSimdVec4f(100000.0f)).AllSet());

    // Testing the period of tan(x) centered at 0 and the adjacent ones
    xiiAngle angle = xiiAngle::MakeFromDegree(-89.0f);
    while (angle.GetDegree() < 89.0f)
    {
      xiiSimdVec4f simdAngle(angle.GetRadian());

      xiiSimdVec4f fTan     = xiiSimdMath::Tan(simdAngle);
      xiiSimdVec4f fTanPrev = xiiSimdMath::Tan(SimdDegree(angle.GetDegree() - 180.0f));
      xiiSimdVec4f fTanNext = xiiSimdMath::Tan(SimdDegree(angle.GetDegree() + 180.0f));
      xiiSimdVec4f fSin     = xiiSimdMath::Sin(simdAngle);
      xiiSimdVec4f fCos     = xiiSimdMath::Cos(simdAngle);

      XII_TEST_BOOL((fTan - fTanPrev).IsEqual(xiiSimdVec4f::MakeZero(), 0.002f).AllSet());
      XII_TEST_BOOL((fTan - fTanNext).IsEqual(xiiSimdVec4f::MakeZero(), 0.002f).AllSet());
      XII_TEST_BOOL((fTan - fSin.CompDiv(fCos)).IsEqual(xiiSimdVec4f::MakeZero(), 0.0005f).AllSet());
      angle += xiiAngle::MakeFromDegree(1.234f);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Tan (double)")
  {
    XII_TEST_BOOL(xiiSimdMath::Tan(SimdDegree(0.0)).IsEqual(xiiSimdVec4d(0.0), 0.000001).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Tan(SimdDegree(45.0)).IsEqual(xiiSimdVec4d(1.0), 0.000001).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Tan(SimdDegree(-45.0)).IsEqual(xiiSimdVec4d(-1.0), 0.000001).AllSet());
    XII_TEST_BOOL((xiiSimdMath::Tan(SimdDegree(90.00001)) < xiiSimdVec4d(1000000.0)).AllSet());
    XII_TEST_BOOL((xiiSimdMath::Tan(SimdDegree(89.9999)) > xiiSimdVec4d(100000.0)).AllSet());

    // Testing the period of tan(x) centered at 0 and the adjacent ones
    xiiAngled angle = xiiAngled::MakeFromDegree(-89.0);
    while (angle.GetDegree() < 89.0)
    {
      xiiSimdVec4d simdAngle(angle.GetRadian());

      xiiSimdVec4d fTan     = xiiSimdMath::Tan(simdAngle);
      xiiSimdVec4d fTanPrev = xiiSimdMath::Tan(SimdDegree(angle.GetDegree() - 180.0));
      xiiSimdVec4d fTanNext = xiiSimdMath::Tan(SimdDegree(angle.GetDegree() + 180.0));
      xiiSimdVec4d fSin     = xiiSimdMath::Sin(simdAngle);
      xiiSimdVec4d fCos     = xiiSimdMath::Cos(simdAngle);

      XII_TEST_BOOL((fTan - fTanPrev).IsEqual(xiiSimdVec4d::MakeZero(), 0.002).AllSet());
      XII_TEST_BOOL((fTan - fTanNext).IsEqual(xiiSimdVec4d::MakeZero(), 0.002).AllSet());
      XII_TEST_BOOL((fTan - fSin.CompDiv(fCos)).IsEqual(xiiSimdVec4d::MakeZero(), 0.0005).AllSet());
      angle += xiiAngled::MakeFromDegree(1.234);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ASin (float)")
  {
    XII_TEST_BOOL(xiiSimdMath::ASin(xiiSimdVec4f(0.0f)).IsEqual(SimdDegree(0.0f), 0.0001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ASin(xiiSimdVec4f(1.0f)).IsEqual(SimdDegree(90.0f), 0.00001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ASin(xiiSimdVec4f(-1.0f)).IsEqual(SimdDegree(-90.0f), 0.00001f).AllSet());

    XII_TEST_BOOL(xiiSimdMath::ASin(xiiSimdVec4f(0.7071067f)).IsEqual(SimdDegree(45.0f), 0.0001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ASin(xiiSimdVec4f(-0.7071067f)).IsEqual(SimdDegree(-45.0f), 0.0001f).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ASin (double)")
  {
    XII_TEST_BOOL(xiiSimdMath::ASin(xiiSimdVec4d(0.0)).IsEqual(SimdDegree(0.0), 0.0001).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ASin(xiiSimdVec4d(1.0)).IsEqual(SimdDegree(90.0), 0.00001).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ASin(xiiSimdVec4d(-1.0)).IsEqual(SimdDegree(-90.0), 0.00001).AllSet());

    XII_TEST_BOOL(xiiSimdMath::ASin(xiiSimdVec4d(0.7071067)).IsEqual(SimdDegree(45.0), 0.0001).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ASin(xiiSimdVec4d(-0.7071067)).IsEqual(SimdDegree(-45.0), 0.0001).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ACos (float)")
  {
    XII_TEST_BOOL(xiiSimdMath::ACos(xiiSimdVec4f(0.0f)).IsEqual(SimdDegree(90.0f), 0.0001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ACos(xiiSimdVec4f(1.0f)).IsEqual(SimdDegree(0.0f), 0.00001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ACos(xiiSimdVec4f(-1.0f)).IsEqual(SimdDegree(180.0f), 0.0001f).AllSet());

    XII_TEST_BOOL(xiiSimdMath::ACos(xiiSimdVec4f(0.7071067f)).IsEqual(SimdDegree(45.0f), 0.0001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ACos(xiiSimdVec4f(-0.7071067f)).IsEqual(SimdDegree(135.0f), 0.0001f).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ACos (double)")
  {
    XII_TEST_BOOL(xiiSimdMath::ACos(xiiSimdVec4d(0.0)).IsEqual(SimdDegree(90.0), 0.0001).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ACos(xiiSimdVec4d(1.0)).IsEqual(SimdDegree(0.0), 0.00001).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ACos(xiiSimdVec4d(-1.0)).IsEqual(SimdDegree(180.0), 0.0001).AllSet());

    XII_TEST_BOOL(xiiSimdMath::ACos(xiiSimdVec4d(0.7071067)).IsEqual(SimdDegree(45.0), 0.0001).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ACos(xiiSimdVec4d(-0.7071067)).IsEqual(SimdDegree(135.0), 0.0001).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ATan (float)")
  {
    XII_TEST_BOOL(xiiSimdMath::ATan(xiiSimdVec4f(0.0f)).IsEqual(SimdDegree(0.0f), 0.0000001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ATan(xiiSimdVec4f(1.0f)).IsEqual(SimdDegree(45.0f), 0.00001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ATan(xiiSimdVec4f(-1.0f)).IsEqual(SimdDegree(-45.0f), 0.00001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ATan(xiiSimdVec4f(10000000.0f)).IsEqual(SimdDegree(90.0f), 0.00002f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ATan(xiiSimdVec4f(-10000000.0f)).IsEqual(SimdDegree(-90.0f), 0.00002f).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ATan (double)")
  {
    XII_TEST_BOOL(xiiSimdMath::ATan(xiiSimdVec4d(0.0)).IsEqual(SimdDegree(0.0), 0.0000001).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ATan(xiiSimdVec4d(1.0)).IsEqual(SimdDegree(45.0), 0.00001).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ATan(xiiSimdVec4d(-1.0)).IsEqual(SimdDegree(-45.0), 0.00001).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ATan(xiiSimdVec4d(10000000.0)).IsEqual(SimdDegree(90.0), 0.00002).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ATan(xiiSimdVec4d(-10000000.0)).IsEqual(SimdDegree(-90.0), 0.00002).AllSet());
  }
}
