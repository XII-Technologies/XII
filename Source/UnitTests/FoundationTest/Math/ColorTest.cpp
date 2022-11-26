#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Mat4.h>

XII_CREATE_SIMPLE_TEST(Math, Color)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor empty")
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    if (xiiMath::SupportsNaN<xiiMathTestType>())
    {
      // In debug the default constructor initializes everything with NaN.
      xiiColor defCtor;
      XII_TEST_BOOL(xiiMath::IsNaN(defCtor.r) && xiiMath::IsNaN(defCtor.g) && xiiMath::IsNaN(defCtor.b) && xiiMath::IsNaN(defCtor.a));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    float     testBlock[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    xiiColor* pDefCtor     = ::new ((void*)&testBlock[0]) xiiColor;
    XII_TEST_BOOL(pDefCtor->r == 1.0f && pDefCtor->g == 2.0f && pDefCtor->b == 3.0f && pDefCtor->a == 4.0f);
#endif

    // Make sure the class didn't accidentally change in size
    XII_TEST_BOOL(sizeof(xiiColor) == sizeof(float) * 4);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor components")
  {
    xiiColor init3F(0.5f, 0.6f, 0.7f);
    XII_TEST_BOOL(init3F.r == 0.5f && init3F.g == 0.6f && init3F.b == 0.7f && init3F.a == 1.0f);

    xiiColor init4F(0.5f, 0.6f, 0.7f, 0.8f);
    XII_TEST_BOOL(init4F.r == 0.5f && init4F.g == 0.6f && init4F.b == 0.7f && init4F.a == 0.8f);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor copy")
  {
    xiiColor init4F(0.5f, 0.6f, 0.7f, 0.8f);
    xiiColor copy(init4F);
    XII_TEST_BOOL(copy.r == 0.5f && copy.g == 0.6f && copy.b == 0.7f && copy.a == 0.8f);
  }

  {
    xiiColor cornflowerBlue(xiiColor(0.39f, 0.58f, 0.93f));

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "Conversion float")
    {
      float* pFloats = cornflowerBlue.GetData();
      XII_TEST_BOOL(
        pFloats[0] == cornflowerBlue.r && pFloats[1] == cornflowerBlue.g && pFloats[2] == cornflowerBlue.b && pFloats[3] == cornflowerBlue.a);

      const float* pConstFloats = cornflowerBlue.GetData();
      XII_TEST_BOOL(pConstFloats[0] == cornflowerBlue.r && pConstFloats[1] == cornflowerBlue.g && pConstFloats[2] == cornflowerBlue.b &&
                    pConstFloats[3] == cornflowerBlue.a);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "HSV conversion")
  {
    xiiColor normalizedColor(0.0f, 1.0f, 0.999f, 0.0001f);
    XII_TEST_BOOL(normalizedColor.IsNormalized());
    xiiColor notNormalizedColor0(-0.01f, 1.0f, 0.999f, 0.0001f);
    XII_TEST_BOOL(!notNormalizedColor0.IsNormalized());
    xiiColor notNormalizedColor1(0.5f, 1.1f, 0.9f, 0.1f);
    XII_TEST_BOOL(!notNormalizedColor1.IsNormalized());
    xiiColor notNormalizedColor2(0.1f, 1.0f, 1.999f, 0.1f);
    XII_TEST_BOOL(!notNormalizedColor2.IsNormalized());
    xiiColor notNormalizedColor3(0.1f, 1.0f, 1.0f, -0.1f);
    XII_TEST_BOOL(!notNormalizedColor3.IsNormalized());


    // hsv test - took some samples from http://www.javascripter.net/faq/rgb2hsv.htm
    const xiiColorGammaUB rgb[] = {xiiColorGammaUB(255, 255, 255), xiiColorGammaUB(0, 0, 0), xiiColorGammaUB(123, 12, 1), xiiColorGammaUB(31, 112, 153)};
    const xiiVec3         hsv[] = {xiiVec3(0, 0, 1), xiiVec3(0, 0, 0), xiiVec3(5.4f, 0.991f, 0.48f), xiiVec3(200.2f, 0.797f, 0.600f)};

    for (int i = 0; i < 4; ++i)
    {
      const xiiColor color = rgb[i];
      float          hue, sat, val;
      color.GetHSV(hue, sat, val);

      XII_TEST_FLOAT(hue, hsv[i].x, 0.1f);
      XII_TEST_FLOAT(sat, hsv[i].y, 0.1f);
      XII_TEST_FLOAT(val, hsv[i].z, 0.1f);

      xiiColor fromHSV;
      fromHSV.SetHSV(hsv[i].x, hsv[i].y, hsv[i].z);
      XII_TEST_FLOAT(fromHSV.r, color.r, 0.01f);
      XII_TEST_FLOAT(fromHSV.g, color.g, 0.01f);
      XII_TEST_FLOAT(fromHSV.b, color.b, 0.01f);
    }
  }

  {
    if (xiiMath::SupportsNaN<xiiMathTestType>())
    {
      float          fNaN        = xiiMath::NaN<float>();
      const xiiColor nanArray[4] = {
        xiiColor(fNaN, 0.0f, 0.0f, 0.0f), xiiColor(0.0f, fNaN, 0.0f, 0.0f), xiiColor(0.0f, 0.0f, fNaN, 0.0f), xiiColor(0.0f, 0.0f, 0.0f, fNaN)};
      const xiiColor compArray[4] = {
        xiiColor(1.0f, 0.0f, 0.0f, 0.0f), xiiColor(0.0f, 1.0f, 0.0f, 0.0f), xiiColor(0.0f, 0.0f, 1.0f, 0.0f), xiiColor(0.0f, 0.0f, 0.0f, 1.0f)};


      XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsNaN")
      {
        for (int i = 0; i < 4; ++i)
        {
          XII_TEST_BOOL(nanArray[i].IsNaN());
          XII_TEST_BOOL(!compArray[i].IsNaN());
        }
      }

      XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsValid")
      {
        for (int i = 0; i < 4; ++i)
        {
          XII_TEST_BOOL(!nanArray[i].IsValid());
          XII_TEST_BOOL(compArray[i].IsValid());

          XII_TEST_BOOL(!(compArray[i] * xiiMath::Infinity<float>()).IsValid());
          XII_TEST_BOOL(!(compArray[i] * -xiiMath::Infinity<float>()).IsValid());
        }
      }
    }
  }

  {
    const xiiColor op1(-4.0, 0.2f, -7.0f, -0.0f);
    const xiiColor op2(2.0, 0.3f, 0.0f, 1.0f);
    const xiiColor compArray[4] = {
      xiiColor(1.0f, 0.0f, 0.0f, 0.0f), xiiColor(0.0f, 1.0f, 0.0f, 0.0f), xiiColor(0.0f, 0.0f, 1.0f, 0.0f), xiiColor(0.0f, 0.0f, 0.0f, 1.0f)};

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetRGB / SetRGBA")
    {
      xiiColor c1(0, 0, 0, 0);

      c1.SetRGBA(1, 2, 3, 4);

      XII_TEST_BOOL(c1 == xiiColor(1, 2, 3, 4));

      c1.SetRGB(5, 6, 7);

      XII_TEST_BOOL(c1 == xiiColor(5, 6, 7, 4));
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsIdenticalRGB")
    {
      xiiColor c1(0, 0, 0, 0);
      xiiColor c2(0, 0, 0, 1);

      XII_TEST_BOOL(c1.IsIdenticalRGB(c2));
      XII_TEST_BOOL(!c1.IsIdenticalRGBA(c2));
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsIdenticalRGBA")
    {
      XII_TEST_BOOL(op1.IsIdenticalRGBA(op1));
      for (int i = 0; i < 4; ++i)
      {
        XII_TEST_BOOL(!op1.IsIdenticalRGBA(op1 + xiiMath::SmallEpsilon<float>() * compArray[i]));
        XII_TEST_BOOL(!op1.IsIdenticalRGBA(op1 - xiiMath::SmallEpsilon<float>() * compArray[i]));
      }
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsEqualRGB")
    {
      xiiColor c1(0, 0, 0, 0);
      xiiColor c2(0, 0, 0.2f, 1);

      XII_TEST_BOOL(!c1.IsEqualRGB(c2, 0.1f));
      XII_TEST_BOOL(c1.IsEqualRGB(c2, 0.3f));
      XII_TEST_BOOL(!c1.IsEqualRGBA(c2, 0.3f));
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsEqualRGBA")
    {
      XII_TEST_BOOL(op1.IsEqualRGBA(op1, 0.0f));
      for (int i = 0; i < 4; ++i)
      {
        XII_TEST_BOOL(op1.IsEqualRGBA(op1 + xiiMath::SmallEpsilon<float>() * compArray[i], 2 * xiiMath::SmallEpsilon<float>()));
        XII_TEST_BOOL(op1.IsEqualRGBA(op1 - xiiMath::SmallEpsilon<float>() * compArray[i], 2 * xiiMath::SmallEpsilon<float>()));
        XII_TEST_BOOL(op1.IsEqualRGBA(op1 + xiiMath::DefaultEpsilon<float>() * compArray[i], 2 * xiiMath::DefaultEpsilon<float>()));
        XII_TEST_BOOL(op1.IsEqualRGBA(op1 - xiiMath::DefaultEpsilon<float>() * compArray[i], 2 * xiiMath::DefaultEpsilon<float>()));
      }
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator+= (xiiColor)")
    {
      xiiColor plusAssign = op1;
      plusAssign += op2;
      XII_TEST_BOOL(plusAssign.IsEqualRGBA(xiiColor(-2.0f, 0.5f, -7.0f, 1.0f), xiiMath::SmallEpsilon<float>()));
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator-= (xiiColor)")
    {
      xiiColor minusAssign = op1;
      minusAssign -= op2;
      XII_TEST_BOOL(minusAssign.IsEqualRGBA(xiiColor(-6.0f, -0.1f, -7.0f, -1.0f), xiiMath::SmallEpsilon<float>()));
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "ooperator*= (float)")
    {
      xiiColor mulFloat = op1;
      mulFloat *= 2.0f;
      XII_TEST_BOOL(mulFloat.IsEqualRGBA(xiiColor(-8.0f, 0.4f, -14.0f, -0.0f), xiiMath::SmallEpsilon<float>()));
      mulFloat *= 0.0f;
      XII_TEST_BOOL(mulFloat.IsEqualRGBA(xiiColor(0.0f, 0.0f, 0.0f, 0.0f), xiiMath::SmallEpsilon<float>()));
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator/= (float)")
    {
      xiiColor vDivFloat = op1;
      vDivFloat /= 2.0f;
      XII_TEST_BOOL(vDivFloat.IsEqualRGBA(xiiColor(-2.0f, 0.1f, -3.5f, -0.0f), xiiMath::SmallEpsilon<float>()));
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator+ (xiiColor, xiiColor)")
    {
      xiiColor plus = (op1 + op2);
      XII_TEST_BOOL(plus.IsEqualRGBA(xiiColor(-2.0f, 0.5f, -7.0f, 1.0f), xiiMath::SmallEpsilon<float>()));
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator- (xiiColor, xiiColor)")
    {
      xiiColor minus = (op1 - op2);
      XII_TEST_BOOL(minus.IsEqualRGBA(xiiColor(-6.0f, -0.1f, -7.0f, -1.0f), xiiMath::SmallEpsilon<float>()));
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator* (float, xiiColor)")
    {
      xiiColor mulFloatVec4 = 2 * op1;
      XII_TEST_BOOL(mulFloatVec4.IsEqualRGBA(xiiColor(-8.0f, 0.4f, -14.0f, -0.0f), xiiMath::SmallEpsilon<float>()));
      mulFloatVec4 = ((float)0 * op1);
      XII_TEST_BOOL(mulFloatVec4.IsEqualRGBA(xiiColor(0.0f, 0.0f, 0.0f, 0.0f), xiiMath::SmallEpsilon<float>()));
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator* (xiiColor, float)")
    {
      xiiColor mulVec4Float = op1 * 2;
      XII_TEST_BOOL(mulVec4Float.IsEqualRGBA(xiiColor(-8.0f, 0.4f, -14.0f, -0.0f), xiiMath::SmallEpsilon<float>()));
      mulVec4Float = (op1 * (float)0);
      XII_TEST_BOOL(mulVec4Float.IsEqualRGBA(xiiColor(0.0f, 0.0f, 0.0f, 0.0f), xiiMath::SmallEpsilon<float>()));
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator/ (xiiColor, float)")
    {
      xiiColor vDivVec4Float = op1 / 2;
      XII_TEST_BOOL(vDivVec4Float.IsEqualRGBA(xiiColor(-2.0f, 0.1f, -3.5f, -0.0f), xiiMath::SmallEpsilon<float>()));
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator== (xiiColor, xiiColor)")
    {
      XII_TEST_BOOL(op1 == op1);
      for (int i = 0; i < 4; ++i)
      {
        XII_TEST_BOOL(!(op1 == (op1 + xiiMath::SmallEpsilon<float>() * compArray[i])));
        XII_TEST_BOOL(!(op1 == (op1 - xiiMath::SmallEpsilon<float>() * compArray[i])));
      }
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator< (xiiColor, xiiColor)")
    {
      for (int i = 0; i < 4; ++i)
      {
        for (int j = 0; j < 4; ++j)
        {
          if (i == j)
          {
            XII_TEST_BOOL(!(compArray[i] < compArray[j]));
            XII_TEST_BOOL(!(compArray[j] < compArray[i]));
          }
          else if (i < j)
          {
            XII_TEST_BOOL(!(compArray[i] < compArray[j]));
            XII_TEST_BOOL(compArray[j] < compArray[i]);
          }
          else
          {
            XII_TEST_BOOL(!(compArray[j] < compArray[i]));
            XII_TEST_BOOL(compArray[i] < compArray[j]);
          }
        }
      }
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator!= (xiiColor, xiiColor)")
    {
      XII_TEST_BOOL(!(op1 != op1));
      for (int i = 0; i < 4; ++i)
      {
        XII_TEST_BOOL(op1 != (op1 + xiiMath::SmallEpsilon<float>() * compArray[i]));
        XII_TEST_BOOL(op1 != (op1 - xiiMath::SmallEpsilon<float>() * compArray[i]));
      }
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator= (xiiColorLinearUB)")
    {
      xiiColor         c;
      xiiColorLinearUB lin(50, 100, 150, 255);

      c = lin;

      XII_TEST_FLOAT(c.r, 50 / 255.0f, 0.001f);
      XII_TEST_FLOAT(c.g, 100 / 255.0f, 0.001f);
      XII_TEST_FLOAT(c.b, 150 / 255.0f, 0.001f);
      XII_TEST_FLOAT(c.a, 1.0f, 0.001f);
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator= (xiiColorGammaUB) / constructor(xiiColorGammaUB)")
    {
      xiiColor        c;
      xiiColorGammaUB gamma(50, 100, 150, 255);

      c           = gamma;
      xiiColor c3 = gamma;

      XII_TEST_BOOL(c == c3);

      XII_TEST_FLOAT(c.r, 0.031f, 0.001f);
      XII_TEST_FLOAT(c.g, 0.127f, 0.001f);
      XII_TEST_FLOAT(c.b, 0.304f, 0.001f);
      XII_TEST_FLOAT(c.a, 1.0f, 0.001f);

      xiiColorGammaUB c2 = c;

      XII_TEST_INT(c2.r, 50);
      XII_TEST_INT(c2.g, 100);
      XII_TEST_INT(c2.b, 150);
      XII_TEST_INT(c2.a, 255);
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetInvertedColor")
    {
      const xiiColor c1(0.1f, 0.3f, 0.7f, 0.9f);

      xiiColor c2 = c1.GetInvertedColor();

      XII_TEST_BOOL(c2.IsEqualRGBA(xiiColor(0.9f, 0.7f, 0.3f, 0.1f), 0.01f));
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetLuminance")
    {
      XII_TEST_FLOAT(xiiColor::Black.GetLuminance(), 0.0f, 0.001f);
      XII_TEST_FLOAT(xiiColor::White.GetLuminance(), 1.0f, 0.001f);

      XII_TEST_FLOAT(xiiColor(0.5f, 0.5f, 0.5f).GetLuminance(), 0.2126f * 0.5f + 0.7152f * 0.5f + 0.0722f * 0.5f, 0.001f);
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetComplementaryColor")
    {
      // black and white have no complementary colors, or rather, they are their own complementary colors, apparently
      XII_TEST_BOOL(xiiColor::Black.GetComplementaryColor().IsEqualRGBA(xiiColor::Black, 0.001f));
      XII_TEST_BOOL(xiiColor::White.GetComplementaryColor().IsEqualRGBA(xiiColor::White, 0.001f));

      XII_TEST_BOOL(xiiColor::Red.GetComplementaryColor().IsEqualRGBA(xiiColor::Cyan, 0.001f));
      XII_TEST_BOOL(xiiColor::Lime.GetComplementaryColor().IsEqualRGBA(xiiColor::Magenta, 0.001f));
      XII_TEST_BOOL(xiiColor::Blue.GetComplementaryColor().IsEqualRGBA(xiiColor::Yellow, 0.001f));
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetSaturation")
    {
      XII_TEST_FLOAT(xiiColor::Black.GetSaturation(), 0.0f, 0.001f);
      XII_TEST_FLOAT(xiiColor::White.GetSaturation(), 0.0f, 0.001f);
      XII_TEST_FLOAT(xiiColor::Red.GetSaturation(), 1.0f, 0.001f);
      XII_TEST_FLOAT(xiiColor::Lime.GetSaturation(), 1.0f, 0.001f);
      ;
      XII_TEST_FLOAT(xiiColor::Blue.GetSaturation(), 1.0f, 0.001f);
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator * / *= (xiiMat4)")
    {
      xiiMat4 m;
      m.SetIdentity();
      m.SetScalingMatrix(xiiVec3(0.5f, 0.75f, 0.25f));
      m.SetTranslationVector(xiiVec3(0.1f, 0.2f, 0.3f));

      xiiColor c1 = m * xiiColor::White;

      XII_TEST_BOOL(c1.IsEqualRGBA(xiiColor(0.6f, 0.95f, 0.55f, 1.0f), 0.01f));
    }
  }
}
