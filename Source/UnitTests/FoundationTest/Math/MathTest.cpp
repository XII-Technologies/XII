#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Math/Vec2.h>

/// ********************* Binary to Int conversion *********************
/// Most significant bit comes first.
/// Adapted from http://bytes.com/topic/c/answers/219656-literal-binary
///
/// Sample usage:
/// XII_8BIT(01010101) == 85
/// XII_16BIT(10101010, 01010101) == 43605
/// XII_32BIT(10000000, 11111111, 10101010, 01010101) == 2164238933
/// ********************************************************************
#define OCT__(n) 0##n##LU

#define XII_8BIT__(iBits)                                                                                                          \
  (((iBits & 000000001) ? 1 : 0) + ((iBits & 000000010) ? 2 : 0) + ((iBits & 000000100) ? 4 : 0) + ((iBits & 000001000) ? 8 : 0) + \
   ((iBits & 000010000) ? 16 : 0) + ((iBits & 000100000) ? 32 : 0) + ((iBits & 001000000) ? 64 : 0) + ((iBits & 010000000) ? 128 : 0))

#define XII_8BIT(B) ((xiiUInt8)XII_8BIT__(OCT__(B)))

#define XII_16BIT(B2, B1) (((xiiUInt8)XII_8BIT(B2) << 8) + XII_8BIT(B1))

#define XII_32BIT(B4, B3, B2, B1) \
  ((unsigned long)XII_8BIT(B4) << 24) + ((unsigned long)XII_8BIT(B3) << 16) + ((unsigned long)XII_8BIT(B2) << 8) + ((unsigned long)XII_8BIT(B1))

namespace
{
  struct UniqueInt
  {
    int i, id;
    UniqueInt(int i, int id) :
      i(i), id(id)
    {
    }

    bool operator<(const UniqueInt& rh) { return this->i < rh.i; }

    bool operator>(const UniqueInt& rh) { return this->i > rh.i; }
  };
}; // namespace


XII_CREATE_SIMPLE_TEST_GROUP(Math);

