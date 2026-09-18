/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>

#include <Foundation/Math/FixedPoint.h>

XII_CREATE_SIMPLE_TEST(Math, Vec2)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    if (xiiMath::SupportsNaN<xiiMathTestType>())
    {
      // In debug the default constructor initializes everything with NaN.
      xiiVec2T vDefCtor;
      XII_TEST_BOOL(xiiMath::IsNaN(vDefCtor.x) && xiiMath::IsNaN(vDefCtor.y));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    xiiVec2T::ComponentType testBlock[2] = {(xiiVec2T::ComponentType)1, (xiiVec2T::ComponentType)2};
    xiiVec2T*               pDefCtor     = ::new ((void*)&testBlock[0]) xiiVec2T;
    XII_TEST_BOOL(pDefCtor->x == (xiiVec2T::ComponentType)1 && pDefCtor->y == (xiiVec2T::ComponentType)2);
#endif
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor(x,y)")
  {
    xiiVec2T v(1, 2);
    XII_TEST_FLOAT(v.x, 1, 0);
    XII_TEST_FLOAT(v.y, 2, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor(xy)")
  {
    xiiVec2T v(3);
    XII_TEST_VEC2(v, xiiVec2T(3, 3), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeZero")
  {
    XII_TEST_VEC2(xiiVec2T::MakeZero(), xiiVec2T(0, 0), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Make(x,y)")
  {
    XII_TEST_VEC2(xiiVec2T::Make(4, 5), xiiVec2T(4, 5), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetAsVec3")
  {
    XII_TEST_VEC3(xiiVec2T(2, 3).GetAsVec3(4), xiiVec3T(2, 3, 4), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetAsVec4")
  {
    XII_TEST_VEC4(xiiVec2T(2, 3).GetAsVec4(4, 5), xiiVec4T(2, 3, 4, 5), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Set(x, y)")
  {
    xiiVec2T v;
    v.Set(2, 3);

    XII_TEST_FLOAT(v.x, 2, 0);
    XII_TEST_FLOAT(v.y, 3, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Set(xy)")
  {
    xiiVec2T v;
    v.Set(4);

    XII_TEST_FLOAT(v.x, 4, 0);
    XII_TEST_FLOAT(v.y, 4, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetZero")
  {
    xiiVec2T v;
    v.Set(4);
    v.SetZero();

    XII_TEST_FLOAT(v.x, 0, 0);
    XII_TEST_FLOAT(v.y, 0, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetLength")
  {
    xiiVec2T v(0);
    XII_TEST_FLOAT(v.GetLength(), 0, 0.0001f);

    v.Set(1, 0);
    XII_TEST_FLOAT(v.GetLength(), 1, 0.0001f);

    v.Set(0, 1);
    XII_TEST_FLOAT(v.GetLength(), 1, 0.0001f);

    v.Set(2, 3);
    XII_TEST_FLOAT(v.GetLength(), xiiMath::Sqrt((xiiMathTestType)(4 + 9)), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetLengthSquared")
  {
    xiiVec2T v(0);
    XII_TEST_FLOAT(v.GetLengthSquared(), 0, 0.0001f);

    v.Set(1, 0);
    XII_TEST_FLOAT(v.GetLengthSquared(), 1, 0.0001f);

    v.Set(0, 1);
    XII_TEST_FLOAT(v.GetLengthSquared(), 1, 0.0001f);

    v.Set(2, 3);
    XII_TEST_FLOAT(v.GetLengthSquared(), 4 + 9, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetLengthAndNormalize")
  {
    xiiVec2T                v(0.5f, 0);
    xiiVec2T::ComponentType l = v.GetLengthAndNormalize();
    XII_TEST_FLOAT(l, 0.5f, 0.0001f);
    XII_TEST_FLOAT(v.GetLength(), 1, 0.0001f);

    v.Set(1, 0);
    l = v.GetLengthAndNormalize();
    XII_TEST_FLOAT(l, 1, 0.0001f);
    XII_TEST_FLOAT(v.GetLength(), 1, 0.0001f);

    v.Set(0, 1);
    l = v.GetLengthAndNormalize();
    XII_TEST_FLOAT(l, 1, 0.0001f);
    XII_TEST_FLOAT(v.GetLength(), 1, 0.0001f);

    v.Set(2, 3);
    l = v.GetLengthAndNormalize();
    XII_TEST_FLOAT(l, xiiMath::Sqrt((xiiMathTestType)(4 + 9)), 0.0001f);
    XII_TEST_FLOAT(v.GetLength(), 1, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetNormalized")
  {
    xiiVec2T v;

    v.Set(10, 0);
    XII_TEST_VEC2(v.GetNormalized(), xiiVec2T(1, 0), 0.001f);

    v.Set(0, 10);
    XII_TEST_VEC2(v.GetNormalized(), xiiVec2T(0, 1), 0.001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Normalize")
  {
    xiiVec2T v;

    v.Set(10, 0);
    v.Normalize();
    XII_TEST_VEC2(v, xiiVec2T(1, 0), 0.001f);

    v.Set(0, 10);
    v.Normalize();
    XII_TEST_VEC2(v, xiiVec2T(0, 1), 0.001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "NormalizeIfNotZero")
  {
    xiiVec2T v;

    v.Set(10, 0);
    XII_TEST_BOOL(v.NormalizeIfNotZero() == XII_SUCCESS);
    XII_TEST_VEC2(v, xiiVec2T(1, 0), 0.001f);

    v.Set(0, 10);
    XII_TEST_BOOL(v.NormalizeIfNotZero() == XII_SUCCESS);
    XII_TEST_VEC2(v, xiiVec2T(0, 1), 0.001f);

    v.SetZero();
    XII_TEST_BOOL(v.NormalizeIfNotZero() == XII_FAILURE);
    XII_TEST_VEC2(v, xiiVec2T(1, 0), 0.001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsZero")
  {
    xiiVec2T v;

    v.Set(1);
    XII_TEST_BOOL(v.IsZero() == false);

    v.Set(0.001f);
    XII_TEST_BOOL(v.IsZero() == false);
    XII_TEST_BOOL(v.IsZero(0.01f) == true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsNormalized")
  {
    xiiVec2T v;

    v.SetZero();
    XII_TEST_BOOL(v.IsNormalized(xiiMath::HugeEpsilon<xiiMathTestType>()) == false);

    v.Set(1, 0);
    XII_TEST_BOOL(v.IsNormalized(xiiMath::HugeEpsilon<xiiMathTestType>()) == true);

    v.Set(0, 1);
    XII_TEST_BOOL(v.IsNormalized(xiiMath::HugeEpsilon<xiiMathTestType>()) == true);

    v.Set(0.1f, 1);
    XII_TEST_BOOL(v.IsNormalized(xiiMath::DefaultEpsilon<xiiMathTestType>()) == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsNaN")
  {
    if (xiiMath::SupportsNaN<xiiMathTestType>())
    {
      xiiVec2T v(0);

      XII_TEST_BOOL(!v.IsNaN());

      v.x = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(v.IsNaN());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsValid")
  {
    if (xiiMath::SupportsNaN<xiiMathTestType>())
    {
      xiiVec2T v(0);

      XII_TEST_BOOL(v.IsValid());

      v.x = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(!v.IsValid());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator-")
  {
    xiiVec2T v(1);

    XII_TEST_VEC2(-v, xiiVec2T(-1), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator+=")
  {
    xiiVec2T v(1, 2);

    v += xiiVec2T(3, 4);
    XII_TEST_VEC2(v, xiiVec2T(4, 6), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator-=")
  {
    xiiVec2T v(1, 2);

    v -= xiiVec2T(3, 5);
    XII_TEST_VEC2(v, xiiVec2T(-2, -3), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator*=(float)")
  {
    xiiVec2T v(1, 2);

    v *= 3;
    XII_TEST_VEC2(v, xiiVec2T(3, 6), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator/=(float)")
  {
    xiiVec2T v(1, 2);

    v /= 2;
    XII_TEST_VEC2(v, xiiVec2T(0.5f, 1), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsIdentical")
  {
    xiiVec2T v1(1, 2);
    xiiVec2T v2 = v1;

    XII_TEST_BOOL(v1.IsIdentical(v2));

    v2.x += xiiVec2T::ComponentType(0.001f);
    XII_TEST_BOOL(!v1.IsIdentical(v2));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsEqual")
  {
    xiiVec2T v1(1, 2);
    xiiVec2T v2 = v1;

    XII_TEST_BOOL(v1.IsEqual(v2, 0.00001f));

    v2.x += xiiVec2T::ComponentType(0.001f);
    XII_TEST_BOOL(!v1.IsEqual(v2, 0.0001f));
    XII_TEST_BOOL(v1.IsEqual(v2, 0.01f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetAngleBetween")
  {
    xiiVec2T v1(1, 0);
    xiiVec2T v2(0, 1);

    XII_TEST_FLOAT(v1.GetAngleBetween(v1).GetDegree(), 0, 0.001f);
    XII_TEST_FLOAT(v2.GetAngleBetween(v2).GetDegree(), 0, 0.001f);
    XII_TEST_FLOAT(v1.GetAngleBetween(v2).GetDegree(), 90, 0.001f);
    XII_TEST_FLOAT(v1.GetAngleBetween(-v1).GetDegree(), 180, 0.001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Dot")
  {
    xiiVec2T v1(1, 0);
    xiiVec2T v2(0, 1);
    xiiVec2T v0(0, 0);

    XII_TEST_FLOAT(v0.Dot(v0), 0, 0.001f);
    XII_TEST_FLOAT(v1.Dot(v1), 1, 0.001f);
    XII_TEST_FLOAT(v2.Dot(v2), 1, 0.001f);
    XII_TEST_FLOAT(v1.Dot(v2), 0, 0.001f);
    XII_TEST_FLOAT(v1.Dot(-v1), -1, 0.001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CompMin")
  {
    xiiVec2T v1(2, 3);
    xiiVec2T v2 = v1.CompMin(xiiVec2T(1, 4));
    XII_TEST_VEC2(v2, xiiVec2T(1, 3), 0);

    v2 = v1.CompMin(xiiVec2T(3, 1));
    XII_TEST_VEC2(v2, xiiVec2T(2, 1), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CompMax")
  {
    xiiVec2T v1(2, 3.5f);
    xiiVec2T v2 = v1.CompMax(xiiVec2T(1, 4));
    XII_TEST_VEC2(v2, xiiVec2T(2, 4), 0);

    v2 = v1.CompMax(xiiVec2T(3, 1));
    XII_TEST_VEC2(v2, xiiVec2T(3, 3.5f), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CompClamp")
  {
    const xiiVec2T vOp1(-4.0, 0.2f);
    const xiiVec2T vOp2(2.0, -0.3f);

    XII_TEST_BOOL(vOp1.CompClamp(vOp1, vOp2).IsEqual(xiiVec2T(-4.0f, -0.3f), xiiMath::SmallEpsilon<xiiMathTestType>()));
    XII_TEST_BOOL(vOp2.CompClamp(vOp1, vOp2).IsEqual(xiiVec2T(2.0f, 0.2f), xiiMath::SmallEpsilon<xiiMathTestType>()));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CompMul")
  {
    xiiVec2T v1(2, 3);
    xiiVec2T v2 = v1.CompMul(xiiVec2T(2, 4));
    XII_TEST_VEC2(v2, xiiVec2T(4, 12), 0);

    v2 = v1.CompMul(xiiVec2T(3, 7));
    XII_TEST_VEC2(v2, xiiVec2T(6, 21), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CompDiv")
  {
    xiiVec2T v1(12, 32);
    xiiVec2T v2 = v1.CompDiv(xiiVec2T(3, 4));
    XII_TEST_VEC2(v2, xiiVec2T(4, 8), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Abs")
  {
    xiiVec2T v1(-5, 7);
    xiiVec2T v2 = v1.Abs();
    XII_TEST_VEC2(v2, xiiVec2T(5, 7), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeOrthogonalTo")
  {
    xiiVec2T v;

    v.Set(1, 1);
    v.MakeOrthogonalTo(xiiVec2T(1, 0));
    XII_TEST_VEC2(v, xiiVec2T(0, 1), 0.001f);

    v.Set(1, 1);
    v.MakeOrthogonalTo(xiiVec2T(0, 1));
    XII_TEST_VEC2(v, xiiVec2T(1, 0), 0.001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetOrthogonalVector")
  {
    xiiVec2T v;

    for (float i = 1; i < 360; i += 3)
    {
      v.Set(i, i * 3);
      XII_TEST_FLOAT(v.GetOrthogonalVector().Dot(v), 0, 0.001f);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetReflectedVector")
  {
    xiiVec2T v, v2;

    v.Set(1, 1);
    v2 = v.GetReflectedVector(xiiVec2T(0, -1));
    XII_TEST_VEC2(v2, xiiVec2T(1, -1), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator+")
  {
    xiiVec2T v = xiiVec2T(1, 2) + xiiVec2T(3, 4);
    XII_TEST_VEC2(v, xiiVec2T(4, 6), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator-")
  {
    xiiVec2T v = xiiVec2T(1, 2) - xiiVec2T(3, 5);
    XII_TEST_VEC2(v, xiiVec2T(-2, -3), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator* (vec, float) | operator* (float, vec)")
  {
    xiiVec2T v = xiiVec2T(1, 2) * xiiVec2T::ComponentType(3);
    XII_TEST_VEC2(v, xiiVec2T(3, 6), 0.0001f);

    v = xiiVec2T::ComponentType(7) * xiiVec2T(1, 2);
    XII_TEST_VEC2(v, xiiVec2T(7, 14), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator/ (vec, float)")
  {
    xiiVec2T v = xiiVec2T(2, 4) / xiiVec2T::ComponentType(2);
    XII_TEST_VEC2(v, xiiVec2T(1, 2), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator== | operator!=")
  {
    xiiVec2T v1(1, 2);
    xiiVec2T v2 = v1;

    XII_TEST_BOOL(v1 == v2);

    v2.x += xiiVec2T::ComponentType(0.001f);
    XII_TEST_BOOL(v1 != v2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "DistanceTo")
  {
    xiiVec2T v1(0.0f, 0.0f);
    xiiVec2T v2(3.0f, 4.0f);
    XII_TEST_FLOAT(v1.GetDistanceTo(v2), 5.0f, xiiMath::SmallEpsilon<xiiMathTestType>());

    v1 = xiiVec2T(1.0f, 2.0f);
    v2 = xiiVec2T(4.0f, 6.0f);
    XII_TEST_FLOAT(v1.GetDistanceTo(v2), 5.0f, xiiMath::SmallEpsilon<xiiMathTestType>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SquaredDistanceTo")
  {
    xiiVec2T v1(0.0f, 0.0f);
    xiiVec2T v2(3.0f, 4.0f);
    XII_TEST_FLOAT(v1.GetSquaredDistanceTo(v2), 25.0f, xiiMath::SmallEpsilon<xiiMathTestType>());

    v1 = xiiVec2T(1.0f, 2.0f);
    v2 = xiiVec2T(4.0f, 6.0f);
    XII_TEST_FLOAT(v1.GetSquaredDistanceTo(v2), 25.0f, xiiMath::SmallEpsilon<xiiMathTestType>());
  }
}
