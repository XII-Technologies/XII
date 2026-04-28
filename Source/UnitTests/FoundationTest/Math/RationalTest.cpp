/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Rational.h>
#include <Foundation/Strings/StringBuilder.h>

XII_CREATE_SIMPLE_TEST(Math, Rational)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Rational")
  {
    xiiRational r1(100, 1);

    XII_TEST_BOOL(r1.IsValid());
    XII_TEST_BOOL(r1.IsIntegral());

    xiiRational r2(100, 0);
    XII_TEST_BOOL(!r2.IsValid());

    XII_TEST_BOOL(r1 != r2);

    xiiRational r3(100, 1);
    XII_TEST_BOOL(r3 == r1);

    xiiRational r4(0, 0);
    XII_TEST_BOOL(r4.IsValid());


    xiiRational r5(30, 6);
    XII_TEST_BOOL(r5.IsIntegral());
    XII_TEST_INT(r5.GetIntegralResult(), 5);
    XII_TEST_FLOAT(r5.GetFloatingPointResult(), 5, xiiMath::SmallEpsilon<double>());

    xiiRational reducedTest(5, 1);
    XII_TEST_BOOL(r5.ReduceIntegralFraction() == reducedTest);

    xiiRational r6(31, 6);
    XII_TEST_BOOL(!r6.IsIntegral());
    XII_TEST_FLOAT(r6.GetFloatingPointResult(), 5.16666666666, xiiMath::SmallEpsilon<double>());


    XII_TEST_INT(r6.GetDenominator(), 6);
    XII_TEST_INT(r6.GetNumerator(), 31);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Rational String Formatting")
  {
    xiiRational r1(50, 25);

    xiiStringBuilder sb;
    sb.SetFormat("Rational: {}", r1);
    XII_TEST_STRING(sb, "Rational: 2");


    xiiRational r2(233, 76);
    sb.SetFormat("Rational: {}", r2);
    XII_TEST_STRING(sb, "Rational: 233/76");
  }
}
