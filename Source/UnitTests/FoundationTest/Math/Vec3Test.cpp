/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Random.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>


XII_CREATE_SIMPLE_TEST(Math, Vec3)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    if (xiiMath::SupportsNaN<xiiVec3T::ComponentType>())
    {
      // In debug the default constructor initializes everything with NaN.
      xiiVec3T vDefCtor;
      XII_TEST_BOOL(xiiMath::IsNaN(vDefCtor.x) && xiiMath::IsNaN(vDefCtor.y) && xiiMath::IsNaN(vDefCtor.z));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    xiiVec3T::ComponentType testBlock[3] = {(xiiVec3T::ComponentType)1, (xiiVec3T::ComponentType)2, (xiiVec3T::ComponentType)3};
    xiiVec3T*               pDefCtor     = ::new ((void*)&testBlock[0]) xiiVec3T;
    XII_TEST_BOOL(pDefCtor->x == (xiiVec3T::ComponentType)1 && pDefCtor->y == (xiiVec3T::ComponentType)2 && pDefCtor->z == (xiiVec3T::ComponentType)3.);
#endif

    // Make sure the class didn't accidentally change in size.
    XII_TEST_BOOL(sizeof(xiiVec3) == 12);
    XII_TEST_BOOL(sizeof(xiiVec3d) == 24);

    xiiVec3T vInit1F(2.0f);
    XII_TEST_BOOL(vInit1F.x == 2.0f && vInit1F.y == 2.0f && vInit1F.z == 2.0f);

    xiiVec3T vInit4F(1.0f, 2.0f, 3.0f);
    XII_TEST_BOOL(vInit4F.x == 1.0f && vInit4F.y == 2.0f && vInit4F.z == 3.0f);

    xiiVec3T vCopy(vInit4F);
    XII_TEST_BOOL(vCopy.x == 1.0f && vCopy.y == 2.0f && vCopy.z == 3.0f);

    xiiVec3T vZero = xiiVec3T::MakeZero();
    XII_TEST_BOOL(vZero.x == 0.0f && vZero.y == 0.0f && vZero.z == 0.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Conversion")
  {
    xiiVec3T vData(1.0f, 2.0f, 3.0f);
    xiiVec2T vToVec2 = vData.GetAsVec2();
    XII_TEST_BOOL(vToVec2.x == vData.x && vToVec2.y == vData.y);

    xiiVec4T vToVec4 = vData.GetAsVec4(42.0f);
    XII_TEST_BOOL(vToVec4.x == vData.x && vToVec4.y == vData.y && vToVec4.z == vData.z && vToVec4.w == 42.0f);

    xiiVec4T vToVec4Pos = vData.GetAsPositionVec4();
    XII_TEST_BOOL(vToVec4Pos.x == vData.x && vToVec4Pos.y == vData.y && vToVec4Pos.z == vData.z && vToVec4Pos.w == 1.0f);

    xiiVec4T vToVec4Dir = vData.GetAsDirectionVec4();
    XII_TEST_BOOL(vToVec4Dir.x == vData.x && vToVec4Dir.y == vData.y && vToVec4Dir.z == vData.z && vToVec4Dir.w == 0.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Setter")
  {
    xiiVec3T vSet1F;
    vSet1F.Set(2.0f);
    XII_TEST_BOOL(vSet1F.x == 2.0f && vSet1F.y == 2.0f && vSet1F.z == 2.0f);

    xiiVec3T vSet4F;
    vSet4F.Set(1.0f, 2.0f, 3.0f);
    XII_TEST_BOOL(vSet4F.x == 1.0f && vSet4F.y == 2.0f && vSet4F.z == 3.0f);

    xiiVec3T vSetZero;
    vSetZero.SetZero();
    XII_TEST_BOOL(vSetZero.x == 0.0f && vSetZero.y == 0.0f && vSetZero.z == 0.0f);
  }


  {
    const xiiVec3T vOp1(-4.0, 4.0f, -2.0f);
    const xiiVec3T compArray[3] = {xiiVec3T(1.0f, 0.0f, 0.0f), xiiVec3T(0.0f, 1.0f, 0.0f), xiiVec3T(0.0f, 0.0f, 1.0f)};

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetLength")
    {
      XII_TEST_FLOAT(vOp1.GetLength(), 6.0f, xiiMath::SmallEpsilon<xiiMathTestType>());
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetLength")
    {
      xiiVec3T vSetLength = vOp1.GetNormalized() * xiiMath::DefaultEpsilon<xiiMathTestType>();
      XII_TEST_BOOL(vSetLength.SetLength(4.0f, xiiMath::LargeEpsilon<xiiMathTestType>()) == XII_FAILURE);
      XII_TEST_BOOL(vSetLength == xiiVec3T::MakeZero());
      vSetLength = vOp1.GetNormalized() * (xiiMathTestType)0.001;
      XII_TEST_BOOL(vSetLength.SetLength(4.0f, (xiiMathTestType)xiiMath::DefaultEpsilon<xiiMathTestType>()) == XII_SUCCESS);
      XII_TEST_FLOAT(vSetLength.GetLength(), 4.0f, xiiMath::SmallEpsilon<xiiMathTestType>());
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetLengthSquared")
    {
      XII_TEST_FLOAT(vOp1.GetLengthSquared(), 36.0f, xiiMath::SmallEpsilon<xiiMathTestType>());
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetLengthAndNormalize")
    {
      xiiVec3T        vLengthAndNorm = vOp1;
      xiiMathTestType fLength        = vLengthAndNorm.GetLengthAndNormalize();
      XII_TEST_FLOAT(vLengthAndNorm.GetLength(), 1.0f, xiiMath::SmallEpsilon<xiiMathTestType>());
      XII_TEST_FLOAT(fLength, 6.0f, xiiMath::SmallEpsilon<xiiMathTestType>());
      XII_TEST_FLOAT(vLengthAndNorm.x * vLengthAndNorm.x + vLengthAndNorm.y * vLengthAndNorm.y + vLengthAndNorm.z * vLengthAndNorm.z, 1.0f, xiiMath::SmallEpsilon<xiiMathTestType>());
      XII_TEST_BOOL(vLengthAndNorm.IsNormalized(xiiMath::SmallEpsilon<xiiMathTestType>()));
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetNormalized")
    {
      xiiVec3T vGetNorm = vOp1.GetNormalized();
      XII_TEST_FLOAT(vGetNorm.x * vGetNorm.x + vGetNorm.y * vGetNorm.y + vGetNorm.z * vGetNorm.z, 1.0f, xiiMath::SmallEpsilon<xiiMathTestType>());
      XII_TEST_BOOL(vGetNorm.IsNormalized(xiiMath::SmallEpsilon<xiiMathTestType>()));
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "Normalize")
    {
      xiiVec3T vNorm = vOp1;
      vNorm.Normalize();
      XII_TEST_FLOAT(vNorm.x * vNorm.x + vNorm.y * vNorm.y + vNorm.z * vNorm.z, 1.0f, xiiMath::SmallEpsilon<xiiMathTestType>());
      XII_TEST_BOOL(vNorm.IsNormalized(xiiMath::SmallEpsilon<xiiMathTestType>()));
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "NormalizeIfNotZero")
    {
      xiiVec3T vNorm = vOp1;
      vNorm.Normalize();

      xiiVec3T vNormCond = vNorm * xiiMath::DefaultEpsilon<xiiMathTestType>();
      XII_TEST_BOOL(vNormCond.NormalizeIfNotZero(vOp1, xiiMath::LargeEpsilon<xiiMathTestType>()) == XII_FAILURE);
      XII_TEST_BOOL(vNormCond == vOp1);
      vNormCond = vNorm * xiiMath::DefaultEpsilon<xiiMathTestType>();
      XII_TEST_BOOL(vNormCond.NormalizeIfNotZero(vOp1, xiiMath::SmallEpsilon<xiiMathTestType>()) == XII_SUCCESS);
      XII_TEST_VEC3(vNormCond, vNorm, xiiMath::DefaultEpsilon<xiiVec3T::ComponentType>());
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsZero")
    {
      XII_TEST_BOOL(xiiVec3T::MakeZero().IsZero());
      for (int i = 0; i < 3; ++i)
      {
        XII_TEST_BOOL(!compArray[i].IsZero());
      }
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsZero(float)")
    {
      XII_TEST_BOOL(xiiVec3T::MakeZero().IsZero(0.0f));
      for (int i = 0; i < 3; ++i)
      {
        XII_TEST_BOOL(!compArray[i].IsZero(0.0f));
        XII_TEST_BOOL(compArray[i].IsZero(1.0f));
        XII_TEST_BOOL((-compArray[i]).IsZero(1.0f));
      }
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsNormalized (2)")
    {
      for (int i = 0; i < 3; ++i)
      {
        XII_TEST_BOOL(compArray[i].IsNormalized());
        XII_TEST_BOOL((-compArray[i]).IsNormalized());
        XII_TEST_BOOL((compArray[i] * (xiiMathTestType)2).IsNormalized((xiiMathTestType)4));
        XII_TEST_BOOL((compArray[i] * (xiiMathTestType)2).IsNormalized((xiiMathTestType)4));
      }
    }

    if (xiiMath::SupportsNaN<xiiMathTestType>())
    {
      xiiMathTestType fNaN        = xiiMath::NaN<xiiMathTestType>();
      const xiiVec3T  nanArray[3] = {xiiVec3T(fNaN, 0.0f, 0.0f), xiiVec3T(0.0f, fNaN, 0.0f), xiiVec3T(0.0f, 0.0f, fNaN)};

      XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsNaN")
      {
        for (int i = 0; i < 3; ++i)
        {
          XII_TEST_BOOL(nanArray[i].IsNaN());
          XII_TEST_BOOL(!compArray[i].IsNaN());
        }
      }

      XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsValid")
      {
        for (int i = 0; i < 3; ++i)
        {
          XII_TEST_BOOL(!nanArray[i].IsValid());
          XII_TEST_BOOL(compArray[i].IsValid());

          XII_TEST_BOOL(!(compArray[i] * xiiMath::Infinity<xiiMathTestType>()).IsValid());
          XII_TEST_BOOL(!(compArray[i] * -xiiMath::Infinity<xiiMathTestType>()).IsValid());
        }
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Operators")
  {
    const xiiVec3T vOp1(-4.0, 0.2f, -7.0f);
    const xiiVec3T vOp2(2.0, 0.3f, 0.0f);
    const xiiVec3T compArray[3] = {xiiVec3T(1.0f, 0.0f, 0.0f), xiiVec3T(0.0f, 1.0f, 0.0f), xiiVec3T(0.0f, 0.0f, 1.0f)};
    // IsIdentical
    XII_TEST_BOOL(vOp1.IsIdentical(vOp1));
    for (int i = 0; i < 3; ++i)
    {
      XII_TEST_BOOL(!vOp1.IsIdentical(vOp1 + (xiiMathTestType)xiiMath::SmallEpsilon<xiiMathTestType>() * compArray[i]));
      XII_TEST_BOOL(!vOp1.IsIdentical(vOp1 - (xiiMathTestType)xiiMath::SmallEpsilon<xiiMathTestType>() * compArray[i]));
    }

    // IsEqual
    XII_TEST_BOOL(vOp1.IsEqual(vOp1, 0.0f));
    for (int i = 0; i < 3; ++i)
    {
      XII_TEST_BOOL(vOp1.IsEqual(vOp1 + xiiMath::SmallEpsilon<xiiMathTestType>() * compArray[i], 2 * xiiMath::SmallEpsilon<xiiMathTestType>()));
      XII_TEST_BOOL(vOp1.IsEqual(vOp1 - xiiMath::SmallEpsilon<xiiMathTestType>() * compArray[i], 2 * xiiMath::SmallEpsilon<xiiMathTestType>()));
      XII_TEST_BOOL(vOp1.IsEqual(vOp1 + xiiMath::DefaultEpsilon<xiiMathTestType>() * compArray[i], 2 * xiiMath::DefaultEpsilon<xiiMathTestType>()));
      XII_TEST_BOOL(vOp1.IsEqual(vOp1 - xiiMath::DefaultEpsilon<xiiMathTestType>() * compArray[i], 2 * xiiMath::DefaultEpsilon<xiiMathTestType>()));
    }

    // operator-
    xiiVec3T vNegated = -vOp1;
    XII_TEST_BOOL(vOp1.x == -vNegated.x && vOp1.y == -vNegated.y && vOp1.z == -vNegated.z);

    // operator+= (xiiVec3T)
    xiiVec3T vPlusAssign = vOp1;
    vPlusAssign += vOp2;
    XII_TEST_BOOL(vPlusAssign.IsEqual(xiiVec3T(-2.0f, 0.5f, -7.0f), xiiMath::SmallEpsilon<xiiMathTestType>()));

    // operator-= (xiiVec3T)
    xiiVec3T vMinusAssign = vOp1;
    vMinusAssign -= vOp2;
    XII_TEST_BOOL(vMinusAssign.IsEqual(xiiVec3T(-6.0f, -0.1f, -7.0f), xiiMath::SmallEpsilon<xiiMathTestType>()));

    // operator*= (float)
    xiiVec3T vMulFloat = vOp1;
    vMulFloat *= 2.0f;
    XII_TEST_BOOL(vMulFloat.IsEqual(xiiVec3T(-8.0f, 0.4f, -14.0f), xiiMath::SmallEpsilon<xiiMathTestType>()));
    vMulFloat *= 0.0f;
    XII_TEST_BOOL(vMulFloat.IsEqual(xiiVec3T::MakeZero(), xiiMath::SmallEpsilon<xiiMathTestType>()));

    // operator/= (float)
    xiiVec3T vDivFloat = vOp1;
    vDivFloat /= 2.0f;
    XII_TEST_BOOL(vDivFloat.IsEqual(xiiVec3T(-2.0f, 0.1f, -3.5f), xiiMath::SmallEpsilon<xiiMathTestType>()));

    // operator+ (xiiVec3T, xiiVec3T)
    xiiVec3T vPlus = (vOp1 + vOp2);
    XII_TEST_BOOL(vPlus.IsEqual(xiiVec3T(-2.0f, 0.5f, -7.0f), xiiMath::SmallEpsilon<xiiMathTestType>()));

    // operator- (xiiVec3T, xiiVec3T)
    xiiVec3T vMinus = (vOp1 - vOp2);
    XII_TEST_BOOL(vMinus.IsEqual(xiiVec3T(-6.0f, -0.1f, -7.0f), xiiMath::SmallEpsilon<xiiMathTestType>()));

    // operator* (float, xiiVec3T)
    xiiVec3T vMulFloatVec3 = ((xiiMathTestType)2 * vOp1);
    XII_TEST_BOOL(vMulFloatVec3.IsEqual(xiiVec3T((xiiMathTestType)-8.0, (xiiMathTestType)0.4, (xiiMathTestType)-14.0), xiiMath::SmallEpsilon<xiiMathTestType>()));
    vMulFloatVec3 = ((xiiMathTestType)0 * vOp1);
    XII_TEST_BOOL(vMulFloatVec3.IsEqual(xiiVec3T::MakeZero(), xiiMath::SmallEpsilon<xiiMathTestType>()));

    // operator* (xiiVec3T, float)
    xiiVec3T vMulVec3Float = (vOp1 * (xiiMathTestType)2);
    XII_TEST_BOOL(vMulVec3Float.IsEqual(xiiVec3T(-8.0f, 0.4f, -14.0f), xiiMath::SmallEpsilon<xiiMathTestType>()));
    vMulVec3Float = (vOp1 * (xiiMathTestType)0);
    XII_TEST_BOOL(vMulVec3Float.IsEqual(xiiVec3T::MakeZero(), xiiMath::SmallEpsilon<xiiMathTestType>()));

    // operator/ (xiiVec3T, float)
    xiiVec3T vDivVec3Float = (vOp1 / (xiiMathTestType)2);
    XII_TEST_BOOL(vDivVec3Float.IsEqual(xiiVec3T(-2.0f, 0.1f, -3.5f), xiiMath::SmallEpsilon<xiiMathTestType>()));

    // operator== (xiiVec3T, xiiVec3T)
    XII_TEST_BOOL(vOp1 == vOp1);
    for (int i = 0; i < 3; ++i)
    {
      XII_TEST_BOOL(!(vOp1 == (vOp1 + (xiiMathTestType)xiiMath::SmallEpsilon<xiiMathTestType>() * compArray[i])));
      XII_TEST_BOOL(!(vOp1 == (vOp1 - (xiiMathTestType)xiiMath::SmallEpsilon<xiiMathTestType>() * compArray[i])));
    }

    // operator!= (xiiVec3T, xiiVec3T)
    XII_TEST_BOOL(!(vOp1 != vOp1));
    for (int i = 0; i < 3; ++i)
    {
      XII_TEST_BOOL(vOp1 != (vOp1 + (xiiMathTestType)xiiMath::SmallEpsilon<xiiMathTestType>() * compArray[i]));
      XII_TEST_BOOL(vOp1 != (vOp1 - (xiiMathTestType)xiiMath::SmallEpsilon<xiiMathTestType>() * compArray[i]));
    }

    // operator< (xiiVec3T, xiiVec3T)
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
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
    const xiiVec3T vOp1(-4.0, 0.2f, -7.0f);
    const xiiVec3T vOp2(2.0, -0.3f, 0.5f);

    const xiiVec3T compArray[3] = {xiiVec3T(1.0f, 0.0f, 0.0f), xiiVec3T(0.0f, 1.0f, 0.0f), xiiVec3T(0.0f, 0.0f, 1.0f)};

    // GetAngleBetween
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        XII_TEST_FLOAT(compArray[i].GetAngleBetween(compArray[j]).GetDegree(), i == j ? 0.0f : 90.0f, 0.00001f);
      }
    }

    // Dot
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        XII_TEST_FLOAT(compArray[i].Dot(compArray[j]), i == j ? 1.0f : 0.0f, xiiMath::SmallEpsilon<xiiMathTestType>());
      }
    }
    XII_TEST_FLOAT(vOp1.Dot(vOp2), -11.56f, xiiMath::SmallEpsilon<xiiMathTestType>());
    XII_TEST_FLOAT(vOp2.Dot(vOp1), -11.56f, xiiMath::SmallEpsilon<xiiMathTestType>());

    // Cross
    // Right-handed coordinate system check
    XII_TEST_BOOL(compArray[0].CrossRH(compArray[1]).IsEqual(compArray[2], xiiMath::SmallEpsilon<xiiMathTestType>()));
    XII_TEST_BOOL(compArray[1].CrossRH(compArray[2]).IsEqual(compArray[0], xiiMath::SmallEpsilon<xiiMathTestType>()));
    XII_TEST_BOOL(compArray[2].CrossRH(compArray[0]).IsEqual(compArray[1], xiiMath::SmallEpsilon<xiiMathTestType>()));

    // CompMin
    XII_TEST_BOOL(vOp1.CompMin(vOp2).IsEqual(xiiVec3T(-4.0f, -0.3f, -7.0f), xiiMath::SmallEpsilon<xiiMathTestType>()));
    XII_TEST_BOOL(vOp2.CompMin(vOp1).IsEqual(xiiVec3T(-4.0f, -0.3f, -7.0f), xiiMath::SmallEpsilon<xiiMathTestType>()));

    // CompMax
    XII_TEST_BOOL(vOp1.CompMax(vOp2).IsEqual(xiiVec3T(2.0f, 0.2f, 0.5f), xiiMath::SmallEpsilon<xiiMathTestType>()));
    XII_TEST_BOOL(vOp2.CompMax(vOp1).IsEqual(xiiVec3T(2.0f, 0.2f, 0.5f), xiiMath::SmallEpsilon<xiiMathTestType>()));

    // CompClamp
    XII_TEST_BOOL(vOp1.CompClamp(vOp1, vOp2).IsEqual(xiiVec3T(-4.0f, -0.3f, -7.0f), xiiMath::SmallEpsilon<xiiMathTestType>()));
    XII_TEST_BOOL(vOp2.CompClamp(vOp1, vOp2).IsEqual(xiiVec3T(2.0f, 0.2f, 0.5f), xiiMath::SmallEpsilon<xiiMathTestType>()));

    // CompMul
    XII_TEST_BOOL(vOp1.CompMul(vOp2).IsEqual(xiiVec3T(-8.0f, -0.06f, -3.5f), xiiMath::SmallEpsilon<xiiMathTestType>()));
    XII_TEST_BOOL(vOp2.CompMul(vOp1).IsEqual(xiiVec3T(-8.0f, -0.06f, -3.5f), xiiMath::SmallEpsilon<xiiMathTestType>()));

    // CompDiv
    XII_TEST_BOOL(vOp1.CompDiv(vOp2).IsEqual(xiiVec3T(-2.0f, -0.66666666f, -14.0f), xiiMath::SmallEpsilon<xiiMathTestType>()));

    // Abs
    XII_TEST_VEC3(vOp1.Abs(), xiiVec3T(4.0, 0.2f, 7.0f), xiiMath::SmallEpsilon<xiiMathTestType>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CalculateNormal")
  {
    xiiVec3T n;
    XII_TEST_BOOL(n.CalculateNormal(xiiVec3T(-1, 0, 1), xiiVec3T(1, 0, 1), xiiVec3T(0, 0, -1)) == XII_SUCCESS);
    XII_TEST_VEC3(n, xiiVec3T(0, 1, 0), 0.001f);

    XII_TEST_BOOL(n.CalculateNormal(xiiVec3T(-1, 0, -1), xiiVec3T(1, 0, -1), xiiVec3T(0, 0, 1)) == XII_SUCCESS);
    XII_TEST_VEC3(n, xiiVec3T(0, -1, 0), 0.001f);

    XII_TEST_BOOL(n.CalculateNormal(xiiVec3T(-1, 0, 1), xiiVec3T(1, 0, 1), xiiVec3T(1, 0, 1)) == XII_FAILURE);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeOrthogonalTo")
  {
    xiiVec3T v;

    v.Set(1, 1, 0);
    v.MakeOrthogonalTo(xiiVec3T(1, 0, 0));
    XII_TEST_VEC3(v, xiiVec3T(0, 1, 0), 0.001f);

    v.Set(1, 1, 0);
    v.MakeOrthogonalTo(xiiVec3T(0, 1, 0));
    XII_TEST_VEC3(v, xiiVec3T(1, 0, 0), 0.001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetOrthogonalVector")
  {
    xiiVec3T v;

    for (float i = 1; i < 360; i += 3.0f)
    {
      v.Set(i, i * 3, i * 7);
      XII_TEST_FLOAT(v.GetOrthogonalVector().Dot(v), 0.0f, 0.001f);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetReflectedVector")
  {
    xiiVec3T v, v2;

    v.Set(1, 1, 0);
    v2 = v.GetReflectedVector(xiiVec3T(0, -1, 0));
    XII_TEST_VEC3(v2, xiiVec3T(1, -1, 0), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeRandomPointInSphere (float)")
  {
    xiiVec3 v;

    xiiRandom rng;
    rng.Initialize(0xEEFF0011AABBCCDDULL);

    xiiVec3 avg;
    avg.SetZero();

    const xiiUInt32 uiNumSamples = 100'000;
    for (xiiUInt32 i = 0; i < uiNumSamples; ++i)
    {
      v = xiiVec3::MakeRandomPointInSphere(rng);

      XII_TEST_BOOL(v.GetLength() <= 1.0f + xiiMath::SmallEpsilon<float>());
      XII_TEST_BOOL(!v.IsZero());

      avg += v;
    }

    avg /= (float)uiNumSamples;

    // the average point cloud center should be within at least 10% of the sphere's center
    // otherwise the points aren't equally distributed
    XII_TEST_BOOL(avg.IsZero(0.1f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeRandomPointInSphere (double)")
  {
    xiiVec3T v;

    xiiRandom rng;
    rng.Initialize(0xEEFF0011AABBCCDDULL);

    xiiVec3T avg;
    avg.SetZero();

    const xiiUInt32 uiNumSamples = 100'000;
    for (xiiUInt32 i = 0; i < uiNumSamples; ++i)
    {
      v = xiiVec3T::MakeRandomPointInSphere(rng);

      XII_TEST_BOOL(v.GetLength() <= 1.0 + xiiMath::SmallEpsilon<double>());
      XII_TEST_BOOL(!v.IsZero());

      avg += v;
    }

    avg /= (double)uiNumSamples;

    // the average point cloud center should be within at least 10% of the sphere's center
    // otherwise the points aren't equally distributed
    XII_TEST_BOOL(avg.IsZero(static_cast<xiiMathTestType>(0.1)));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeRandomDirection (float)")
  {
    xiiVec3 v;

    xiiRandom rng;
    rng.InitializeFromCurrentTime();

    xiiVec3 avg;
    avg.SetZero();

    const xiiUInt32 uiNumSamples = 100'000;
    for (xiiUInt32 i = 0; i < uiNumSamples; ++i)
    {
      v = xiiVec3::MakeRandomDirection(rng);

      XII_TEST_BOOL(v.IsNormalized());

      avg += v;
    }

    avg /= (float)uiNumSamples;

    // the average point cloud center should be within at least 10% of the sphere's center
    // otherwise the points aren't equally distributed
    XII_TEST_BOOL(avg.IsZero(0.1f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeRandomDirection (double)")
  {
    xiiVec3d v;

    xiiRandom rng;
    rng.InitializeFromCurrentTime();

    xiiVec3d avg;
    avg.SetZero();

    const xiiUInt32 uiNumSamples = 100'000;
    for (xiiUInt32 i = 0; i < uiNumSamples; ++i)
    {
      v = xiiVec3d::MakeRandomDirection(rng);

      XII_TEST_BOOL(v.IsNormalized());

      avg += v;
    }

    avg /= (double)uiNumSamples;

    // the average point cloud center should be within at least 10% of the sphere's center
    // otherwise the points aren't equally distributed
    XII_TEST_BOOL(avg.IsZero(0.1));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeRandomDeviationX (float)")
  {
    xiiVec3 v;
    xiiVec3 avg;
    avg.SetZero();

    xiiRandom rng;
    rng.InitializeFromCurrentTime();

    const xiiAngle  dev          = xiiAngle::MakeFromDegree(65);
    const xiiUInt32 uiNumSamples = 100'000;
    const xiiVec3   vAxis(1, 0, 0);

    for (xiiUInt32 i = 0; i < uiNumSamples; ++i)
    {
      v = xiiVec3::MakeRandomDeviationX(rng, dev);

      XII_TEST_BOOL(v.IsNormalized());

      XII_TEST_BOOL(vAxis.GetAngleBetween(v).GetRadian() <= dev.GetRadian() + xiiMath::DefaultEpsilon<float>());

      avg += v;
    }

    // average direction should be close to the main axis
    avg.Normalize();
    XII_TEST_BOOL(avg.IsEqual(vAxis, 0.1f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeRandomDeviationX (double)")
  {
    xiiVec3d v;
    xiiVec3d avg;
    avg.SetZero();

    xiiRandom rng;
    rng.InitializeFromCurrentTime();

    const xiiAngled dev          = xiiAngled::MakeFromDegree(65);
    const xiiUInt32 uiNumSamples = 100'000;
    const xiiVec3d  vAxis(1, 0, 0);

    for (xiiUInt32 i = 0; i < uiNumSamples; ++i)
    {
      v = xiiVec3d::MakeRandomDeviationX(rng, dev);

      XII_TEST_BOOL(v.IsNormalized());

      XII_TEST_BOOL(vAxis.GetAngleBetween(v).GetRadian() <= dev.GetRadian() + xiiMath::DefaultEpsilon<double>());

      avg += v;
    }

    // average direction should be close to the main axis
    avg.Normalize();
    XII_TEST_BOOL(avg.IsEqual(vAxis, 0.1));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeRandomDeviationY (float)")
  {
    xiiVec3 v;
    xiiVec3 avg;
    avg.SetZero();

    xiiRandom rng;
    rng.InitializeFromCurrentTime();

    const xiiAngle  dev          = xiiAngle::MakeFromDegree(65);
    const xiiUInt32 uiNumSamples = 100'000;
    const xiiVec3   vAxis(0, 1, 0);

    for (xiiUInt32 i = 0; i < uiNumSamples; ++i)
    {
      v = xiiVec3::MakeRandomDeviationY(rng, dev);

      XII_TEST_BOOL(v.IsNormalized());

      XII_TEST_BOOL(vAxis.GetAngleBetween(v).GetRadian() <= dev.GetRadian() + xiiMath::DefaultEpsilon<float>());

      avg += v;
    }

    // average direction should be close to the main axis
    avg.Normalize();
    XII_TEST_BOOL(avg.IsEqual(vAxis, 0.1f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeRandomDeviationY (double)")
  {
    xiiVec3d v;
    xiiVec3d avg;
    avg.SetZero();

    xiiRandom rng;
    rng.InitializeFromCurrentTime();

    const xiiAngled dev          = xiiAngled::MakeFromDegree(65);
    const xiiUInt32 uiNumSamples = 100'000;
    const xiiVec3d  vAxis(0, 1, 0);

    for (xiiUInt32 i = 0; i < uiNumSamples; ++i)
    {
      v = xiiVec3d::MakeRandomDeviationY(rng, dev);

      XII_TEST_BOOL(v.IsNormalized());

      XII_TEST_BOOL(vAxis.GetAngleBetween(v).GetRadian() <= dev.GetRadian() + xiiMath::DefaultEpsilon<double>());

      avg += v;
    }

    // average direction should be close to the main axis
    avg.Normalize();
    XII_TEST_BOOL(avg.IsEqual(vAxis, 0.1));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeRandomDeviationZ (float)")
  {
    xiiVec3 v;
    xiiVec3 avg;
    avg.SetZero();

    xiiRandom rng;
    rng.InitializeFromCurrentTime();

    const xiiAngle  dev          = xiiAngle::MakeFromDegree(65);
    const xiiUInt32 uiNumSamples = 100'000;
    const xiiVec3   vAxis(0, 0, 1);

    for (xiiUInt32 i = 0; i < uiNumSamples; ++i)
    {
      v = xiiVec3::MakeRandomDeviationZ(rng, dev);

      XII_TEST_BOOL(v.IsNormalized());

      XII_TEST_BOOL(vAxis.GetAngleBetween(v).GetRadian() <= dev.GetRadian() + xiiMath::DefaultEpsilon<float>());

      avg += v;
    }

    // average direction should be close to the main axis
    avg.Normalize();
    XII_TEST_BOOL(avg.IsEqual(vAxis, 0.1f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeRandomDeviationZ (double)")
  {
    xiiVec3d v;
    xiiVec3d avg;
    avg.SetZero();

    xiiRandom rng;
    rng.InitializeFromCurrentTime();

    const xiiAngled dev          = xiiAngled::MakeFromDegree(65);
    const xiiUInt32 uiNumSamples = 100'000;
    const xiiVec3d  vAxis(0, 0, 1);

    for (xiiUInt32 i = 0; i < uiNumSamples; ++i)
    {
      v = xiiVec3d::MakeRandomDeviationZ(rng, dev);

      XII_TEST_BOOL(v.IsNormalized());

      XII_TEST_BOOL(vAxis.GetAngleBetween(v).GetRadian() <= dev.GetRadian() + xiiMath::DefaultEpsilon<float>());

      avg += v;
    }

    // average direction should be close to the main axis
    avg.Normalize();
    XII_TEST_BOOL(avg.IsEqual(vAxis, 0.1));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeRandomDeviation (float)")
  {
    xiiVec3 v;

    xiiRandom rng;
    rng.InitializeFromCurrentTime();

    const xiiAngle  dev          = xiiAngle::MakeFromDegree(65);
    const xiiUInt32 uiNumSamples = 100'000;
    xiiVec3         vAxis;

    for (xiiUInt32 i = 0; i < uiNumSamples; ++i)
    {
      vAxis = xiiVec3::MakeRandomDirection(rng);

      v = xiiVec3::MakeRandomDeviation(rng, dev, vAxis);

      XII_TEST_BOOL(v.IsNormalized());

      XII_TEST_BOOL(vAxis.GetAngleBetween(v).GetDegree() <= dev.GetDegree() + 1.0f);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeRandomDeviation (double)")
  {
    xiiVec3d v;

    xiiRandom rng;
    rng.InitializeFromCurrentTime();

    const xiiAngled dev          = xiiAngled::MakeFromDegree(65);
    const xiiUInt32 uiNumSamples = 100'000;
    xiiVec3d        vAxis;

    for (xiiUInt32 i = 0; i < uiNumSamples; ++i)
    {
      vAxis = xiiVec3d::MakeRandomDirection(rng);

      v = xiiVec3d::MakeRandomDeviation(rng, dev, vAxis);

      XII_TEST_BOOL(v.IsNormalized());

      XII_TEST_BOOL(vAxis.GetAngleBetween(v).GetDegree() <= dev.GetDegree() + 1.0);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "DistanceTo")
  {
    xiiVec3T v1(0.0f, 0.0f, 0.0f);
    xiiVec3T v2(3.0f, 0.0f, 0.0f);
    XII_TEST_FLOAT(v1.GetDistanceTo(v2), 3.0f, xiiMath::SmallEpsilon<xiiMathTestType>());

    v1 = xiiVec3T(1.0f, 2.0f, 3.0f);
    v2 = xiiVec3T(4.0f, 6.0f, 3.0f);
    XII_TEST_FLOAT(v1.GetDistanceTo(v2), 5.0f, xiiMath::SmallEpsilon<xiiMathTestType>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SquaredDistanceTo")
  {
    xiiVec3T v1(0.0f, 0.0f, 0.0f);
    xiiVec3T v2(3.0f, 0.0f, 0.0f);
    XII_TEST_FLOAT(v1.GetSquaredDistanceTo(v2), 9.0f, xiiMath::SmallEpsilon<xiiMathTestType>());

    v1 = xiiVec3T(1.0f, 2.0f, 3.0f);
    v2 = xiiVec3T(4.0f, 6.0f, 3.0f);
    XII_TEST_FLOAT(v1.GetSquaredDistanceTo(v2), 25.0f, xiiMath::SmallEpsilon<xiiMathTestType>());
  }
}
