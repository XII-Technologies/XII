#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdMath.h>

namespace
{
  xiiSimdVec4f SimdDegree(float degree)
  {
    return xiiSimdVec4f(xiiAngle::Degree(degree));
  }
} // namespace

XII_CREATE_SIMPLE_TEST(SimdMath, SimdMath)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Exp")
  {
    float testVals[] = {0.0f, 1.0f, 2.0f};
    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(testVals); ++i)
    {
      const float v = testVals[i];
      const float r = xiiMath::Exp(v);
      XII_TEST_BOOL(xiiSimdMath::Exp(xiiSimdVec4f(v)).IsEqual(xiiSimdVec4f(r), 0.000001f).AllSet());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Ln")
  {
    float testVals[] = {1.0f, 2.7182818284f, 7.3890560989f};
    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(testVals); ++i)
    {
      const float v = testVals[i];
      const float r = xiiMath::Ln(v);
      XII_TEST_BOOL(xiiSimdMath::Ln(xiiSimdVec4f(v)).IsEqual(xiiSimdVec4f(r), 0.000001f).AllSet());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Log2")
  {
    float testVals[] = {1.0f, 2.0f, 4.0f};
    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(testVals); ++i)
    {
      const float v = testVals[i];
      const float r = xiiMath::Log2(v);
      XII_TEST_BOOL(xiiSimdMath::Log2(xiiSimdVec4f(v)).IsEqual(xiiSimdVec4f(r), 0.000001f).AllSet());
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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Log10")
  {
    float testVals[] = {1.0f, 10.0f, 100.0f};
    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(testVals); ++i)
    {
      const float v = testVals[i];
      const float r = xiiMath::Log10(v);
      XII_TEST_BOOL(xiiSimdMath::Log10(xiiSimdVec4f(v)).IsEqual(xiiSimdVec4f(r), 0.000001f).AllSet());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Pow2")
  {
    float testVals[] = {0.0f, 1.0f, 2.0f};
    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(testVals); ++i)
    {
      const float v = testVals[i];
      const float r = xiiMath::Pow2(v);
      XII_TEST_BOOL(xiiSimdMath::Pow2(xiiSimdVec4f(v)).IsEqual(xiiSimdVec4f(r), 0.000001f).AllSet());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Sin")
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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Cos")
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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Tan")
  {
    XII_TEST_BOOL(xiiSimdMath::Tan(SimdDegree(0.0f)).IsEqual(xiiSimdVec4f(0.0f), 0.000001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Tan(SimdDegree(45.0f)).IsEqual(xiiSimdVec4f(1.0f), 0.000001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::Tan(SimdDegree(-45.0f)).IsEqual(xiiSimdVec4f(-1.0f), 0.000001f).AllSet());
    XII_TEST_BOOL((xiiSimdMath::Tan(SimdDegree(90.00001f)) < xiiSimdVec4f(1000000.0f)).AllSet());
    XII_TEST_BOOL((xiiSimdMath::Tan(SimdDegree(89.9999f)) > xiiSimdVec4f(100000.0f)).AllSet());

    // Testing the period of tan(x) centered at 0 and the adjacent ones
    xiiAngle angle = xiiAngle::Degree(-89.0f);
    while (angle.GetDegree() < 89.0f)
    {
      xiiSimdVec4f simdAngle(angle.GetRadian());

      xiiSimdVec4f fTan     = xiiSimdMath::Tan(simdAngle);
      xiiSimdVec4f fTanPrev = xiiSimdMath::Tan(SimdDegree(angle.GetDegree() - 180.0f));
      xiiSimdVec4f fTanNext = xiiSimdMath::Tan(SimdDegree(angle.GetDegree() + 180.0f));
      xiiSimdVec4f fSin     = xiiSimdMath::Sin(simdAngle);
      xiiSimdVec4f fCos     = xiiSimdMath::Cos(simdAngle);

      XII_TEST_BOOL((fTan - fTanPrev).IsEqual(xiiSimdVec4f::ZeroVector(), 0.002f).AllSet());
      XII_TEST_BOOL((fTan - fTanNext).IsEqual(xiiSimdVec4f::ZeroVector(), 0.002f).AllSet());
      XII_TEST_BOOL((fTan - fSin.CompDiv(fCos)).IsEqual(xiiSimdVec4f::ZeroVector(), 0.0005f).AllSet());
      angle += xiiAngle::Degree(1.234f);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ASin")
  {
    XII_TEST_BOOL(xiiSimdMath::ASin(xiiSimdVec4f(0.0f)).IsEqual(SimdDegree(0.0f), 0.0001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ASin(xiiSimdVec4f(1.0f)).IsEqual(SimdDegree(90.0f), 0.00001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ASin(xiiSimdVec4f(-1.0f)).IsEqual(SimdDegree(-90.0f), 0.00001f).AllSet());

    XII_TEST_BOOL(xiiSimdMath::ASin(xiiSimdVec4f(0.7071067f)).IsEqual(SimdDegree(45.0f), 0.0001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ASin(xiiSimdVec4f(-0.7071067f)).IsEqual(SimdDegree(-45.0f), 0.0001f).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ACos")
  {
    XII_TEST_BOOL(xiiSimdMath::ACos(xiiSimdVec4f(0.0f)).IsEqual(SimdDegree(90.0f), 0.0001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ACos(xiiSimdVec4f(1.0f)).IsEqual(SimdDegree(0.0f), 0.00001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ACos(xiiSimdVec4f(-1.0f)).IsEqual(SimdDegree(180.0f), 0.0001f).AllSet());

    XII_TEST_BOOL(xiiSimdMath::ACos(xiiSimdVec4f(0.7071067f)).IsEqual(SimdDegree(45.0f), 0.0001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ACos(xiiSimdVec4f(-0.7071067f)).IsEqual(SimdDegree(135.0f), 0.0001f).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ATan")
  {
    XII_TEST_BOOL(xiiSimdMath::ATan(xiiSimdVec4f(0.0f)).IsEqual(SimdDegree(0.0f), 0.0000001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ATan(xiiSimdVec4f(1.0f)).IsEqual(SimdDegree(45.0f), 0.00001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ATan(xiiSimdVec4f(-1.0f)).IsEqual(SimdDegree(-45.0f), 0.00001f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ATan(xiiSimdVec4f(10000000.0f)).IsEqual(SimdDegree(90.0f), 0.00002f).AllSet());
    XII_TEST_BOOL(xiiSimdMath::ATan(xiiSimdVec4f(-10000000.0f)).IsEqual(SimdDegree(-90.0f), 0.00002f).AllSet());
  }
}
