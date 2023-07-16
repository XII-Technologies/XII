#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Angle.h>

XII_CREATE_SIMPLE_TEST(Math, Angle)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "DegToRad (Float)")
  {
    XII_TEST_FLOAT(xiiAngle::DegToRad(0.0f), 0.0f, 0.00001f);
    XII_TEST_FLOAT(xiiAngle::DegToRad(45.0f), 0.785398163f, 0.00001f);
    XII_TEST_FLOAT(xiiAngle::DegToRad(90.0f), 1.570796327f, 0.00001f);
    XII_TEST_FLOAT(xiiAngle::DegToRad(120.0f), 2.094395102f, 0.00001f);
    XII_TEST_FLOAT(xiiAngle::DegToRad(170.0f), 2.967059728f, 0.00001f);
    XII_TEST_FLOAT(xiiAngle::DegToRad(180.0f), 3.141592654f, 0.00001f);
    XII_TEST_FLOAT(xiiAngle::DegToRad(250.0f), 4.36332313f, 0.00001f);
    XII_TEST_FLOAT(xiiAngle::DegToRad(320.0f), 5.585053606f, 0.00001f);
    XII_TEST_FLOAT(xiiAngle::DegToRad(360.0f), 6.283185307f, 0.00001f);
    XII_TEST_FLOAT(xiiAngle::DegToRad(700.0f), 12.217304764f, 0.00001f);
    XII_TEST_FLOAT(xiiAngle::DegToRad(-123.0f), -2.14675498f, 0.00001f);
    XII_TEST_FLOAT(xiiAngle::DegToRad(-1234.0f), -21.53736297f, 0.00001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "DegToRad (Double)")
  {
    XII_TEST_DOUBLE(xiiAngled::DegToRad(0.0), 0.0, 0.00001);
    XII_TEST_DOUBLE(xiiAngled::DegToRad(45.0), 0.785398163, 0.00001);
    XII_TEST_DOUBLE(xiiAngled::DegToRad(90.0), 1.570796327, 0.00001);
    XII_TEST_DOUBLE(xiiAngled::DegToRad(120.0), 2.094395102, 0.00001);
    XII_TEST_DOUBLE(xiiAngled::DegToRad(170.0), 2.967059728, 0.00001);
    XII_TEST_DOUBLE(xiiAngled::DegToRad(180.0), 3.141592654, 0.00001);
    XII_TEST_DOUBLE(xiiAngled::DegToRad(250.0), 4.36332313, 0.00001);
    XII_TEST_DOUBLE(xiiAngled::DegToRad(320.0), 5.585053606, 0.00001);
    XII_TEST_DOUBLE(xiiAngled::DegToRad(360.0), 6.283185307, 0.00001);
    XII_TEST_DOUBLE(xiiAngled::DegToRad(700.0), 12.217304764, 0.00001);
    XII_TEST_DOUBLE(xiiAngled::DegToRad(-123.0), -2.14675498, 0.00001);
    XII_TEST_DOUBLE(xiiAngled::DegToRad(-1234.0), -21.53736297, 0.00001);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RadToDeg (Float)")
  {
    XII_TEST_FLOAT(xiiAngle::RadToDeg(0.0f), 0.0f, 0.00001f);
    XII_TEST_FLOAT(xiiAngle::RadToDeg(0.785398163f), 45.0f, 0.00001f);
    XII_TEST_FLOAT(xiiAngle::RadToDeg(1.570796327f), 90.0f, 0.00001f);
    XII_TEST_FLOAT(xiiAngle::RadToDeg(2.094395102f), 120.0f, 0.00001f);
    XII_TEST_FLOAT(xiiAngle::RadToDeg(2.967059728f), 170.0f, 0.0001f);
    XII_TEST_FLOAT(xiiAngle::RadToDeg(3.141592654f), 180.0f, 0.00001f);
    XII_TEST_FLOAT(xiiAngle::RadToDeg(4.36332313f), 250.0f, 0.0001f);
    XII_TEST_FLOAT(xiiAngle::RadToDeg(5.585053606f), 320.0f, 0.0001f);
    XII_TEST_FLOAT(xiiAngle::RadToDeg(6.283185307f), 360.0f, 0.00001f);
    XII_TEST_FLOAT(xiiAngle::RadToDeg(12.217304764f), 700.0f, 0.00001f);
    XII_TEST_FLOAT(xiiAngle::RadToDeg(-2.14675498f), -123.0f, 0.00001f);
    XII_TEST_FLOAT(xiiAngle::RadToDeg(-21.53736297f), -1234.0f, 0.001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RadToDeg (Double)")
  {
    XII_TEST_DOUBLE(xiiAngled::RadToDeg(0.0), 0.0, 0.00001);
    XII_TEST_DOUBLE(xiiAngled::RadToDeg(0.785398163), 45.0, 0.00001);
    XII_TEST_DOUBLE(xiiAngled::RadToDeg(1.570796327), 90.0, 0.00001);
    XII_TEST_DOUBLE(xiiAngled::RadToDeg(2.094395102), 120.0, 0.00001);
    XII_TEST_DOUBLE(xiiAngled::RadToDeg(2.967059728), 170.0, 0.0001);
    XII_TEST_DOUBLE(xiiAngled::RadToDeg(3.141592654), 180.0, 0.00001);
    XII_TEST_DOUBLE(xiiAngled::RadToDeg(4.36332313), 250.0, 0.0001);
    XII_TEST_DOUBLE(xiiAngled::RadToDeg(5.585053606), 320.0, 0.0001);
    XII_TEST_DOUBLE(xiiAngled::RadToDeg(6.283185307), 360.0, 0.00001);
    XII_TEST_DOUBLE(xiiAngled::RadToDeg(12.217304764), 700.0, 0.00001);
    XII_TEST_DOUBLE(xiiAngled::RadToDeg(-2.14675498), -123.0, 0.00001);
    XII_TEST_DOUBLE(xiiAngled::RadToDeg(-21.53736297), -1234.0, 0.001);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Init (Float)")
  {
    xiiAngle a0;
    XII_TEST_FLOAT(a0.GetRadian(), 0.0f, 0.0f);
    XII_TEST_FLOAT(a0.GetDegree(), 0.0f, 0.0f);

    xiiAngle a1 = xiiAngle::Radian(1.570796327f);
    XII_TEST_FLOAT(a1.GetRadian(), 1.570796327f, 0.00001f);
    XII_TEST_FLOAT(a1.GetDegree(), 90.0f, 0.00001f);

    xiiAngle a2 = xiiAngle::Degree(90);
    XII_TEST_FLOAT(a2.GetRadian(), 1.570796327f, 0.00001f);
    XII_TEST_FLOAT(a2.GetDegree(), 90.0f, 0.00001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Init (Double)")
  {
    xiiAngled a0;
    XII_TEST_DOUBLE(a0.GetRadian(), 0.0, 0.0);
    XII_TEST_DOUBLE(a0.GetDegree(), 0.0, 0.0);

    xiiAngled a1 = xiiAngled::Radian(1.570796327);
    XII_TEST_DOUBLE(a1.GetRadian(), 1.570796327, 0.00001);
    XII_TEST_DOUBLE(a1.GetDegree(), 90.0, 0.00001);

    xiiAngled a2 = xiiAngled::Degree(90);
    XII_TEST_DOUBLE(a2.GetRadian(), 1.570796327, 0.00001);
    XII_TEST_DOUBLE(a2.GetDegree(), 90.0, 0.00001);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "NormalizeRange / IsEqual (Float)")
  {
    xiiAngle a;

    for (xiiInt32 i = 1; i < 359; i++)
    {
      a = xiiAngle::Degree((float)i);
      a.NormalizeRange();
      XII_TEST_FLOAT(a.GetDegree(), (float)i, 0.01f);
      a = xiiAngle::Degree((float)i);
      a.NormalizeRange();
      XII_TEST_FLOAT(a.GetDegree(), (float)i, 0.01f);
      a = xiiAngle::Degree((float)i + 360.0f);
      a.NormalizeRange();
      XII_TEST_FLOAT(a.GetDegree(), (float)i, 0.01f);
      a = xiiAngle::Degree((float)i - 360.0f);
      a.NormalizeRange();
      XII_TEST_FLOAT(a.GetDegree(), (float)i, 0.01f);
      a = xiiAngle::Degree((float)i + 3600.0f);
      a.NormalizeRange();
      XII_TEST_FLOAT(a.GetDegree(), (float)i, 0.01f);
      a = xiiAngle::Degree((float)i - 3600.0f);
      a.NormalizeRange();
      XII_TEST_FLOAT(a.GetDegree(), (float)i, 0.01f);
      a = xiiAngle::Degree((float)i + 36000.0f);
      a.NormalizeRange();
      XII_TEST_FLOAT(a.GetDegree(), (float)i, 0.01f);
      a = xiiAngle::Degree((float)i - 36000.0f);
      a.NormalizeRange();
      XII_TEST_FLOAT(a.GetDegree(), (float)i, 0.01f);
    }

    for (xiiInt32 i = 0; i < 360; i++)
    {
      a = xiiAngle::Degree((float)i);
      XII_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(xiiAngle::Degree((float)i), xiiAngle::Degree(0.01f)));
      a = xiiAngle::Degree((float)i);
      XII_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(xiiAngle::Degree((float)i), xiiAngle::Degree(0.01f)));
      a = xiiAngle::Degree((float)i + 360.0f);
      XII_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(xiiAngle::Degree((float)i), xiiAngle::Degree(0.01f)));
      a = xiiAngle::Degree((float)i - 360.0f);
      XII_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(xiiAngle::Degree((float)i), xiiAngle::Degree(0.01f)));
      a = xiiAngle::Degree((float)i + 3600.0f);
      XII_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(xiiAngle::Degree((float)i), xiiAngle::Degree(0.01f)));
      a = xiiAngle::Degree((float)i - 3600.0f);
      XII_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(xiiAngle::Degree((float)i), xiiAngle::Degree(0.01f)));
      a = xiiAngle::Degree((float)i + 36000.0f);
      XII_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(xiiAngle::Degree((float)i), xiiAngle::Degree(0.01f)));
      a = xiiAngle::Degree((float)i - 36000.0f);
      XII_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(xiiAngle::Degree((float)i), xiiAngle::Degree(0.01f)));
    }

    for (xiiInt32 i = 0; i < 360; i++)
    {
      a = xiiAngle::Degree((float)i);
      XII_TEST_BOOL(a.IsEqualNormalized(xiiAngle::Degree((float)i), xiiAngle::Degree(0.01f)));
      a = xiiAngle::Degree((float)i);
      XII_TEST_BOOL(a.IsEqualNormalized(xiiAngle::Degree((float)i), xiiAngle::Degree(0.01f)));
      a = xiiAngle::Degree((float)i + 360.0f);
      XII_TEST_BOOL(a.IsEqualNormalized(xiiAngle::Degree((float)i), xiiAngle::Degree(0.01f)));
      a = xiiAngle::Degree((float)i - 360.0f);
      XII_TEST_BOOL(a.IsEqualNormalized(xiiAngle::Degree((float)i), xiiAngle::Degree(0.01f)));
      a = xiiAngle::Degree((float)i + 3600.0f);
      XII_TEST_BOOL(a.IsEqualNormalized(xiiAngle::Degree((float)i), xiiAngle::Degree(0.01f)));
      a = xiiAngle::Degree((float)i - 3600.0f);
      XII_TEST_BOOL(a.IsEqualNormalized(xiiAngle::Degree((float)i), xiiAngle::Degree(0.01f)));
      a = xiiAngle::Degree((float)i + 36000.0f);
      XII_TEST_BOOL(a.IsEqualNormalized(xiiAngle::Degree((float)i), xiiAngle::Degree(0.01f)));
      a = xiiAngle::Degree((float)i - 36000.0f);
      XII_TEST_BOOL(a.IsEqualNormalized(xiiAngle::Degree((float)i), xiiAngle::Degree(0.01f)));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "NormalizeRange / IsEqual (Double)")
  {
    xiiAngled a;

    for (xiiInt32 i = 1; i < 359; i++)
    {
      a = xiiAngled::Degree((double)i);
      a.NormalizeRange();
      XII_TEST_DOUBLE(a.GetDegree(), (double)i, 0.01);
      a = xiiAngled::Degree((double)i);
      a.NormalizeRange();
      XII_TEST_DOUBLE(a.GetDegree(), (double)i, 0.01);
      a = xiiAngled::Degree((double)i + 360.0);
      a.NormalizeRange();
      XII_TEST_DOUBLE(a.GetDegree(), (double)i, 0.01);
      a = xiiAngled::Degree((double)i - 360.0);
      a.NormalizeRange();
      XII_TEST_DOUBLE(a.GetDegree(), (double)i, 0.01);
      a = xiiAngled::Degree((double)i + 3600.0);
      a.NormalizeRange();
      XII_TEST_DOUBLE(a.GetDegree(), (double)i, 0.01);
      a = xiiAngled::Degree((double)i - 3600.0);
      a.NormalizeRange();
      XII_TEST_DOUBLE(a.GetDegree(), (double)i, 0.01);
      a = xiiAngled::Degree((double)i + 36000.0);
      a.NormalizeRange();
      XII_TEST_DOUBLE(a.GetDegree(), (double)i, 0.01);
      a = xiiAngled::Degree((double)i - 36000.0);
      a.NormalizeRange();
      XII_TEST_DOUBLE(a.GetDegree(), (double)i, 0.01);
    }

    for (xiiInt32 i = 0; i < 360; i++)
    {
      a = xiiAngled::Degree((double)i);
      XII_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(xiiAngled::Degree((double)i), xiiAngled::Degree(0.01)));
      a = xiiAngled::Degree((double)i);
      XII_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(xiiAngled::Degree((double)i), xiiAngled::Degree(0.01)));
      a = xiiAngled::Degree((double)i + 360.0);
      XII_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(xiiAngled::Degree((double)i), xiiAngled::Degree(0.01)));
      a = xiiAngled::Degree((double)i - 360.0);
      XII_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(xiiAngled::Degree((double)i), xiiAngled::Degree(0.01)));
      a = xiiAngled::Degree((double)i + 3600.0);
      XII_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(xiiAngled::Degree((double)i), xiiAngled::Degree(0.01)));
      a = xiiAngled::Degree((double)i - 3600.0);
      XII_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(xiiAngled::Degree((double)i), xiiAngled::Degree(0.01)));
      a = xiiAngled::Degree((double)i + 36000.0);
      XII_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(xiiAngled::Degree((double)i), xiiAngled::Degree(0.01)));
      a = xiiAngled::Degree((double)i - 36000.0);
      XII_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(xiiAngled::Degree((double)i), xiiAngled::Degree(0.01)));
    }

    for (xiiInt32 i = 0; i < 360; i++)
    {
      a = xiiAngled::Degree((double)i);
      XII_TEST_BOOL(a.IsEqualNormalized(xiiAngled::Degree((double)i), xiiAngled::Degree(0.01)));
      a = xiiAngled::Degree((double)i);
      XII_TEST_BOOL(a.IsEqualNormalized(xiiAngled::Degree((double)i), xiiAngled::Degree(0.01)));
      a = xiiAngled::Degree((double)i + 360.0);
      XII_TEST_BOOL(a.IsEqualNormalized(xiiAngled::Degree((double)i), xiiAngled::Degree(0.01)));
      a = xiiAngled::Degree((double)i - 360.0);
      XII_TEST_BOOL(a.IsEqualNormalized(xiiAngled::Degree((double)i), xiiAngled::Degree(0.01)));
      a = xiiAngled::Degree((double)i + 3600.0);
      XII_TEST_BOOL(a.IsEqualNormalized(xiiAngled::Degree((double)i), xiiAngled::Degree(0.01)));
      a = xiiAngled::Degree((double)i - 3600.0);
      XII_TEST_BOOL(a.IsEqualNormalized(xiiAngled::Degree((double)i), xiiAngled::Degree(0.01)));
      a = xiiAngled::Degree((double)i + 36000.0);
      XII_TEST_BOOL(a.IsEqualNormalized(xiiAngled::Degree((double)i), xiiAngled::Degree(0.01)));
      a = xiiAngled::Degree((double)i - 36000.0);
      XII_TEST_BOOL(a.IsEqualNormalized(xiiAngled::Degree((double)i), xiiAngled::Degree(0.01)));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "AngleBetween (Float)")
  {
    XII_TEST_FLOAT(xiiAngle::AngleBetween(xiiAngle::Degree(0), xiiAngle::Degree(0)).GetDegree(), 0.0f, 0.0001f);
    XII_TEST_FLOAT(xiiAngle::AngleBetween(xiiAngle::Degree(0), xiiAngle::Degree(360)).GetDegree(), 0.0f, 0.0001f);
    XII_TEST_FLOAT(xiiAngle::AngleBetween(xiiAngle::Degree(360), xiiAngle::Degree(360)).GetDegree(), 0.0f, 0.0001f);
    XII_TEST_FLOAT(xiiAngle::AngleBetween(xiiAngle::Degree(360), xiiAngle::Degree(0)).GetDegree(), 0.0f, 0.0001f);

    XII_TEST_FLOAT(xiiAngle::AngleBetween(xiiAngle::Degree(5), xiiAngle::Degree(186)).GetDegree(), 179.0f, 0.0001f);
    XII_TEST_FLOAT(xiiAngle::AngleBetween(xiiAngle::Degree(-5), xiiAngle::Degree(-186)).GetDegree(), 179.0f, 0.0001f);

    XII_TEST_FLOAT(xiiAngle::AngleBetween(xiiAngle::Degree(360.0f + 5), xiiAngle::Degree(360.0f + 186)).GetDegree(), 179.0f, 0.0001f);
    XII_TEST_FLOAT(xiiAngle::AngleBetween(xiiAngle::Degree(360.0f + -5), xiiAngle::Degree(360.0f - 186)).GetDegree(), 179.0f, 0.0001f);

    for (xiiInt32 i = 0; i <= 179; ++i)
      XII_TEST_FLOAT(xiiAngle::AngleBetween(xiiAngle::Degree((float)i), xiiAngle::Degree((float)(i + i))).GetDegree(), (float)i, 0.0001f);

    for (xiiInt32 i = -179; i <= 0; ++i)
      XII_TEST_FLOAT(xiiAngle::AngleBetween(xiiAngle::Degree((float)i), xiiAngle::Degree((float)(i + i))).GetDegree(), (float)-i, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "AngleBetween (Double)")
  {
    XII_TEST_DOUBLE(xiiAngled::AngleBetween(xiiAngled::Degree(0), xiiAngled::Degree(0)).GetDegree(), 0.0, 0.0001);
    XII_TEST_DOUBLE(xiiAngled::AngleBetween(xiiAngled::Degree(0), xiiAngled::Degree(360)).GetDegree(), 0.0, 0.0001);
    XII_TEST_DOUBLE(xiiAngled::AngleBetween(xiiAngled::Degree(360), xiiAngled::Degree(360)).GetDegree(), 0.0, 0.0001);
    XII_TEST_DOUBLE(xiiAngled::AngleBetween(xiiAngled::Degree(360), xiiAngled::Degree(0)).GetDegree(), 0.0, 0.0001);

    XII_TEST_DOUBLE(xiiAngled::AngleBetween(xiiAngled::Degree(5), xiiAngled::Degree(186)).GetDegree(), 179.0, 0.0001);
    XII_TEST_DOUBLE(xiiAngled::AngleBetween(xiiAngled::Degree(-5), xiiAngled::Degree(-186)).GetDegree(), 179.0, 0.0001);

    XII_TEST_DOUBLE(xiiAngled::AngleBetween(xiiAngled::Degree(360.0 + 5), xiiAngled::Degree(360.0 + 186)).GetDegree(), 179.0f, 0.0001);
    XII_TEST_DOUBLE(xiiAngled::AngleBetween(xiiAngled::Degree(360.0 + -5), xiiAngled::Degree(360.0 - 186)).GetDegree(), 179.0f, 0.0001);

    for (xiiInt32 i = 0; i <= 179; ++i)
      XII_TEST_DOUBLE(xiiAngled::AngleBetween(xiiAngled::Degree((double)i), xiiAngled::Degree((double)(i + i))).GetDegree(), (double)i, 0.0001);

    for (xiiInt32 i = -179; i <= 0; ++i)
      XII_TEST_DOUBLE(xiiAngled::AngleBetween(xiiAngled::Degree((double)i), xiiAngled::Degree((double)(i + i))).GetDegree(), (double)-i, 0.0001);
  }
}
