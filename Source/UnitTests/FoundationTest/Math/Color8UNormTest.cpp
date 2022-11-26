#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Color8UNorm.h>


XII_CREATE_SIMPLE_TEST(Math, Color8UNorm)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor empty")
  {
    // Placement new of the default constructor should not have any effect on the previous data.
    xiiUInt8          testBlock[4] = {0, 64, 128, 255};
    xiiColorLinearUB* pDefCtor     = ::new ((void*)&testBlock[0]) xiiColorLinearUB;
    XII_TEST_BOOL(pDefCtor->r == 0 && pDefCtor->g == 64 && pDefCtor->b == 128 && pDefCtor->a == 255);

    // Make sure the class didn't accidentally change in size
    XII_TEST_BOOL(sizeof(xiiColorLinearUB) == sizeof(xiiUInt8) * 4);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor components")
  {
    xiiColorLinearUB init3(100, 123, 255);
    XII_TEST_BOOL(init3.r == 100 && init3.g == 123 && init3.b == 255 && init3.a == 255);

    xiiColorLinearUB init4(100, 123, 255, 42);
    XII_TEST_BOOL(init4.r == 100 && init4.g == 123 && init4.b == 255 && init4.a == 42);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor copy")
  {
    xiiColorLinearUB init4(100, 123, 255, 42);
    xiiColorLinearUB copy(init4);
    XII_TEST_BOOL(copy.r == 100 && copy.g == 123 && copy.b == 255 && copy.a == 42);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor xiiColor")
  {
    xiiColorLinearUB fromColor32f(xiiColor(0.39f, 0.58f, 0.93f));
    XII_TEST_BOOL(xiiMath::IsEqual<xiiUInt8>(fromColor32f.r, static_cast<xiiUInt8>(xiiColor(0.39f, 0.58f, 0.93f).r * 255), 2) &&
                  xiiMath::IsEqual<xiiUInt8>(fromColor32f.g, static_cast<xiiUInt8>(xiiColor(0.39f, 0.58f, 0.93f).g * 255), 2) &&
                  xiiMath::IsEqual<xiiUInt8>(fromColor32f.b, static_cast<xiiUInt8>(xiiColor(0.39f, 0.58f, 0.93f).b * 255), 2) &&
                  xiiMath::IsEqual<xiiUInt8>(fromColor32f.a, static_cast<xiiUInt8>(xiiColor(0.39f, 0.58f, 0.93f).a * 255), 2));
  }

  // conversion
  {
    xiiColorLinearUB cornflowerBlue(xiiColor(0.39f, 0.58f, 0.93f));

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "Conversion xiiColor")
    {
      xiiColor color32f = cornflowerBlue;
      XII_TEST_BOOL(xiiMath::IsEqual<float>(color32f.r, xiiColor(0.39f, 0.58f, 0.93f).r, 2.0f / 255.0f) &&
                    xiiMath::IsEqual<float>(color32f.g, xiiColor(0.39f, 0.58f, 0.93f).g, 2.0f / 255.0f) &&
                    xiiMath::IsEqual<float>(color32f.b, xiiColor(0.39f, 0.58f, 0.93f).b, 2.0f / 255.0f) &&
                    xiiMath::IsEqual<float>(color32f.a, xiiColor(0.39f, 0.58f, 0.93f).a, 2.0f / 255.0f));
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "Conversion xiiUInt*")
    {
      const xiiUInt8* pUIntsConst = cornflowerBlue.GetData();
      XII_TEST_BOOL(pUIntsConst[0] == cornflowerBlue.r && pUIntsConst[1] == cornflowerBlue.g && pUIntsConst[2] == cornflowerBlue.b &&
                    pUIntsConst[3] == cornflowerBlue.a);

      xiiUInt8* pUInts = cornflowerBlue.GetData();
      XII_TEST_BOOL(pUInts[0] == cornflowerBlue.r && pUInts[1] == cornflowerBlue.g && pUInts[2] == cornflowerBlue.b && pUInts[3] == cornflowerBlue.a);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiColorGammaUB: Constructor")
  {
    xiiColorGammaUB c(50, 150, 200, 100);
    XII_TEST_INT(c.r, 50);
    XII_TEST_INT(c.g, 150);
    XII_TEST_INT(c.b, 200);
    XII_TEST_INT(c.a, 100);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiColorGammaUB: Constructor (xiiColor)")
  {
    xiiColorGammaUB c2 = xiiColor::RebeccaPurple;

    xiiColor c3 = c2;

    XII_TEST_BOOL(c3.IsEqualRGBA(xiiColor::RebeccaPurple, 0.001f));
  }
}