XII_CREATE_SIMPLE_TEST(Math, General)
{
  // XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constants")
  //{
  //  // Macro test
  //  XII_TEST_BOOL(XII_8BIT(01010101) == 85);
  //  XII_TEST_BOOL(XII_16BIT(10101010, 01010101) == 43605);
  //  XII_TEST_BOOL(XII_32BIT(10000000, 11111111, 10101010, 01010101) == 2164238933);

  //  // Infinity test
  //  //                           Sign:_
  //  //                       Exponent: _______  _
  //  //                       Fraction:           _______  ________  ________
  //  xiiIntFloatUnion uInf = { XII_32BIT(01111111, 10000000, 00000000, 00000000) };
  //  XII_TEST_BOOL(uInf.f == xiiMath::FloatInfinity());

  //  // FloatMax_Pos test
  //  xiiIntFloatUnion uMax = { XII_32BIT(01111111, 01111111, 11111111, 11111111) };
  //  XII_TEST_BOOL(uMax.f == xiiMath::FloatMax_Pos());

  //  // FloatMax_Neg test
  //  xiiIntFloatUnion uMin = { XII_32BIT(11111111, 01111111, 11111111, 11111111) };
  //  XII_TEST_BOOL(uMin.f == xiiMath::FloatMax_Neg());
  //}

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Sin")
  {
    XII_TEST_FLOAT(xiiMath::Sin(xiiAngle::Degree(0.0f)), 0.0f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::Sin(xiiAngle::Degree(90.0f)), 1.0f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::Sin(xiiAngle::Degree(180.0f)), 0.0f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::Sin(xiiAngle::Degree(270.0f)), -1.0f, 0.000001f);

    XII_TEST_FLOAT(xiiMath::Sin(xiiAngle::Degree(45.0f)), 0.7071067f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::Sin(xiiAngle::Degree(135.0f)), 0.7071067f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::Sin(xiiAngle::Degree(225.0f)), -0.7071067f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::Sin(xiiAngle::Degree(315.0f)), -0.7071067f, 0.000001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Cos")
  {
    XII_TEST_FLOAT(xiiMath::Cos(xiiAngle::Degree(0.0f)), 1.0f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::Cos(xiiAngle::Degree(90.0f)), 0.0f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::Cos(xiiAngle::Degree(180.0f)), -1.0f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::Cos(xiiAngle::Degree(270.0f)), 0.0f, 0.000001f);

    XII_TEST_FLOAT(xiiMath::Cos(xiiAngle::Degree(45.0f)), 0.7071067f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::Cos(xiiAngle::Degree(135.0f)), -0.7071067f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::Cos(xiiAngle::Degree(225.0f)), -0.7071067f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::Cos(xiiAngle::Degree(315.0f)), 0.7071067f, 0.000001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Tan")
  {
    XII_TEST_FLOAT(xiiMath::Tan(xiiAngle::Degree(0.0f)), 0.0f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::Tan(xiiAngle::Degree(45.0f)), 1.0f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::Tan(xiiAngle::Degree(-45.0f)), -1.0f, 0.000001f);
    XII_TEST_BOOL(xiiMath::Tan(xiiAngle::Degree(90.00001f)) < 1000000.0f);
    XII_TEST_BOOL(xiiMath::Tan(xiiAngle::Degree(89.9999f)) > 100000.0f);

    // Testing the period of tan(x) centered at 0 and the adjacent ones
    xiiAngle angle = xiiAngle::Degree(-89.0f);
    while (angle.GetDegree() < 89.0f)
    {
      float fTan     = xiiMath::Tan(angle);
      float fTanPrev = xiiMath::Tan(xiiAngle::Degree(angle.GetDegree() - 180.0f));
      float fTanNext = xiiMath::Tan(xiiAngle::Degree(angle.GetDegree() + 180.0f));
      float fSin     = xiiMath::Sin(angle);
      float fCos     = xiiMath::Cos(angle);

      XII_TEST_FLOAT(fTan - fTanPrev, 0.0f, 0.002f);
      XII_TEST_FLOAT(fTan - fTanNext, 0.0f, 0.002f);
      XII_TEST_FLOAT(fTan - (fSin / fCos), 0.0f, 0.0005f);
      angle += xiiAngle::Degree(1.234f);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ASin")
  {
    XII_TEST_FLOAT(xiiMath::ASin(0.0f).GetDegree(), 0.0f, 0.00001f);
    XII_TEST_FLOAT(xiiMath::ASin(1.0f).GetDegree(), 90.0f, 0.00001f);
    XII_TEST_FLOAT(xiiMath::ASin(-1.0f).GetDegree(), -90.0f, 0.00001f);

    XII_TEST_FLOAT(xiiMath::ASin(0.7071067f).GetDegree(), 45.0f, 0.0001f);
    XII_TEST_FLOAT(xiiMath::ASin(-0.7071067f).GetDegree(), -45.0f, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ACos")
  {
    XII_TEST_FLOAT(xiiMath::ACos(0.0f).GetDegree(), 90.0f, 0.00001f);
    XII_TEST_FLOAT(xiiMath::ACos(1.0f).GetDegree(), 0.0f, 0.00001f);
    XII_TEST_FLOAT(xiiMath::ACos(-1.0f).GetDegree(), 180.0f, 0.0001f);

    XII_TEST_FLOAT(xiiMath::ACos(0.7071067f).GetDegree(), 45.0f, 0.0001f);
    XII_TEST_FLOAT(xiiMath::ACos(-0.7071067f).GetDegree(), 135.0f, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ATan")
  {
    XII_TEST_FLOAT(xiiMath::ATan(0.0f).GetDegree(), 0.0f, 0.0000001f);
    XII_TEST_FLOAT(xiiMath::ATan(1.0f).GetDegree(), 45.0f, 0.00001f);
    XII_TEST_FLOAT(xiiMath::ATan(-1.0f).GetDegree(), -45.0f, 0.00001f);
    XII_TEST_FLOAT(xiiMath::ATan(10000000.0f).GetDegree(), 90.0f, 0.00002f);
    XII_TEST_FLOAT(xiiMath::ATan(-10000000.0f).GetDegree(), -90.0f, 0.00002f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ATan2")
  {
    for (float fScale = 0.125f; fScale < 1000000.0f; fScale *= 2.0f)
    {
      XII_TEST_FLOAT(xiiMath::ATan2(0.0f, fScale).GetDegree(), 0.0f, 0.0000001f);
      XII_TEST_FLOAT(xiiMath::ATan2(fScale, fScale).GetDegree(), 45.0f, 0.00001f);
      XII_TEST_FLOAT(xiiMath::ATan2(fScale, 0.0f).GetDegree(), 90.0f, 0.00001f);
      XII_TEST_FLOAT(xiiMath::ATan2(-fScale, fScale).GetDegree(), -45.0f, 0.00001f);
      XII_TEST_FLOAT(xiiMath::ATan2(-fScale, 0.0f).GetDegree(), -90.0f, 0.00001f);
      XII_TEST_FLOAT(xiiMath::ATan2(0.0f, -fScale).GetDegree(), 180.0f, 0.0001f);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Exp")
  {
    XII_TEST_FLOAT(1.0f, xiiMath::Exp(0.0f), 0.000001f);
    XII_TEST_FLOAT(2.7182818284f, xiiMath::Exp(1.0f), 0.000001f);
    XII_TEST_FLOAT(7.3890560989f, xiiMath::Exp(2.0f), 0.000001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Ln")
  {
    XII_TEST_FLOAT(0.0f, xiiMath::Ln(1.0f), 0.000001f);
    XII_TEST_FLOAT(1.0f, xiiMath::Ln(2.7182818284f), 0.000001f);
    XII_TEST_FLOAT(2.0f, xiiMath::Ln(7.3890560989f), 0.000001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Log2")
  {
    XII_TEST_FLOAT(0.0f, xiiMath::Log2(1.0f), 0.000001f);
    XII_TEST_FLOAT(1.0f, xiiMath::Log2(2.0f), 0.000001f);
    XII_TEST_FLOAT(2.0f, xiiMath::Log2(4.0f), 0.000001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Log2i")
  {
    XII_TEST_BOOL(xiiMath::Log2i(1) == 0);
    XII_TEST_BOOL(xiiMath::Log2i(2) == 1);
    XII_TEST_BOOL(xiiMath::Log2i(3) == 1);
    XII_TEST_BOOL(xiiMath::Log2i(4) == 2);
    XII_TEST_BOOL(xiiMath::Log2i(6) == 2);
    XII_TEST_BOOL(xiiMath::Log2i(7) == 2);
    XII_TEST_BOOL(xiiMath::Log2i(8) == 3);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Log10")
  {
    XII_TEST_FLOAT(0.0f, xiiMath::Log10(1.0f), 0.000001f);
    XII_TEST_FLOAT(1.0f, xiiMath::Log10(10.0f), 0.000001f);
    XII_TEST_FLOAT(2.0f, xiiMath::Log10(100.0f), 0.000001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Log")
  {
    XII_TEST_FLOAT(0.0f, xiiMath::Log(2.7182818284f, 1.0f), 0.000001f);
    XII_TEST_FLOAT(1.0f, xiiMath::Log(2.7182818284f, 2.7182818284f), 0.000001f);
    XII_TEST_FLOAT(2.0f, xiiMath::Log(2.7182818284f, 7.3890560989f), 0.000001f);

    XII_TEST_FLOAT(0.0f, xiiMath::Log(2.0f, 1.0f), 0.000001f);
    XII_TEST_FLOAT(1.0f, xiiMath::Log(2.0f, 2.0f), 0.000001f);
    XII_TEST_FLOAT(2.0f, xiiMath::Log(2.0f, 4.0f), 0.000001f);

    XII_TEST_FLOAT(0.0f, xiiMath::Log(10.0f, 1.0f), 0.000001f);
    XII_TEST_FLOAT(1.0f, xiiMath::Log(10.0f, 10.0f), 0.000001f);
    XII_TEST_FLOAT(2.0f, xiiMath::Log(10.0f, 100.0f), 0.000001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Pow2")
  {
    XII_TEST_FLOAT(1.0f, xiiMath::Pow2(0.0f), 0.000001f);
    XII_TEST_FLOAT(2.0f, xiiMath::Pow2(1.0f), 0.000001f);
    XII_TEST_FLOAT(4.0f, xiiMath::Pow2(2.0f), 0.000001f);

    XII_TEST_BOOL(xiiMath::Pow2(0) == 1);
    XII_TEST_BOOL(xiiMath::Pow2(1) == 2);
    XII_TEST_BOOL(xiiMath::Pow2(2) == 4);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Pow")
  {
    XII_TEST_FLOAT(1.0f, xiiMath::Pow(3.0f, 0.0f), 0.000001f);
    XII_TEST_FLOAT(3.0f, xiiMath::Pow(3.0f, 1.0f), 0.000001f);
    XII_TEST_FLOAT(9.0f, xiiMath::Pow(3.0f, 2.0f), 0.000001f);

    XII_TEST_BOOL(xiiMath::Pow(3, 0) == 1);
    XII_TEST_BOOL(xiiMath::Pow(3, 1) == 3);
    XII_TEST_BOOL(xiiMath::Pow(3, 2) == 9);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Square")
  {
    XII_TEST_FLOAT(0.0f, xiiMath::Square(0.0f), 0.000001f);
    XII_TEST_FLOAT(1.0f, xiiMath::Square(1.0f), 0.000001f);
    XII_TEST_FLOAT(4.0f, xiiMath::Square(2.0f), 0.000001f);
    XII_TEST_FLOAT(4.0f, xiiMath::Square(-2.0f), 0.000001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Sqrt (float)")
  {
    XII_TEST_FLOAT(0.0f, xiiMath::Sqrt(0.0f), 0.000001f);
    XII_TEST_FLOAT(1.0f, xiiMath::Sqrt(1.0f), 0.000001f);
    XII_TEST_FLOAT(2.0f, xiiMath::Sqrt(4.0f), 0.000001f);
    XII_TEST_FLOAT(4.0f, xiiMath::Sqrt(16.0f), 0.000001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Sqrt (double)")
  {
    XII_TEST_DOUBLE(0.0, xiiMath::Sqrt(0.0), 0.000001);
    XII_TEST_DOUBLE(1.0, xiiMath::Sqrt(1.0), 0.000001);
    XII_TEST_DOUBLE(2.0, xiiMath::Sqrt(4.0), 0.000001);
    XII_TEST_DOUBLE(4.0, xiiMath::Sqrt(16.0), 0.000001);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Root")
  {
    XII_TEST_FLOAT(3.0f, xiiMath::Root(27.0f, 3.0f), 0.000001f);
    XII_TEST_FLOAT(3.0f, xiiMath::Root(81.0f, 4.0f), 0.000001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Sign")
  {
    XII_TEST_FLOAT(0.0f, xiiMath::Sign(0.0f), 0.00000001f);
    XII_TEST_FLOAT(1.0f, xiiMath::Sign(0.01f), 0.00000001f);
    XII_TEST_FLOAT(-1.0f, xiiMath::Sign(-0.01f), 0.00000001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Abs")
  {
    XII_TEST_FLOAT(0.0f, xiiMath::Abs(0.0f), 0.00000001f);
    XII_TEST_FLOAT(20.0f, xiiMath::Abs(20.0f), 0.00000001f);
    XII_TEST_FLOAT(20.0f, xiiMath::Abs(-20.0f), 0.00000001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Min")
  {
    XII_TEST_FLOAT(0.0f, xiiMath::Min(0.0f, 23.0f), 0.00000001f);
    XII_TEST_FLOAT(-23.0f, xiiMath::Min(0.0f, -23.0f), 0.00000001f);

    XII_TEST_BOOL(xiiMath::Min(1, 2, 3) == 1);
    XII_TEST_BOOL(xiiMath::Min(4, 2, 3) == 2);
    XII_TEST_BOOL(xiiMath::Min(4, 5, 3) == 3);

    XII_TEST_BOOL(xiiMath::Min(1, 2, 3, 4) == 1);
    XII_TEST_BOOL(xiiMath::Min(5, 2, 3, 4) == 2);
    XII_TEST_BOOL(xiiMath::Min(5, 6, 3, 4) == 3);
    XII_TEST_BOOL(xiiMath::Min(5, 6, 7, 4) == 4);

    XII_TEST_BOOL(xiiMath::Min(UniqueInt(1, 0), UniqueInt(1, 1)).id == 0);
    XII_TEST_BOOL(xiiMath::Min(UniqueInt(1, 1), UniqueInt(1, 0)).id == 1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Max")
  {
    XII_TEST_FLOAT(23.0f, xiiMath::Max(0.0f, 23.0f), 0.00000001f);
    XII_TEST_FLOAT(0.0f, xiiMath::Max(0.0f, -23.0f), 0.00000001f);

    XII_TEST_BOOL(xiiMath::Max(1, 2, 3) == 3);
    XII_TEST_BOOL(xiiMath::Max(1, 2, 0) == 2);
    XII_TEST_BOOL(xiiMath::Max(1, 0, 0) == 1);

    XII_TEST_BOOL(xiiMath::Max(1, 2, 3, 4) == 4);
    XII_TEST_BOOL(xiiMath::Max(1, 2, 3, 0) == 3);
    XII_TEST_BOOL(xiiMath::Max(1, 2, 0, 0) == 2);
    XII_TEST_BOOL(xiiMath::Max(1, 0, 0, 0) == 1);

    XII_TEST_BOOL(xiiMath::Max(UniqueInt(1, 0), UniqueInt(1, 1)).id == 0);
    XII_TEST_BOOL(xiiMath::Max(UniqueInt(1, 1), UniqueInt(1, 0)).id == 1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Clamp")
  {
    XII_TEST_FLOAT(15.0f, xiiMath::Clamp(23.0f, 12.0f, 15.0f), 0.00000001f);
    XII_TEST_FLOAT(12.0f, xiiMath::Clamp(3.0f, 12.0f, 15.0f), 0.00000001f);
    XII_TEST_FLOAT(14.0f, xiiMath::Clamp(14.0f, 12.0f, 15.0f), 0.00000001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Saturate")
  {
    XII_TEST_FLOAT(0.0f, xiiMath::Saturate(-1.5f), 0.00000001f);
    XII_TEST_FLOAT(0.5f, xiiMath::Saturate(0.5f), 0.00000001f);
    XII_TEST_FLOAT(1.0f, xiiMath::Saturate(12345.0f), 0.00000001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Floor")
  {
    XII_TEST_BOOL(12 == xiiMath::Floor(12.34f));
    XII_TEST_BOOL(-13 == xiiMath::Floor(-12.34f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Ceil")
  {
    XII_TEST_BOOL(13 == xiiMath::Ceil(12.34f));
    XII_TEST_BOOL(-12 == xiiMath::Ceil(-12.34f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RoundDown (float)")
  {
    XII_TEST_FLOAT(10.0f, xiiMath::RoundDown(12.34f, 5.0f), 0.0000001f);
    XII_TEST_FLOAT(-15.0f, xiiMath::RoundDown(-12.34f, 5.0f), 0.0000001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RoundUp (float)")
  {
    XII_TEST_FLOAT(15.0f, xiiMath::RoundUp(12.34f, 5.0f), 0.0000001f);
    XII_TEST_FLOAT(-10.0f, xiiMath::RoundUp(-12.34f, 5.0f), 0.0000001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RoundDown (double)")
  {
    XII_TEST_DOUBLE(10.0, xiiMath::RoundDown(12.34, 5.0), 0.0000001);
    XII_TEST_DOUBLE(-15.0, xiiMath::RoundDown(-12.34, 5.0), 0.0000001);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RoundUp (double)")
  {
    XII_TEST_DOUBLE(15.0, xiiMath::RoundUp(12.34, 5.0), 0.0000001);
    XII_TEST_DOUBLE(-10.0, xiiMath::RoundUp(-12.34, 5.0), 0.0000001);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Trunc")
  {
    XII_TEST_BOOL(xiiMath::Trunc(12.34f) == 12);
    XII_TEST_BOOL(xiiMath::Trunc(-12.34f) == -12);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FloatToInt")
  {
    XII_TEST_BOOL(xiiMath::FloatToInt(12.34f) == 12);
    XII_TEST_BOOL(xiiMath::FloatToInt(-12.34f) == -12);

#if XII_DISABLED(XII_PLATFORM_ARCH_X86) || (_MSC_VER <= 1916)
    XII_TEST_BOOL(xiiMath::FloatToInt(12000000000000.34) == 12000000000000);
    XII_TEST_BOOL(xiiMath::FloatToInt(-12000000000000.34) == -12000000000000);
#endif
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Round")
  {
    XII_TEST_BOOL(xiiMath::Round(12.34f) == 12);
    XII_TEST_BOOL(xiiMath::Round(-12.34f) == -12);

    XII_TEST_BOOL(xiiMath::Round(12.54f) == 13);
    XII_TEST_BOOL(xiiMath::Round(-12.54f) == -13);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RoundClosest (float)")
  {
    XII_TEST_FLOAT(xiiMath::RoundToMultiple(12.0f, 3.0f), 12.0f, 0.00001f);
    XII_TEST_FLOAT(xiiMath::RoundToMultiple(-12.0f, 3.0f), -12.0f, 0.00001f);
    XII_TEST_FLOAT(xiiMath::RoundToMultiple(12.34f, 7.0f), 14.0f, 0.00001f);
    XII_TEST_FLOAT(xiiMath::RoundToMultiple(-12.34f, 7.0f), -14.0f, 0.00001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RoundClosest (double)")
  {
    XII_TEST_DOUBLE(xiiMath::RoundToMultiple(12.0, 3.0), 12.0, 0.00001);
    XII_TEST_DOUBLE(xiiMath::RoundToMultiple(-12.0, 3.0), -12.0, 0.00001);
    XII_TEST_DOUBLE(xiiMath::RoundToMultiple(12.34, 7.0), 14.0, 0.00001);
    XII_TEST_DOUBLE(xiiMath::RoundToMultiple(-12.34, 7.0), -14.0, 0.00001);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RoundUp (int)")
  {
    XII_TEST_INT(xiiMath::RoundUp(12, 7), 14);
    XII_TEST_INT(xiiMath::RoundUp(-12, 7), -7);
    XII_TEST_INT(xiiMath::RoundUp(16, 4), 16);
    XII_TEST_INT(xiiMath::RoundUp(-16, 4), -16);
    XII_TEST_INT(xiiMath::RoundUp(17, 4), 20);
    XII_TEST_INT(xiiMath::RoundUp(-17, 4), -16);
    XII_TEST_INT(xiiMath::RoundUp(15, 4), 16);
    XII_TEST_INT(xiiMath::RoundUp(-15, 4), -12);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RoundDown (int)")
  {
    XII_TEST_INT(xiiMath::RoundDown(12, 7), 7);
    XII_TEST_INT(xiiMath::RoundDown(-12, 7), -14);
    XII_TEST_INT(xiiMath::RoundDown(16, 4), 16);
    XII_TEST_INT(xiiMath::RoundDown(-16, 4), -16);
    XII_TEST_INT(xiiMath::RoundDown(17, 4), 16);
    XII_TEST_INT(xiiMath::RoundDown(-17, 4), -20);
    XII_TEST_INT(xiiMath::RoundDown(15, 4), 12);
    XII_TEST_INT(xiiMath::RoundDown(-15, 4), -16);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RoundUp (unsigned int)")
  {
    XII_TEST_INT(xiiMath::RoundUp(12u, 7), 14);
    XII_TEST_INT(xiiMath::RoundUp(16u, 4), 16);
    XII_TEST_INT(xiiMath::RoundUp(17u, 4), 20);
    XII_TEST_INT(xiiMath::RoundUp(15u, 4), 16);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RoundDown (unsigned int)")
  {
    XII_TEST_INT(xiiMath::RoundDown(12u, 7), 7);
    XII_TEST_INT(xiiMath::RoundDown(16u, 4), 16);
    XII_TEST_INT(xiiMath::RoundDown(17u, 4), 16);
    XII_TEST_INT(xiiMath::RoundDown(15u, 4), 12);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Fraction")
  {
    XII_TEST_FLOAT(xiiMath::Fraction(12.34f), 0.34f, 0.00001f);
    XII_TEST_FLOAT(xiiMath::Fraction(-12.34f), -0.34f, 0.00001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Mod (float)")
  {
    XII_TEST_FLOAT(2.34f, xiiMath::Mod(12.34f, 2.5f), 0.000001f);
    XII_TEST_FLOAT(-2.34f, xiiMath::Mod(-12.34f, 2.5f), 0.000001f);

    XII_TEST_FLOAT(2.34f, xiiMath::Mod(12.34f, -2.5f), 0.000001f);
    XII_TEST_FLOAT(-2.34f, xiiMath::Mod(-12.34f, -2.5f), 0.000001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Mod (double)")
  {
    XII_TEST_DOUBLE(2.34, xiiMath::Mod(12.34, 2.5), 0.000001);
    XII_TEST_DOUBLE(-2.34, xiiMath::Mod(-12.34, 2.5), 0.000001);

    XII_TEST_DOUBLE(2.34, xiiMath::Mod(12.34, -2.5), 0.000001);
    XII_TEST_DOUBLE(-2.34, xiiMath::Mod(-12.34, -2.5), 0.000001);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Invert")
  {
    XII_TEST_FLOAT(xiiMath::Invert(1.0f), 1.0f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::Invert(2.0f), 0.5f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::Invert(4.0f), 0.25f, 0.000001f);

    XII_TEST_FLOAT(xiiMath::Invert(-1.0f), -1.0f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::Invert(-2.0f), -0.5f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::Invert(-4.0f), -0.25f, 0.000001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Odd")
  {
    XII_TEST_BOOL(xiiMath::IsOdd(0) == false);
    XII_TEST_BOOL(xiiMath::IsOdd(1) == true);
    XII_TEST_BOOL(xiiMath::IsOdd(2) == false);
    XII_TEST_BOOL(xiiMath::IsOdd(-1) == true);
    XII_TEST_BOOL(xiiMath::IsOdd(-2) == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Even")
  {
    XII_TEST_BOOL(xiiMath::IsEven(0) == true);
    XII_TEST_BOOL(xiiMath::IsEven(1) == false);
    XII_TEST_BOOL(xiiMath::IsEven(2) == true);
    XII_TEST_BOOL(xiiMath::IsEven(-1) == false);
    XII_TEST_BOOL(xiiMath::IsEven(-2) == true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Swap")
  {
    xiiInt32 a = 1;
    xiiInt32 b = 2;
    xiiMath::Swap(a, b);
    XII_TEST_BOOL((a == 2) && (b == 1));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Lerp")
  {
    XII_TEST_FLOAT(xiiMath::Lerp(-5.0f, 5.0f, 0.5f), 0.0f, 0.000001);
    XII_TEST_FLOAT(xiiMath::Lerp(0.0f, 5.0f, 0.5f), 2.5f, 0.000001);
    XII_TEST_FLOAT(xiiMath::Lerp(-5.0f, 5.0f, 0.0f), -5.0f, 0.000001);
    XII_TEST_FLOAT(xiiMath::Lerp(-5.0f, 5.0f, 1.0f), 5.0f, 0.000001);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Step")
  {
    XII_TEST_FLOAT(xiiMath::Step(0.5f, 0.4f), 1.0f, 0.00001f);
    XII_TEST_FLOAT(xiiMath::Step(0.3f, 0.4f), 0.0f, 0.00001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SmoothStep")
  {
    // Only test values that must be true for any symmetric step function.
    // How should one test smoothness?
    for (int iScale = -19; iScale <= 19; iScale += 2)
    {
      XII_TEST_FLOAT(xiiMath::SmoothStep(0.0f * iScale, 0.1f * iScale, 0.4f * iScale), 0.0f, 0.000001);
      XII_TEST_FLOAT(xiiMath::SmoothStep(0.1f * iScale, 0.1f * iScale, 0.4f * iScale), 0.0f, 0.000001);
      XII_TEST_FLOAT(xiiMath::SmoothStep(0.4f * iScale, 0.1f * iScale, 0.4f * iScale), 1.0f, 0.000001);
      XII_TEST_FLOAT(xiiMath::SmoothStep(0.25f * iScale, 0.1f * iScale, 0.4f * iScale), 0.5f, 0.000001);
      XII_TEST_FLOAT(xiiMath::SmoothStep(0.5f * iScale, 0.1f * iScale, 0.4f * iScale), 1.0f, 0.000001);

      XII_TEST_FLOAT(xiiMath::SmoothStep(0.5f * iScale, 0.4f * iScale, 0.1f * iScale), 0.0f, 0.000001);
      XII_TEST_FLOAT(xiiMath::SmoothStep(0.4f * iScale, 0.4f * iScale, 0.1f * iScale), 0.0f, 0.000001);
      XII_TEST_FLOAT(xiiMath::SmoothStep(0.1f * iScale, 0.4f * iScale, 0.1f * iScale), 1.0f, 0.000001);
      XII_TEST_FLOAT(xiiMath::SmoothStep(0.25f * iScale, 0.1f * iScale, 0.4f * iScale), 0.5f, 0.000001);
      XII_TEST_FLOAT(xiiMath::SmoothStep(0.0f * iScale, 0.4f * iScale, 0.1f * iScale), 1.0f, 0.000001);

      // For edge1 == edge2 SmoothStep should behave like Step
      XII_TEST_FLOAT(xiiMath::SmoothStep(0.0f * iScale, 0.1f * iScale, 0.1f * iScale), iScale > 0 ? 0.0f : 1.0f, 0.000001);
      XII_TEST_FLOAT(xiiMath::SmoothStep(0.2f * iScale, 0.1f * iScale, 0.1f * iScale), iScale < 0 ? 0.0f : 1.0f, 0.000001);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsPowerOf")
  {
    XII_TEST_BOOL(xiiMath::IsPowerOf(4, 2) == true);
    XII_TEST_BOOL(xiiMath::IsPowerOf(5, 2) == false);
    XII_TEST_BOOL(xiiMath::IsPowerOf(0, 2) == false);
    XII_TEST_BOOL(xiiMath::IsPowerOf(1, 2) == true);

    XII_TEST_BOOL(xiiMath::IsPowerOf(4, 3) == false);
    XII_TEST_BOOL(xiiMath::IsPowerOf(3, 3) == true);
    XII_TEST_BOOL(xiiMath::IsPowerOf(1, 3) == true);
    XII_TEST_BOOL(xiiMath::IsPowerOf(27, 3) == true);
    XII_TEST_BOOL(xiiMath::IsPowerOf(28, 3) == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsPowerOf2")
  {
    XII_TEST_BOOL(xiiMath::IsPowerOf2(4) == true);
    XII_TEST_BOOL(xiiMath::IsPowerOf2(5) == false);
    XII_TEST_BOOL(xiiMath::IsPowerOf2(0) == false);
    XII_TEST_BOOL(xiiMath::IsPowerOf2(1) == true);
    XII_TEST_BOOL(xiiMath::IsPowerOf2(0x7FFFFFFFu) == false);
    XII_TEST_BOOL(xiiMath::IsPowerOf2(0x80000000u) == true);
    XII_TEST_BOOL(xiiMath::IsPowerOf2(0x80000001u) == false);
    XII_TEST_BOOL(xiiMath::IsPowerOf2(0xFFFFFFFFu) == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "PowerOf2_Floor")
  {
    XII_TEST_INT(xiiMath::PowerOfTwo_Floor(64), 64);
    XII_TEST_INT(xiiMath::PowerOfTwo_Floor(33), 32);
    XII_TEST_INT(xiiMath::PowerOfTwo_Floor(4), 4);
    XII_TEST_INT(xiiMath::PowerOfTwo_Floor(5), 4);
    XII_TEST_INT(xiiMath::PowerOfTwo_Floor(1), 1);
    XII_TEST_INT(xiiMath::PowerOfTwo_Floor(0x80000000), 0x80000000);
    XII_TEST_INT(xiiMath::PowerOfTwo_Floor(0x80000001), 0x80000000);
    // strange case...
    XII_TEST_INT(xiiMath::PowerOfTwo_Floor(0), 1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "PowerOf2_Ceil")
  {
    XII_TEST_INT(xiiMath::PowerOfTwo_Ceil(64), 64);
    XII_TEST_INT(xiiMath::PowerOfTwo_Ceil(33), 64);
    XII_TEST_INT(xiiMath::PowerOfTwo_Ceil(4), 4);
    XII_TEST_INT(xiiMath::PowerOfTwo_Ceil(5), 8);
    XII_TEST_INT(xiiMath::PowerOfTwo_Ceil(1), 1);
    XII_TEST_INT(xiiMath::PowerOfTwo_Ceil(0), 1);
    XII_TEST_INT(xiiMath::PowerOfTwo_Ceil(0x7FFFFFFF), 0x80000000);
    XII_TEST_INT(xiiMath::PowerOfTwo_Ceil(0x80000000), 0x80000000);
    // anything above 0x80000000 is undefined behavior due to how left-shift works
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GreatestCommonDivisor")
  {
    XII_TEST_INT(xiiMath::GreatestCommonDivisor(13, 13), 13);
    XII_TEST_INT(xiiMath::GreatestCommonDivisor(37, 600), 1);
    XII_TEST_INT(xiiMath::GreatestCommonDivisor(20, 100), 20);
    XII_TEST_INT(xiiMath::GreatestCommonDivisor(624129, 2061517), 18913);
  }


  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsEqual")
  {
    XII_TEST_BOOL(xiiMath::IsEqual(1.0f, 0.999f, 0.01f) == true);
    XII_TEST_BOOL(xiiMath::IsEqual(1.0f, 1.001f, 0.01f) == true);
    XII_TEST_BOOL(xiiMath::IsEqual(1.0f, 0.999f, 0.0001f) == false);
    XII_TEST_BOOL(xiiMath::IsEqual(1.0f, 1.001f, 0.0001f) == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "NaN_Infinity")
  {
    if (xiiMath::SupportsNaN<xiiMathTestType>())
    {
      XII_TEST_BOOL(xiiMath::IsNaN(xiiMath::NaN<xiiMathTestType>()) == true);

      XII_TEST_BOOL(xiiMath::Infinity<xiiMathTestType>() == xiiMath::Infinity<xiiMathTestType>() - (xiiMathTestType)1);
      XII_TEST_BOOL(xiiMath::Infinity<xiiMathTestType>() == xiiMath::Infinity<xiiMathTestType>() + (xiiMathTestType)1);

      XII_TEST_BOOL(xiiMath::IsNaN(xiiMath::Infinity<xiiMathTestType>() - xiiMath::Infinity<xiiMathTestType>()));

      XII_TEST_BOOL(!xiiMath::IsFinite(xiiMath::Infinity<xiiMathTestType>()));
      XII_TEST_BOOL(!xiiMath::IsFinite(-xiiMath::Infinity<xiiMathTestType>()));
      XII_TEST_BOOL(!xiiMath::IsFinite(xiiMath::NaN<xiiMathTestType>()));
      XII_TEST_BOOL(!xiiMath::IsNaN(xiiMath::Infinity<xiiMathTestType>()));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsInRange")
  {
    XII_TEST_BOOL(xiiMath::IsInRange(1.0f, 0.0f, 2.0f) == true);
    XII_TEST_BOOL(xiiMath::IsInRange(1.0f, 0.0f, 1.0f) == true);
    XII_TEST_BOOL(xiiMath::IsInRange(1.0f, 1.0f, 2.0f) == true);
    XII_TEST_BOOL(xiiMath::IsInRange(0.0f, 1.0f, 2.0f) == false);
    XII_TEST_BOOL(xiiMath::IsInRange(3.0f, 0.0f, 2.0f) == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsZero")
  {
    XII_TEST_BOOL(xiiMath::IsZero(0.009f, 0.01f) == true);
    XII_TEST_BOOL(xiiMath::IsZero(0.001f, 0.01f) == true);
    XII_TEST_BOOL(xiiMath::IsZero(0.009f, 0.0001f) == false);
    XII_TEST_BOOL(xiiMath::IsZero(0.001f, 0.0001f) == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ColorFloatToByte")
  {
    XII_TEST_INT(xiiMath::ColorFloatToByte(xiiMath::NaN<float>()), 0);
    XII_TEST_INT(xiiMath::ColorFloatToByte(-1.0f), 0);
    XII_TEST_INT(xiiMath::ColorFloatToByte(0.0f), 0);
    XII_TEST_INT(xiiMath::ColorFloatToByte(0.4f), 102);
    XII_TEST_INT(xiiMath::ColorFloatToByte(1.0f), 255);
    XII_TEST_INT(xiiMath::ColorFloatToByte(1.5f), 255);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ColorFloatToShort")
  {
    XII_TEST_INT(xiiMath::ColorFloatToShort(xiiMath::NaN<float>()), 0);
    XII_TEST_INT(xiiMath::ColorFloatToShort(-1.0f), 0);
    XII_TEST_INT(xiiMath::ColorFloatToShort(0.0f), 0);
    XII_TEST_INT(xiiMath::ColorFloatToShort(0.4f), 26214);
    XII_TEST_INT(xiiMath::ColorFloatToShort(1.0f), 65535);
    XII_TEST_INT(xiiMath::ColorFloatToShort(1.5f), 65535);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ColorFloatToSignedByte")
  {
    XII_TEST_INT(xiiMath::ColorFloatToSignedByte(xiiMath::NaN<float>()), 0);
    XII_TEST_INT(xiiMath::ColorFloatToSignedByte(-1.0f), -127);
    XII_TEST_INT(xiiMath::ColorFloatToSignedByte(0.0f), 0);
    XII_TEST_INT(xiiMath::ColorFloatToSignedByte(0.4f), 51);
    XII_TEST_INT(xiiMath::ColorFloatToSignedByte(1.0f), 127);
    XII_TEST_INT(xiiMath::ColorFloatToSignedByte(1.5f), 127);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ColorFloatToSignedShort")
  {
    XII_TEST_INT(xiiMath::ColorFloatToSignedShort(xiiMath::NaN<float>()), 0);
    XII_TEST_INT(xiiMath::ColorFloatToSignedShort(-1.0f), -32767);
    XII_TEST_INT(xiiMath::ColorFloatToSignedShort(0.0f), 0);
    XII_TEST_INT(xiiMath::ColorFloatToSignedShort(0.4f), 13107);
    XII_TEST_INT(xiiMath::ColorFloatToSignedShort(0.5f), 16384);
    XII_TEST_INT(xiiMath::ColorFloatToSignedShort(1.0f), 32767);
    XII_TEST_INT(xiiMath::ColorFloatToSignedShort(1.5f), 32767);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ColorByteToFloat")
  {
    XII_TEST_FLOAT(xiiMath::ColorByteToFloat(0), 0.0f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::ColorByteToFloat(128), 0.501960784f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::ColorByteToFloat(255), 1.0f, 0.000001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ColorShortToFloat")
  {
    XII_TEST_FLOAT(xiiMath::ColorShortToFloat(0), 0.0f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::ColorShortToFloat(32768), 0.5000076f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::ColorShortToFloat(65535), 1.0f, 0.000001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ColorSignedByteToFloat")
  {
    XII_TEST_FLOAT(xiiMath::ColorSignedByteToFloat(-128), -1.0f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::ColorSignedByteToFloat(-127), -1.0f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::ColorSignedByteToFloat(0), 0.0f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::ColorSignedByteToFloat(64), 0.50393700787f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::ColorSignedByteToFloat(127), 1.0f, 0.000001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ColorSignedShortToFloat")
  {
    XII_TEST_FLOAT(xiiMath::ColorSignedShortToFloat(-32768), -1.0f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::ColorSignedShortToFloat(-32767), -1.0f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::ColorSignedShortToFloat(0), 0.0f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::ColorSignedShortToFloat(16384), 0.50001526f, 0.000001f);
    XII_TEST_FLOAT(xiiMath::ColorSignedShortToFloat(32767), 1.0f, 0.000001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "EvaluateBxiiierCurve")
  {
    // Determined through the scientific method of manually comparing the result of the function with an online Bxiiier curve generator:
    // https://www.desmos.com/calculator/cahqdxeshd
    const xiiVec2 res[] = {xiiVec2(1, 5), xiiVec2(0.893f, 4.455f), xiiVec2(1.112f, 4.008f), xiiVec2(1.557f, 3.631f), xiiVec2(2.136f, 3.304f), xiiVec2(2.750f, 3.000f),
                           xiiVec2(3.303f, 2.695f), xiiVec2(3.701f, 2.368f), xiiVec2(3.847f, 1.991f), xiiVec2(3.645f, 1.543f), xiiVec2(3, 1)};

    const float step = 1.0f / (XII_ARRAY_SIZE(res) - 1);
    for (int i = 0; i < XII_ARRAY_SIZE(res); ++i)
    {
      const xiiVec2 r = xiiMath::EvaluateBxiiierCurve<xiiVec2>(step * i, xiiVec2(1, 5), xiiVec2(0, 3), xiiVec2(6, 3), xiiVec2(3, 1));
      XII_TEST_VEC2(r, res[i], 0.002f);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FirstBitLow")
  {
    XII_TEST_INT(xiiMath::FirstBitLow(xiiUInt32(0b1111)), 0);
    XII_TEST_INT(xiiMath::FirstBitLow(xiiUInt32(0b1110)), 1);
    XII_TEST_INT(xiiMath::FirstBitLow(xiiUInt32(0b1100)), 2);
    XII_TEST_INT(xiiMath::FirstBitLow(xiiUInt32(0b1000)), 3);
    XII_TEST_INT(xiiMath::FirstBitLow(xiiUInt32(0xFFFFFFFF)), 0);

    XII_TEST_INT(xiiMath::FirstBitLow(xiiUInt64(0xFF000000FF00000F)), 0);
    XII_TEST_INT(xiiMath::FirstBitLow(xiiUInt64(0xFF000000FF00000E)), 1);
    XII_TEST_INT(xiiMath::FirstBitLow(xiiUInt64(0xFF000000FF00000C)), 2);
    XII_TEST_INT(xiiMath::FirstBitLow(xiiUInt64(0xFF000000FF000008)), 3);
    XII_TEST_INT(xiiMath::FirstBitLow(xiiUInt64(0xFFFFFFFFFFFFFFFF)), 0);

    // Edge cases specifically for 32-bit systems where upper and lower 32-bit are handled individually.
    XII_TEST_INT(xiiMath::FirstBitLow(xiiUInt64(0x00000000FFFFFFFF)), 0);
    XII_TEST_INT(xiiMath::FirstBitLow(xiiUInt64(0xFFFFFFFF00000000)), 32);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FirstBitHigh")
  {
    XII_TEST_INT(xiiMath::FirstBitHigh(xiiUInt32(0b1111)), 3);
    XII_TEST_INT(xiiMath::FirstBitHigh(xiiUInt32(0b0111)), 2);
    XII_TEST_INT(xiiMath::FirstBitHigh(xiiUInt32(0b0011)), 1);
    XII_TEST_INT(xiiMath::FirstBitHigh(xiiUInt32(0b0001)), 0);
    XII_TEST_INT(xiiMath::FirstBitHigh(xiiUInt32(0xFFFFFFFF)), 31);

    XII_TEST_INT(xiiMath::FirstBitHigh(xiiUInt64(0x00FF000000FF000F)), 55);
    XII_TEST_INT(xiiMath::FirstBitHigh(xiiUInt64(0x007F000000FF000F)), 54);
    XII_TEST_INT(xiiMath::FirstBitHigh(xiiUInt64(0x003F000000FF000F)), 53);
    XII_TEST_INT(xiiMath::FirstBitHigh(xiiUInt64(0x001F000000FF000F)), 52);
    XII_TEST_INT(xiiMath::FirstBitHigh(xiiUInt64(0xFFFFFFFFFFFFFFFF)), 63);

    // Edge cases specifically for 32-bit systems where upper and lower 32-bit are handled individually.
    XII_TEST_INT(xiiMath::FirstBitHigh(xiiUInt64(0x00000000FFFFFFFF)), 31);
    XII_TEST_INT(xiiMath::FirstBitHigh(xiiUInt64(0xFFFFFFFF00000000)), 63);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CountTrailingZeros (32)")
  {
    XII_TEST_INT(xiiMath::CountTrailingZeros(0b1111u), 0);
    XII_TEST_INT(xiiMath::CountTrailingZeros(0b1110u), 1);
    XII_TEST_INT(xiiMath::CountTrailingZeros(0b1100u), 2);
    XII_TEST_INT(xiiMath::CountTrailingZeros(0b1000u), 3);
    XII_TEST_INT(xiiMath::CountTrailingZeros(0xFFFFFFFF), 0);
    XII_TEST_INT(xiiMath::CountTrailingZeros(0u), 32);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CountTrailingZeros (64)")
  {
    XII_TEST_INT(xiiMath::CountTrailingZeros(0b1111llu), 0);
    XII_TEST_INT(xiiMath::CountTrailingZeros(0b1110llu), 1);
    XII_TEST_INT(xiiMath::CountTrailingZeros(0b1100llu), 2);
    XII_TEST_INT(xiiMath::CountTrailingZeros(0b1000llu), 3);
    XII_TEST_INT(xiiMath::CountTrailingZeros(0xFFFFFFFF0llu), 4);
    XII_TEST_INT(xiiMath::CountTrailingZeros(0llu), 64);
    XII_TEST_INT(xiiMath::CountTrailingZeros(0xFFFFFFFF00llu), 8);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CountLeadingZeros")
  {
    XII_TEST_INT(xiiMath::CountLeadingZeros(0b1111), 28);
    XII_TEST_INT(xiiMath::CountLeadingZeros(0b0111), 29);
    XII_TEST_INT(xiiMath::CountLeadingZeros(0b0011), 30);
    XII_TEST_INT(xiiMath::CountLeadingZeros(0b0001), 31);
    XII_TEST_INT(xiiMath::CountLeadingZeros(0xFFFFFFFF), 0);
    XII_TEST_INT(xiiMath::CountLeadingZeros(0), 32);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TryMultiply32")
  {
    xiiUInt32 res;

    res = 0;
    XII_TEST_BOOL(xiiMath::TryMultiply32(res, 1, 1, 2, 3).Succeeded());
    XII_TEST_INT(res, 6);

    res = 0;
    XII_TEST_BOOL(xiiMath::TryMultiply32(res, 1, 1, 1, 0xFFFFFFFF).Succeeded());
    XII_TEST_BOOL(res == 0xFFFFFFFF);

    res = 0;
    XII_TEST_BOOL(xiiMath::TryMultiply32(res, 0xFFFF, 0x10001).Succeeded());
    XII_TEST_BOOL(res == 0xFFFFFFFF);

    res = 0;
    XII_TEST_BOOL(xiiMath::TryMultiply32(res, 0x3FFFFFF, 2, 4, 8).Succeeded());
    XII_TEST_BOOL(res == 0xFFFFFFC0);

    res = 1;
    XII_TEST_BOOL(xiiMath::TryMultiply32(res, 0xFFFFFFFF, 2).Failed());
    XII_TEST_BOOL(res == 1);

    res = 0;
    XII_TEST_BOOL(xiiMath::TryMultiply32(res, 0x80000000, 2).Failed()); // slightly above 0xFFFFFFFF
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TryMultiply64")
  {
    xiiUInt64 res;

    res = 0;
    XII_TEST_BOOL(xiiMath::TryMultiply64(res, 1, 1, 2, 3).Succeeded());
    XII_TEST_INT(res, 6);

    res = 0;
    XII_TEST_BOOL(xiiMath::TryMultiply64(res, 1, 1, 1, 0xFFFFFFFF).Succeeded());
    XII_TEST_BOOL(res == 0xFFFFFFFF);

    res = 0;
    XII_TEST_BOOL(xiiMath::TryMultiply64(res, 0xFFFF, 0x10001).Succeeded());
    XII_TEST_BOOL(res == 0xFFFFFFFF);

    res = 0;
    XII_TEST_BOOL(xiiMath::TryMultiply64(res, 0x3FFFFFF, 2, 4, 8).Succeeded());
    XII_TEST_BOOL(res == 0xFFFFFFC0);

    res = 0;
    XII_TEST_BOOL(xiiMath::TryMultiply64(res, 0xFFFFFFFF, 2).Succeeded());
    XII_TEST_BOOL(res == 0x1FFFFFFFE);

    res = 0;
    XII_TEST_BOOL(xiiMath::TryMultiply64(res, 0x80000000, 2).Succeeded());
    XII_TEST_BOOL(res == 0x100000000);

    res = 0;
    XII_TEST_BOOL(xiiMath::TryMultiply64(res, 0xFFFFFFFF, 0xFFFFFFFF).Succeeded());
    XII_TEST_BOOL(res == 0xFFFFFFFE00000001);

    res = 0;
    XII_TEST_BOOL(xiiMath::TryMultiply64(res, 0xFFFFFFFFFFFFFFFF, 2).Failed());

    res = 0;
    XII_TEST_BOOL(xiiMath::TryMultiply64(res, 0xFFFFFFFF, 0xFFFFFFFF, 2).Failed());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TryConvertToSizeT")
  {
    xiiUInt64 x = xiiMath::MaxValue<xiiUInt32>();
    xiiUInt64 y = x + 1;

    size_t res = 0;

    XII_TEST_BOOL(xiiMath::TryConvertToSizeT(res, x).Succeeded());
    XII_TEST_BOOL(res == x);

    res = 0;
#if XII_ENABLED(XII_PLATFORM_32BIT)
    XII_TEST_BOOL(xiiMath::TryConvertToSizeT(res, y).Failed());
#else
    XII_TEST_BOOL(xiiMath::TryConvertToSizeT(res, y).Succeeded());
    XII_TEST_BOOL(res == y);
#endif
  }
}
