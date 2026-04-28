/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/FixedPoint.h>

XII_CREATE_SIMPLE_TEST(Math, FixedPoint)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor (int) / Conversion to Int")
  {
    // positive values
    for (xiiInt32 i = 0; i < 1024; ++i)
    {
      xiiFixedPoint<12> fp(i);
      XII_TEST_INT(fp.ToInt(), i);
    }

    // negative values
    for (xiiInt32 i = 0; i < 1024; ++i)
    {
      xiiFixedPoint<12> fp(-i);
      XII_TEST_INT(fp.ToInt(), -i);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor (float) / Conversion to Float")
  {
    // positive values
    for (float f = 0.0f; f < 100.0f; f += 0.01f)
    {
      xiiFixedPoint<12> fp(f);

      XII_TEST_FLOAT(fp, f, 0.001f);
    }

    // negative values
    for (float f = 0.0f; f < 100.0f; f += 0.01f)
    {
      xiiFixedPoint<12> fp(-f);

      XII_TEST_FLOAT(fp, -f, 0.001f);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor (double) / Conversion to double")
  {
    // positive values
    for (double f = 0.0; f < 100.0; f += 0.01)
    {
      xiiFixedPoint<12> fp(f);

      XII_TEST_DOUBLE(fp.ToDouble(), f, 0.001);
    }

    // negative values
    for (double f = 0.0; f < 100.0f; f += 0.01)
    {
      xiiFixedPoint<12> fp(-f);

      XII_TEST_DOUBLE(fp.ToDouble(), -f, 0.001);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor (Other) / Assignment")
  {
    xiiFixedPoint<12> fp1(2.4f);
    xiiFixedPoint<12> fp2(fp1);
    xiiFixedPoint<12> fp3;

    fp3 = fp1;

    XII_TEST_BOOL(fp1 == fp1);
    XII_TEST_BOOL(fp2 == fp2);
    XII_TEST_BOOL(fp3 == fp3);

    XII_TEST_BOOL(fp1 == fp2);
    XII_TEST_BOOL(fp1 == fp3);
    XII_TEST_BOOL(fp2 == fp3);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Max Value")
  {
    xiiFixedPoint<12> fp1((1 << 19) - 1);
    xiiFixedPoint<12> fp2((1 << 19));
    xiiFixedPoint<12> fp3(-(1 << 19)); // one more value available in the negative range
    xiiFixedPoint<12> fp4(-(1 << 19) - 1);

    // 12 Bits for the fraction -> 19 Bits for the integral part and 1 'Sign Bit'
    XII_TEST_BOOL(fp1.ToInt() == (1 << 19) - 1); // This maximum value is still representable
    XII_TEST_BOOL(fp2.ToInt() != (1 << 19));     // The next value isn't representable anymore
    XII_TEST_BOOL(fp3.ToInt() == -(1 << 19));
    XII_TEST_BOOL(fp4.ToInt() != -(1 << 19) - 1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator*(fp, int)")
  {
    xiiFixedPoint<12> fp(3.2f);
    fp = fp * 2;

    XII_TEST_FLOAT(fp.ToFloat(), 6.4f, 0.001f);

    fp = 3 * fp;

    XII_TEST_FLOAT(fp.ToFloat(), 19.2f, 0.001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator/(fp, int)")
  {
    xiiFixedPoint<12> fp(12.4f);
    fp = fp / 2;

    XII_TEST_FLOAT(fp.ToFloat(), 6.2f, 0.001f);

    fp = fp / 3;

    XII_TEST_FLOAT(fp.ToFloat(), 2.066f, 0.001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator+(fp, fp)")
  {
    xiiFixedPoint<12> fp(3.2f);
    fp = fp + xiiFixedPoint<12>(2);

    XII_TEST_FLOAT(fp.ToFloat(), 5.2f, 0.001f);

    fp = xiiFixedPoint<12>(3) + fp;

    XII_TEST_FLOAT(fp.ToFloat(), 8.2f, 0.001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator-(fp, fp)")
  {
    xiiFixedPoint<12> fp(3.2f);
    fp = fp - xiiFixedPoint<12>(2);

    XII_TEST_FLOAT(fp.ToFloat(), 1.2f, 0.001f);

    fp = xiiFixedPoint<12>(3) - fp;

    XII_TEST_FLOAT(fp.ToFloat(), 1.8f, 0.001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator*(fp, fp)")
  {
    xiiFixedPoint<12> fp(3.2f);

    fp = fp * xiiFixedPoint<12>(2.5f);
    XII_TEST_FLOAT(fp.ToFloat(), 8.0f, 0.001f);

    fp = fp * xiiFixedPoint<12>(-123.456f);
    XII_TEST_FLOAT(fp.ToFloat(), -987.648f, 0.1f);
  }

  // Disabled because MSVC 2017 has code generation issues in Release builds
  XII_TEST_BLOCK(xiiTestBlock::Disabled, "operator/(fp, fp)")
  {
    xiiFixedPoint<12> fp(100000.248f);

    fp = fp / xiiFixedPoint<12>(-2);
    XII_TEST_FLOAT(fp.ToFloat(), -50000.124f, 0.001f);

    fp = fp / xiiFixedPoint<12>(-4);
    XII_TEST_FLOAT(fp.ToFloat(), 12500.031f, 0.001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Operator<,>,<=,>=,==,!=")
  {
    xiiFixedPoint<12> fp1(1);
    xiiFixedPoint<12> fp2(2.0f);
    xiiFixedPoint<12> fp3(3);
    xiiFixedPoint<12> fp3b(3.0f);

    XII_TEST_BOOL(fp1 < fp2);
    XII_TEST_BOOL(fp3 > fp2);
    XII_TEST_BOOL(fp3 <= fp3b);
    XII_TEST_BOOL(fp3 >= fp3b);
    XII_TEST_BOOL(fp1 != fp2);
    XII_TEST_BOOL(fp3 == fp3b);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Assignment Rounding")
  {
    xiiFixedPoint<2> fp; // 2 Bits -> 4 fractional values

    fp = 1000.25f;
    XII_TEST_FLOAT(fp, 1000.25f, 0.01f);

    fp = 1000.75f;
    XII_TEST_FLOAT(fp, 1000.75f, 0.01f);



    fp = 1000.1f;
    XII_TEST_DOUBLE(fp.ToDouble(), 1000.0, 0.01);

    fp = 1000.2f;
    XII_TEST_DOUBLE(fp.ToDouble(), 1000.25, 0.01);

    fp = 1000.3f;
    XII_TEST_DOUBLE(fp.ToDouble(), 1000.25, 0.01);

    fp = 1000.4f;
    XII_TEST_DOUBLE(fp.ToDouble(), 1000.5, 0.01);

    fp = 1000.5f;
    XII_TEST_DOUBLE(fp.ToDouble(), 1000.5, 0.01);

    fp = 1000.6f;
    XII_TEST_DOUBLE(fp.ToDouble(), 1000.5, 0.01);

    fp = 1000.7f;
    XII_TEST_DOUBLE(fp.ToDouble(), 1000.75, 0.01);

    fp = 1000.8f;
    XII_TEST_DOUBLE(fp.ToDouble(), 1000.75, 0.01);

    fp = 1000.9f;
    XII_TEST_DOUBLE(fp.ToDouble(), 1001.0, 0.01);


    // negative
    fp = -1000.1;
    XII_TEST_FLOAT(fp.ToFloat(), -1000.0f, 0.01f);

    fp = -1000.2;
    XII_TEST_FLOAT(fp.ToFloat(), -1000.25f, 0.01f);

    fp = -1000.3;
    XII_TEST_FLOAT(fp.ToFloat(), -1000.25f, 0.01f);

    fp = -1000.4;
    XII_TEST_FLOAT(fp.ToFloat(), -1000.5f, 0.01f);

    fp = -1000.5;
    XII_TEST_FLOAT(fp.ToFloat(), -1000.5f, 0.01f);

    fp = -1000.6;
    XII_TEST_FLOAT(fp.ToFloat(), -1000.5f, 0.01f);

    fp = -1000.7;
    XII_TEST_FLOAT(fp.ToFloat(), -1000.75f, 0.01f);

    fp = -1000.8;
    XII_TEST_FLOAT(fp.ToFloat(), -1000.75f, 0.01f);

    fp = -1000.9;
    XII_TEST_FLOAT(fp.ToFloat(), -1001.0f, 0.01f);
  }

  // Disabled because MSVC 2017 has code generation issues in Release builds
  XII_TEST_BLOCK(xiiTestBlock::Disabled, "Multiplication Rounding")
  {
    xiiFixedPoint<2> fp; // 2 Bits -> 4 fractional values

    fp = 0.25;
    fp *= xiiFixedPoint<2>(1.5); // -> should be 0.375, which is not representable -> will be rounded up

    XII_TEST_FLOAT(fp, 0.5f, 0.01f);

    fp = -0.25;
    fp *= xiiFixedPoint<2>(1.5); // -> should be -0.375, which is not representable -> will be rounded up (towards zero)

    XII_TEST_FLOAT(fp, -0.25f, 0.01f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Division Rounding")
  {
    xiiFixedPoint<12> fp2(1000);
    XII_TEST_INT(fp2.GetRawValue(), 1000 << 12);

    fp2 /= xiiFixedPoint<12>(2);
    XII_TEST_INT(fp2.GetRawValue(), 500 << 12);

    fp2 += xiiFixedPoint<12>(1);
    XII_TEST_INT(fp2.GetRawValue(), 501 << 12);

    fp2 /= xiiFixedPoint<12>(2);
    XII_TEST_INT(fp2.GetRawValue(), (250 << 12) + (1 << 11));

    fp2 /= xiiFixedPoint<12>(2);
    XII_TEST_INT(fp2.GetRawValue(), (125 << 12) + (1 << 10));

    fp2 /= xiiFixedPoint<12>(2);
    XII_TEST_INT(fp2.GetRawValue(), (62 << 12) + (1 << 11) + (1 << 9));

    fp2 /= xiiFixedPoint<12>(2);
    XII_TEST_INT(fp2.GetRawValue(), (31 << 12) + (1 << 10) + (1 << 8));

    fp2 /= xiiFixedPoint<12>(2);
    XII_TEST_INT(fp2.GetRawValue(), (15 << 12) + (1 << 11) + (1 << 9) + (1 << 7));

    fp2 /= xiiFixedPoint<12>(2);
    XII_TEST_INT(fp2.GetRawValue(), (7 << 12) + (1 << 11) + (1 << 10) + (1 << 8) + (1 << 6));

    fp2 /= xiiFixedPoint<12>(2);
    XII_TEST_INT(fp2.GetRawValue(), (3 << 12) + (1 << 11) + (1 << 10) + (1 << 9) + (1 << 7) + (1 << 5));

    fp2 /= xiiFixedPoint<12>(2);
    XII_TEST_INT(fp2.GetRawValue(), (1 << 12) + (1 << 11) + (1 << 10) + (1 << 9) + (1 << 8) + (1 << 6) + (1 << 4));

    fp2 /= xiiFixedPoint<12>(2);
    XII_TEST_INT(fp2.GetRawValue(), (1 << 11) + (1 << 10) + (1 << 9) + (1 << 8) + (1 << 7) + (1 << 5) + (1 << 3));

    fp2 /= xiiFixedPoint<12>(2);
    XII_TEST_INT(fp2.GetRawValue(), (1 << 10) + (1 << 9) + (1 << 8) + (1 << 7) + (1 << 6) + (1 << 4) + (1 << 2));

    fp2 /= xiiFixedPoint<12>(2);
    XII_TEST_INT(fp2.GetRawValue(), (1 << 9) + (1 << 8) + (1 << 7) + (1 << 6) + (1 << 5) + (1 << 3) + (1 << 1));

    fp2 /= xiiFixedPoint<12>(2);
    XII_TEST_INT(fp2.GetRawValue(), (1 << 8) + (1 << 7) + (1 << 6) + (1 << 5) + (1 << 4) + (1 << 2) + (1 << 0));

    fp2 /= xiiFixedPoint<12>(2);
    XII_TEST_INT(fp2.GetRawValue(), (1 << 7) + (1 << 6) + (1 << 5) + (1 << 4) + (1 << 3) + (1 << 1) + (1 << 0)); // here we round up

    fp2 /= xiiFixedPoint<12>(2);
    XII_TEST_INT(fp2.GetRawValue(), (1 << 6) + (1 << 5) + (1 << 4) + (1 << 3) + (1 << 2) + (1 << 1));

    fp2 /= xiiFixedPoint<12>(2);
    XII_TEST_INT(fp2.GetRawValue(), (1 << 5) + (1 << 4) + (1 << 3) + (1 << 2) + (1 << 1) + (1 << 0));

    fp2 /= xiiFixedPoint<12>(2);
    XII_TEST_INT(fp2.GetRawValue(), (1 << 4) + (1 << 3) + (1 << 2) + (1 << 1) + (1 << 0) + (1 << 0)); // here we round up again

    fp2 /= xiiFixedPoint<12>(2);
    XII_TEST_INT(fp2.GetRawValue(), (1 << 3) + (1 << 2) + (1 << 1) + (1 << 0) + (1 << 0)); // here we round up again

    fp2 /= xiiFixedPoint<12>(2);
    XII_TEST_INT(fp2.GetRawValue(), (1 << 2) + (1 << 1) + (1 << 1)); // here we round up again

    fp2 /= xiiFixedPoint<12>(2);
    XII_TEST_INT(fp2.GetRawValue(), (1 << 1) + (1 << 1)); // here we round up again

    fp2 /= xiiFixedPoint<12>(2);
    XII_TEST_INT(fp2.GetRawValue(), (1 << 1));

    fp2 /= xiiFixedPoint<12>(2);
    XII_TEST_INT(fp2.GetRawValue(), (1 << 0));

    fp2 /= xiiFixedPoint<12>(2);
    XII_TEST_INT(fp2.GetRawValue(), (1 << 0)); // we can never get lower than this by dividing by 2, as it will always get rounded up again

    fp2 /= xiiFixedPoint<12>(2.01);
    XII_TEST_INT(fp2.GetRawValue(), 0); // finally we round down
  }
}
