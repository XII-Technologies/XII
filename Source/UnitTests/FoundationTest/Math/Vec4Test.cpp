/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>


XII_CREATE_SIMPLE_TEST(Math, Vec4)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    if (xiiMath::SupportsNaN<xiiMathTestType>())
    {
      // In debug the default constructor initializes everything with NaN.
      xiiVec4T vDefCtor;
      XII_TEST_BOOL(xiiMath::IsNaN(vDefCtor.x) && xiiMath::IsNaN(vDefCtor.y) /* && xiiMath::IsNaN(vDefCtor.z) && xiiMath::IsNaN(vDefCtor.w)*/);
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    xiiVec4T::ComponentType testBlock[4] = {(xiiVec4T::ComponentType)1, (xiiVec4T::ComponentType)2, (xiiVec4T::ComponentType)3, (xiiVec4T::ComponentType)4};
    xiiVec4T*               pDefCtor     = ::new ((void*)&testBlock[0]) xiiVec4T;
    XII_TEST_BOOL(pDefCtor->x == (xiiVec4T::ComponentType)1 && pDefCtor->y == (xiiVec4T::ComponentType)2 && pDefCtor->z == (xiiVec4T::ComponentType)3 && pDefCtor->w == (xiiVec4T::ComponentType)4);
#endif

    // Make sure the class didn't accidentally change in size.
    XII_TEST_BOOL(sizeof(xiiVec4) == 16);
    XII_TEST_BOOL(sizeof(xiiVec4d) == 32);

    xiiVec4T vInit1F(2.0f);
    XII_TEST_BOOL(vInit1F.x == 2.0f && vInit1F.y == 2.0f && vInit1F.z == 2.0f && vInit1F.w == 2.0f);

    xiiVec4T vInit4F(1.0f, 2.0f, 3.0f, 4.0f);
    XII_TEST_BOOL(vInit4F.x == 1.0f && vInit4F.y == 2.0f && vInit4F.z == 3.0f && vInit4F.w == 4.0f);

    xiiVec4T vCopy(vInit4F);
    XII_TEST_BOOL(vCopy.x == 1.0f && vCopy.y == 2.0f && vCopy.z == 3.0f && vCopy.w == 4.0f);

    xiiVec4T vZero = xiiVec4T::MakeZero();
    XII_TEST_BOOL(vZero.x == 0.0f && vZero.y == 0.0f && vZero.z == 0.0f && vZero.w == 0.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Conversion")
  {
    xiiVec4T vData(1.0f, 2.0f, 3.0f, 4.0f);
    xiiVec2T vToVec2 = vData.GetAsVec2();
    XII_TEST_BOOL(vToVec2.x == vData.x && vToVec2.y == vData.y);

    xiiVec3T vToVec3 = vData.GetAsVec3();
    XII_TEST_BOOL(vToVec3.x == vData.x && vToVec3.y == vData.y && vToVec3.z == vData.z);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Setter")
  {
    xiiVec4T vSet1F;
    vSet1F.Set(2.0f);
    XII_TEST_BOOL(vSet1F.x == 2.0f && vSet1F.y == 2.0f && vSet1F.z == 2.0f && vSet1F.w == 2.0f);

    xiiVec4T vSet4F;
    vSet4F.Set(1.0f, 2.0f, 3.0f, 4.0f);
    XII_TEST_BOOL(vSet4F.x == 1.0f && vSet4F.y == 2.0f && vSet4F.z == 3.0f && vSet4F.w == 4.0f);

    xiiVec4T vSetZero;
    vSetZero.SetZero();
    XII_TEST_BOOL(vSetZero.x == 0.0f && vSetZero.y == 0.0f && vSetZero.z == 0.0f && vSetZero.w == 0.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Length")
  {
    const xiiVec4T vOp1(-4.0, 4.0f, -2.0f, -0.0f);
    const xiiVec4T compArray[4] = {
      xiiVec4T(1.0f, 0.0f, 0.0f, 0.0f), xiiVec4T(0.0f, 1.0f, 0.0f, 0.0f), xiiVec4T(0.0f, 0.0f, 1.0f, 0.0f), xiiVec4T(0.0f, 0.0f, 0.0f, 1.0f)};

    // GetLength
    XII_TEST_FLOAT(vOp1.GetLength(), 6.0f, xiiMath::SmallEpsilon<xiiMathTestType>());

    // GetLengthSquared
    XII_TEST_FLOAT(vOp1.GetLengthSquared(), 36.0f, xiiMath::SmallEpsilon<xiiMathTestType>());

    // GetLengthAndNormalize
    xiiVec4T        vLengthAndNorm = vOp1;
    xiiMathTestType fLength        = vLengthAndNorm.GetLengthAndNormalize();
    XII_TEST_FLOAT(vLengthAndNorm.GetLength(), 1.0f, xiiMath::SmallEpsilon<xiiMathTestType>());
    XII_TEST_FLOAT(fLength, 6.0f, xiiMath::SmallEpsilon<xiiMathTestType>());
    XII_TEST_FLOAT(vLengthAndNorm.x * vLengthAndNorm.x + vLengthAndNorm.y * vLengthAndNorm.y + vLengthAndNorm.z * vLengthAndNorm.z + vLengthAndNorm.w * vLengthAndNorm.w, 1.0f, xiiMath::SmallEpsilon<xiiMathTestType>());
    XII_TEST_BOOL(vLengthAndNorm.IsNormalized(xiiMath::SmallEpsilon<xiiMathTestType>()));

    // GetNormalized
    xiiVec4T vGetNorm = vOp1.GetNormalized();
    XII_TEST_FLOAT(vGetNorm.x * vGetNorm.x + vGetNorm.y * vGetNorm.y + vGetNorm.z * vGetNorm.z + vGetNorm.w * vGetNorm.w, 1.0f, xiiMath::SmallEpsilon<xiiMathTestType>());
    XII_TEST_BOOL(vGetNorm.IsNormalized(xiiMath::SmallEpsilon<xiiMathTestType>()));

    // Normalize
    xiiVec4T vNorm = vOp1;
    vNorm.Normalize();
    XII_TEST_FLOAT(vNorm.x * vNorm.x + vNorm.y * vNorm.y + vNorm.z * vNorm.z + vNorm.w * vNorm.w, 1.0f, xiiMath::SmallEpsilon<xiiMathTestType>());
    XII_TEST_BOOL(vNorm.IsNormalized(xiiMath::SmallEpsilon<xiiMathTestType>()));

    // NormalizeIfNotZero
    xiiVec4T vNormCond = vNorm * xiiMath::DefaultEpsilon<xiiMathTestType>();
    XII_TEST_BOOL(vNormCond.NormalizeIfNotZero(vOp1, xiiMath::LargeEpsilon<xiiMathTestType>()) == XII_FAILURE);
    XII_TEST_BOOL(vNormCond == vOp1);
    vNormCond = vNorm * xiiMath::DefaultEpsilon<xiiMathTestType>();
    XII_TEST_BOOL(vNormCond.NormalizeIfNotZero(vOp1, xiiMath::SmallEpsilon<xiiMathTestType>()) == XII_SUCCESS);
    XII_TEST_VEC4(vNormCond, vNorm, xiiMath::DefaultEpsilon<xiiVec4T::ComponentType>());

    // IsZero
    XII_TEST_BOOL(xiiVec4T::MakeZero().IsZero());
    for (int i = 0; i < 4; ++i)
    {
      XII_TEST_BOOL(!compArray[i].IsZero());
    }

    // IsZero(float)
    XII_TEST_BOOL(xiiVec4T::MakeZero().IsZero(0.0f));
    for (int i = 0; i < 4; ++i)
    {
      XII_TEST_BOOL(!compArray[i].IsZero(0.0f));
      XII_TEST_BOOL(compArray[i].IsZero(1.0f));
      XII_TEST_BOOL((-compArray[i]).IsZero(1.0f));
    }

    // IsNormalized (already tested above)
    for (int i = 0; i < 4; ++i)
    {
      XII_TEST_BOOL(compArray[i].IsNormalized());
      XII_TEST_BOOL((-compArray[i]).IsNormalized());
      XII_TEST_BOOL((compArray[i] * (xiiMathTestType)2).IsNormalized((xiiMathTestType)4));
      XII_TEST_BOOL((compArray[i] * (xiiMathTestType)2).IsNormalized((xiiMathTestType)4));
    }

    if (xiiMath::SupportsNaN<xiiMathTestType>())
    {
      xiiMathTestType TypeNaN     = xiiMath::NaN<xiiMathTestType>();
      const xiiVec4T  nanArray[4] = {xiiVec4T(TypeNaN, 0.0f, 0.0f, 0.0f), xiiVec4T(0.0f, TypeNaN, 0.0f, 0.0f), xiiVec4T(0.0f, 0.0f, TypeNaN, 0.0f), xiiVec4T(0.0f, 0.0f, 0.0f, TypeNaN)};

      // IsNaN
      for (int i = 0; i < 4; ++i)
      {
        XII_TEST_BOOL(nanArray[i].IsNaN());
        XII_TEST_BOOL(!compArray[i].IsNaN());
      }

      // IsValid
      for (int i = 0; i < 4; ++i)
      {
        XII_TEST_BOOL(!nanArray[i].IsValid());
        XII_TEST_BOOL(compArray[i].IsValid());

        XII_TEST_BOOL(!(compArray[i] * xiiMath::Infinity<xiiMathTestType>()).IsValid());
        XII_TEST_BOOL(!(compArray[i] * -xiiMath::Infinity<xiiMathTestType>()).IsValid());
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Operators")
  {
    const xiiVec4T vOp1(-4.0, 0.2f, -7.0f, -0.0f);
    const xiiVec4T vOp2(2.0, 0.3f, 0.0f, 1.0f);
    const xiiVec4T compArray[4] = {xiiVec4T(1.0f, 0.0f, 0.0f, 0.0f), xiiVec4T(0.0f, 1.0f, 0.0f, 0.0f), xiiVec4T(0.0f, 0.0f, 1.0f, 0.0f), xiiVec4T(0.0f, 0.0f, 0.0f, 1.0f)};

    // IsIdentical
    XII_TEST_BOOL(vOp1.IsIdentical(vOp1));
    for (int i = 0; i < 4; ++i)
    {
      XII_TEST_BOOL(!vOp1.IsIdentical(vOp1 + (xiiMathTestType)xiiMath::SmallEpsilon<xiiMathTestType>() * compArray[i]));
      XII_TEST_BOOL(!vOp1.IsIdentical(vOp1 - (xiiMathTestType)xiiMath::SmallEpsilon<xiiMathTestType>() * compArray[i]));
    }

    // IsEqual
    XII_TEST_BOOL(vOp1.IsEqual(vOp1, 0.0f));
    for (int i = 0; i < 4; ++i)
    {
      XII_TEST_BOOL(vOp1.IsEqual(vOp1 + xiiMath::SmallEpsilon<xiiMathTestType>() * compArray[i], 2 * xiiMath::SmallEpsilon<xiiMathTestType>()));
      XII_TEST_BOOL(vOp1.IsEqual(vOp1 - xiiMath::SmallEpsilon<xiiMathTestType>() * compArray[i], 2 * xiiMath::SmallEpsilon<xiiMathTestType>()));
      XII_TEST_BOOL(vOp1.IsEqual(vOp1 + xiiMath::DefaultEpsilon<xiiMathTestType>() * compArray[i], 2 * xiiMath::DefaultEpsilon<xiiMathTestType>()));
      XII_TEST_BOOL(vOp1.IsEqual(vOp1 - xiiMath::DefaultEpsilon<xiiMathTestType>() * compArray[i], 2 * xiiMath::DefaultEpsilon<xiiMathTestType>()));
    }

    // operator-
    xiiVec4T vNegated = -vOp1;
    XII_TEST_BOOL(vOp1.x == -vNegated.x && vOp1.y == -vNegated.y && vOp1.z == -vNegated.z && vOp1.w == -vNegated.w);

    // operator+= (xiiVec4T)
    xiiVec4T vPlusAssign = vOp1;
    vPlusAssign += vOp2;
    XII_TEST_BOOL(vPlusAssign.IsEqual(xiiVec4T(-2.0f, 0.5f, -7.0f, 1.0f), xiiMath::SmallEpsilon<xiiMathTestType>()));

    // operator-= (xiiVec4T)
    xiiVec4T vMinusAssign = vOp1;
    vMinusAssign -= vOp2;
    XII_TEST_BOOL(vMinusAssign.IsEqual(xiiVec4T(-6.0f, -0.1f, -7.0f, -1.0f), xiiMath::SmallEpsilon<xiiMathTestType>()));

    // operator*= (float)
    xiiVec4T vMulFloat = vOp1;
    vMulFloat *= 2.0f;
    XII_TEST_BOOL(vMulFloat.IsEqual(xiiVec4T(-8.0f, 0.4f, -14.0f, -0.0f), xiiMath::SmallEpsilon<xiiMathTestType>()));
    vMulFloat *= 0.0f;
    XII_TEST_BOOL(vMulFloat.IsEqual(xiiVec4T::MakeZero(), xiiMath::SmallEpsilon<xiiMathTestType>()));

    // operator/= (float)
    xiiVec4T vDivFloat = vOp1;
    vDivFloat /= 2.0f;
    XII_TEST_BOOL(vDivFloat.IsEqual(xiiVec4T(-2.0f, 0.1f, -3.5f, -0.0f), xiiMath::SmallEpsilon<xiiMathTestType>()));

    // operator+ (xiiVec4T, xiiVec4T)
    xiiVec4T vPlus = (vOp1 + vOp2);
    XII_TEST_BOOL(vPlus.IsEqual(xiiVec4T(-2.0f, 0.5f, -7.0f, 1.0f), xiiMath::SmallEpsilon<xiiMathTestType>()));

    // operator- (xiiVec4T, xiiVec4T)
    xiiVec4T vMinus = (vOp1 - vOp2);
    XII_TEST_BOOL(vMinus.IsEqual(xiiVec4T(-6.0f, -0.1f, -7.0f, -1.0f), xiiMath::SmallEpsilon<xiiMathTestType>()));

    // operator* (float, xiiVec4T)
    xiiVec4T vMulFloatVec4 = ((xiiMathTestType)2 * vOp1);
    XII_TEST_BOOL(vMulFloatVec4.IsEqual(xiiVec4T(-8.0f, 0.4f, -14.0f, -0.0f), xiiMath::SmallEpsilon<xiiMathTestType>()));
    vMulFloatVec4 = ((xiiMathTestType)0 * vOp1);
    XII_TEST_BOOL(vMulFloatVec4.IsEqual(xiiVec4T::MakeZero(), xiiMath::SmallEpsilon<xiiMathTestType>()));

    // operator* (xiiVec4T, float)
    xiiVec4T vMulVec4Float = (vOp1 * (xiiMathTestType)2);
    XII_TEST_BOOL(vMulVec4Float.IsEqual(xiiVec4T(-8.0f, 0.4f, -14.0f, -0.0f), xiiMath::SmallEpsilon<xiiMathTestType>()));
    vMulVec4Float = (vOp1 * (xiiMathTestType)0);
    XII_TEST_BOOL(vMulVec4Float.IsEqual(xiiVec4T::MakeZero(), xiiMath::SmallEpsilon<xiiMathTestType>()));

    // operator/ (xiiVec4T, float)
    xiiVec4T vDivVec4Float = (vOp1 / (xiiMathTestType)2);
    XII_TEST_BOOL(vDivVec4Float.IsEqual(xiiVec4T(-2.0f, 0.1f, -3.5f, -0.0f), xiiMath::SmallEpsilon<xiiMathTestType>()));

    // operator== (xiiVec4T, xiiVec4T)
    XII_TEST_BOOL(vOp1 == vOp1);
    for (int i = 0; i < 4; ++i)
    {
      XII_TEST_BOOL(!(vOp1 == (vOp1 + (xiiMathTestType)xiiMath::SmallEpsilon<xiiMathTestType>() * compArray[i])));
      XII_TEST_BOOL(!(vOp1 == (vOp1 - (xiiMathTestType)xiiMath::SmallEpsilon<xiiMathTestType>() * compArray[i])));
    }

    // operator!= (xiiVec4T, xiiVec4T)
    XII_TEST_BOOL(!(vOp1 != vOp1));
    for (int i = 0; i < 4; ++i)
    {
      XII_TEST_BOOL(vOp1 != (vOp1 + (xiiMathTestType)xiiMath::SmallEpsilon<xiiMathTestType>() * compArray[i]));
      XII_TEST_BOOL(vOp1 != (vOp1 - (xiiMathTestType)xiiMath::SmallEpsilon<xiiMathTestType>() * compArray[i]));
    }

    // operator< (xiiVec4T, xiiVec4T)
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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Common")
  {
    const xiiVec4T vOp1(-4.0, 0.2f, -7.0f, -0.0f);
    const xiiVec4T vOp2(2.0, -0.3f, 0.5f, 1.0f);

    // Dot
    XII_TEST_FLOAT(vOp1.Dot(vOp2), -11.56f, xiiMath::SmallEpsilon<xiiMathTestType>());
    XII_TEST_FLOAT(vOp2.Dot(vOp1), -11.56f, xiiMath::SmallEpsilon<xiiMathTestType>());

    // CompMin
    XII_TEST_BOOL(vOp1.CompMin(vOp2).IsEqual(xiiVec4T(-4.0f, -0.3f, -7.0f, -0.0f), xiiMath::SmallEpsilon<xiiMathTestType>()));
    XII_TEST_BOOL(vOp2.CompMin(vOp1).IsEqual(xiiVec4T(-4.0f, -0.3f, -7.0f, -0.0f), xiiMath::SmallEpsilon<xiiMathTestType>()));

    // CompMax
    XII_TEST_BOOL(vOp1.CompMax(vOp2).IsEqual(xiiVec4T(2.0f, 0.2f, 0.5f, 1.0f), xiiMath::SmallEpsilon<xiiMathTestType>()));
    XII_TEST_BOOL(vOp2.CompMax(vOp1).IsEqual(xiiVec4T(2.0f, 0.2f, 0.5f, 1.0f), xiiMath::SmallEpsilon<xiiMathTestType>()));

    // CompClamp
    XII_TEST_BOOL(vOp1.CompClamp(vOp1, vOp2).IsEqual(xiiVec4T(-4.0f, -0.3f, -7.0f, -0.0f), xiiMath::SmallEpsilon<xiiMathTestType>()));
    XII_TEST_BOOL(vOp2.CompClamp(vOp1, vOp2).IsEqual(xiiVec4T(2.0f, 0.2f, 0.5f, 1.0f), xiiMath::SmallEpsilon<xiiMathTestType>()));

    // CompMul
    XII_TEST_BOOL(vOp1.CompMul(vOp2).IsEqual(xiiVec4T(-8.0f, -0.06f, -3.5f, 0.0f), xiiMath::SmallEpsilon<xiiMathTestType>()));
    XII_TEST_BOOL(vOp2.CompMul(vOp1).IsEqual(xiiVec4T(-8.0f, -0.06f, -3.5f, 0.0f), xiiMath::SmallEpsilon<xiiMathTestType>()));

    // CompDiv
    XII_TEST_BOOL(vOp1.CompDiv(vOp2).IsEqual(xiiVec4T(-2.0f, -0.66666666f, -14.0f, 0.0f), xiiMath::SmallEpsilon<xiiMathTestType>()));

    // Abs
    XII_TEST_VEC4(vOp1.Abs(), xiiVec4T(4.0, 0.2f, 7.0f, 0.0f), xiiMath::SmallEpsilon<xiiMathTestType>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "DistanceTo")
  {
    xiiVec4T v1(0.0f, 0.0f, 0.0f, 0.0f);
    xiiVec4T v2(3.0f, 0.0f, 0.0f, 0.0f);
    XII_TEST_FLOAT(v1.GetDistanceTo(v2), 3.0f, xiiMath::SmallEpsilon<xiiMathTestType>());

    v1 = xiiVec4T(1.0f, 2.0f, 3.0f, 4.0f);
    v2 = xiiVec4T(4.0f, 6.0f, 3.0f, 0.0f);
    XII_TEST_FLOAT(v1.GetDistanceTo(v2), 6.403124237f, xiiMath::SmallEpsilon<xiiMathTestType>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SquaredDistanceTo")
  {
    xiiVec4T v1(0.0f, 0.0f, 0.0f, 0.0f);
    xiiVec4T v2(3.0f, 0.0f, 0.0f, 0.0f);
    XII_TEST_FLOAT(v1.GetSquaredDistanceTo(v2), 9.0f, xiiMath::SmallEpsilon<xiiMathTestType>());

    v1 = xiiVec4T(1.0f, 2.0f, 3.0f, 4.0f);
    v2 = xiiVec4T(4.0f, 6.0f, 3.0f, 0.0f);
    XII_TEST_FLOAT(v1.GetSquaredDistanceTo(v2), 41.0f, xiiMath::SmallEpsilon<xiiMathTestType>());
  }
}
