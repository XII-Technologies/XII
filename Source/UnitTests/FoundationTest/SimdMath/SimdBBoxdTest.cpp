/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/SimdMath/SimdBBoxd.h>
#include <Foundation/SimdMath/SimdConversion.h>

#define XII_TEST_SIMD_VECTOR_EQUAL(NUM_COMPONENTS, A, B, EPSILON)                                                                                                           \
  do                                                                                                                                                                        \
  {                                                                                                                                                                         \
    auto _xiiDiff = B - A;                                                                                                                                                  \
    xiiTestBool((A).IsEqual((B), EPSILON).AllSet<NUM_COMPONENTS>(), "Test failed: " XII_PP_STRINGIFY(A) ".IsEqual(" XII_PP_STRINGIFY(B) ", " XII_PP_STRINGIFY(EPSILON) ")", \
                XII_SOURCE_FILE, XII_SOURCE_LINE, XII_SOURCE_FUNCTION,                                                                                                      \
                "Difference %lf %lf %lf %lf", _xiiDiff.x(), _xiiDiff.y(), _xiiDiff.z(), _xiiDiff.w());                                                                      \
  } while (false)


XII_CREATE_SIMPLE_TEST(SimdMath, SimdBBoxd)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
    xiiSimdBBoxd b(xiiSimdVec4d(-1, -2, -3), xiiSimdVec4d(1, 2, 3));

    XII_TEST_BOOL((b.m_Min == xiiSimdVec4d(-1, -2, -3)).AllSet<3>());
    XII_TEST_BOOL((b.m_Max == xiiSimdVec4d(1, 2, 3)).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeInvalid")
  {
    xiiSimdBBoxd b = xiiSimdBBoxd::MakeInvalid();

    XII_TEST_BOOL(!b.IsValid());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsNaN")
  {
    xiiSimdBBoxd b = xiiSimdBBoxd::MakeInvalid();

    b = xiiSimdBBoxd::MakeInvalid();
    XII_TEST_BOOL(!b.IsNaN());

    b = xiiSimdBBoxd::MakeInvalid();
    b.m_Min.SetX(xiiMath::NaN<double>());
    XII_TEST_BOOL(b.IsNaN());

    b = xiiSimdBBoxd::MakeInvalid();
    b.m_Min.SetY(xiiMath::NaN<double>());
    XII_TEST_BOOL(b.IsNaN());

    b = xiiSimdBBoxd::MakeInvalid();
    b.m_Min.SetZ(xiiMath::NaN<double>());
    XII_TEST_BOOL(b.IsNaN());

    b = xiiSimdBBoxd::MakeInvalid();
    b.m_Max.SetX(xiiMath::NaN<double>());
    XII_TEST_BOOL(b.IsNaN());

    b = xiiSimdBBoxd::MakeInvalid();
    b.m_Max.SetY(xiiMath::NaN<double>());
    XII_TEST_BOOL(b.IsNaN());

    b = xiiSimdBBoxd::MakeInvalid();
    b.m_Max.SetZ(xiiMath::NaN<double>());
    XII_TEST_BOOL(b.IsNaN());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeFromCenterAndHalfExtents")
  {
    const xiiSimdBBoxd b = xiiSimdBBoxd::MakeFromCenterAndHalfExtents(xiiSimdVec4d(1, 2, 3), xiiSimdVec4d(4, 5, 6));

    XII_TEST_BOOL((b.m_Min == xiiSimdVec4d(-3, -3, -3)).AllSet<3>());
    XII_TEST_BOOL((b.m_Max == xiiSimdVec4d(5, 7, 9)).AllSet<3>());

    XII_TEST_BOOL((b.GetCenter() == xiiSimdVec4d(1, 2, 3)).AllSet<3>());
    XII_TEST_BOOL((b.GetExtents() == xiiSimdVec4d(8, 10, 12)).AllSet<3>());
    XII_TEST_BOOL((b.GetHalfExtents() == xiiSimdVec4d(4, 5, 6)).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeFromPoints")
  {
    xiiSimdVec4d p[6] = {
      xiiSimdVec4d(-4, 0, 0),
      xiiSimdVec4d(5, 0, 0),
      xiiSimdVec4d(0, -6, 0),
      xiiSimdVec4d(0, 7, 0),
      xiiSimdVec4d(0, 0, -8),
      xiiSimdVec4d(0, 0, 9),
    };

    const xiiSimdBBoxd b = xiiSimdBBoxd::MakeFromPoints(p, 6);

    XII_TEST_BOOL((b.m_Min == xiiSimdVec4d(-4, -6, -8)).AllSet<3>());
    XII_TEST_BOOL((b.m_Max == xiiSimdVec4d(5, 7, 9)).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExpandToInclude (Point)")
  {
    xiiSimdBBoxd b = xiiSimdBBoxd::MakeInvalid();
    b.ExpandToInclude(xiiSimdVec4d(1, 2, 3));

    XII_TEST_BOOL((b.m_Min == xiiSimdVec4d(1, 2, 3)).AllSet<3>());
    XII_TEST_BOOL((b.m_Max == xiiSimdVec4d(1, 2, 3)).AllSet<3>());


    b.ExpandToInclude(xiiSimdVec4d(2, 3, 4));

    XII_TEST_BOOL((b.m_Min == xiiSimdVec4d(1, 2, 3)).AllSet<3>());
    XII_TEST_BOOL((b.m_Max == xiiSimdVec4d(2, 3, 4)).AllSet<3>());

    b.ExpandToInclude(xiiSimdVec4d(0, 1, 2));

    XII_TEST_BOOL((b.m_Min == xiiSimdVec4d(0, 1, 2)).AllSet<3>());
    XII_TEST_BOOL((b.m_Max == xiiSimdVec4d(2, 3, 4)).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExpandToInclude (array)")
  {
    xiiSimdVec4d v[4] = {xiiSimdVec4d(1, 1, 1), xiiSimdVec4d(-1, -1, -1), xiiSimdVec4d(2, 2, 2), xiiSimdVec4d(4, 4, 4)};

    xiiSimdBBoxd b = xiiSimdBBoxd::MakeInvalid();
    b.ExpandToInclude(v, 2, sizeof(xiiSimdVec4d) * 2);

    XII_TEST_BOOL((b.m_Min == xiiSimdVec4d(1, 1, 1)).AllSet<3>());
    XII_TEST_BOOL((b.m_Max == xiiSimdVec4d(2, 2, 2)).AllSet<3>());

    b.ExpandToInclude(v, 4);

    XII_TEST_BOOL((b.m_Min == xiiSimdVec4d(-1, -1, -1)).AllSet<3>());
    XII_TEST_BOOL((b.m_Max == xiiSimdVec4d(4, 4, 4)).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExpandToInclude (Box)")
  {
    xiiSimdBBoxd b1(xiiSimdVec4d(-1, -2, -3), xiiSimdVec4d(1, 2, 3));
    xiiSimdBBoxd b2(xiiSimdVec4d(0), xiiSimdVec4d(4, 5, 6));

    b1.ExpandToInclude(b2);

    XII_TEST_BOOL((b1.m_Min == xiiSimdVec4d(-1, -2, -3)).AllSet<3>());
    XII_TEST_BOOL((b1.m_Max == xiiSimdVec4d(4, 5, 6)).AllSet<3>());

    xiiSimdBBoxd b3 = xiiSimdBBoxd::MakeInvalid();
    b3.ExpandToInclude(b1);
    XII_TEST_BOOL(b3 == b1);

    b2.m_Min = xiiSimdVec4d(-4, -5, -6);
    b2.m_Max.SetZero();

    b1.ExpandToInclude(b2);

    XII_TEST_BOOL((b1.m_Min == xiiSimdVec4d(-4, -5, -6)).AllSet<3>());
    XII_TEST_BOOL((b1.m_Max == xiiSimdVec4d(4, 5, 6)).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExpandToCube")
  {
    xiiSimdBBoxd b = xiiSimdBBoxd::MakeFromCenterAndHalfExtents(xiiSimdVec4d(1, 2, 3), xiiSimdVec4d(4, 5, 6));

    b.ExpandToCube();

    XII_TEST_BOOL((b.GetCenter() == xiiSimdVec4d(1, 2, 3)).AllSet<3>());
    XII_TEST_BOOL((b.GetHalfExtents() == xiiSimdVec4d(6, 6, 6)).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Contains (Point)")
  {
    xiiSimdBBoxd b(xiiSimdVec4d(0), xiiSimdVec4d(0));

    XII_TEST_BOOL(b.Contains(xiiSimdVec4d(0)));
    XII_TEST_BOOL(!b.Contains(xiiSimdVec4d(1, 0, 0)));
    XII_TEST_BOOL(!b.Contains(xiiSimdVec4d(-1, 0, 0)));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Contains (Box)")
  {
    xiiSimdBBoxd b1(xiiSimdVec4d(-3), xiiSimdVec4d(3));
    xiiSimdBBoxd b2(xiiSimdVec4d(-1), xiiSimdVec4d(1));
    xiiSimdBBoxd b3(xiiSimdVec4d(-1), xiiSimdVec4d(4));

    XII_TEST_BOOL(b1.Contains(b1));
    XII_TEST_BOOL(b2.Contains(b2));
    XII_TEST_BOOL(b3.Contains(b3));

    XII_TEST_BOOL(b1.Contains(b2));
    XII_TEST_BOOL(!b1.Contains(b3));

    XII_TEST_BOOL(!b2.Contains(b1));
    XII_TEST_BOOL(!b2.Contains(b3));

    XII_TEST_BOOL(!b3.Contains(b1));
    XII_TEST_BOOL(b3.Contains(b2));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Contains (Sphere)")
  {
    xiiSimdBBoxd b(xiiSimdVec4d(1), xiiSimdVec4d(5));

    XII_TEST_BOOL(b.Contains(xiiSimdBSphered(xiiSimdVec4d(3), 2)));
    XII_TEST_BOOL(!b.Contains(xiiSimdBSphered(xiiSimdVec4d(3), 2.1)));
    XII_TEST_BOOL(!b.Contains(xiiSimdBSphered(xiiSimdVec4d(8), 2)));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Overlaps (box)")
  {
    xiiSimdBBoxd b1(xiiSimdVec4d(-3), xiiSimdVec4d(3));
    xiiSimdBBoxd b2(xiiSimdVec4d(-1), xiiSimdVec4d(1));
    xiiSimdBBoxd b3(xiiSimdVec4d(1), xiiSimdVec4d(4));
    xiiSimdBBoxd b4(xiiSimdVec4d(-4, 1, 1), xiiSimdVec4d(4, 2, 2));

    XII_TEST_BOOL(b1.Overlaps(b1));
    XII_TEST_BOOL(b2.Overlaps(b2));
    XII_TEST_BOOL(b3.Overlaps(b3));
    XII_TEST_BOOL(b4.Overlaps(b4));

    XII_TEST_BOOL(b1.Overlaps(b2));
    XII_TEST_BOOL(b1.Overlaps(b3));
    XII_TEST_BOOL(b1.Overlaps(b4));

    XII_TEST_BOOL(!b2.Overlaps(b3));
    XII_TEST_BOOL(!b2.Overlaps(b4));

    XII_TEST_BOOL(b3.Overlaps(b4));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Overlaps (Sphere)")
  {
    xiiSimdBBoxd b(xiiSimdVec4d(1), xiiSimdVec4d(5));

    XII_TEST_BOOL(b.Overlaps(xiiSimdBSphered(xiiSimdVec4d(3), 2)));
    XII_TEST_BOOL(b.Overlaps(xiiSimdBSphered(xiiSimdVec4d(3), 2.1)));
    XII_TEST_BOOL(!b.Overlaps(xiiSimdBSphered(xiiSimdVec4d(8), 2)));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Grow")
  {
    xiiSimdBBoxd b(xiiSimdVec4d(1, 2, 3), xiiSimdVec4d(4, 5, 6));
    b.Grow(xiiSimdVec4d(2, 4, 6));

    XII_TEST_BOOL((b.m_Min == xiiSimdVec4d(-1, -2, -3)).AllSet<3>());
    XII_TEST_BOOL((b.m_Max == xiiSimdVec4d(6, 9, 12)).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Transform")
  {
    xiiSimdBBoxd b(xiiSimdVec4d(3), xiiSimdVec4d(5));

    xiiSimdTransformd t(xiiSimdVec4d(4, 5, 6));
    t.m_Rotation = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 0, 1), xiiAngled::MakeFromDegree(90));
    t.m_Scale    = xiiSimdVec4d(1, -2, -4);

    b.Transform(t);

    XII_TEST_SIMD_VECTOR_EQUAL(3, b.m_Min, xiiSimdVec4d(10, 8, -14), 0.00001);
    XII_TEST_SIMD_VECTOR_EQUAL(3, b.m_Max, xiiSimdVec4d(14, 10, -6), 0.00001);

    t.m_Rotation = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 0, 1), xiiAngled::MakeFromDegree(-30));

    b.m_Min = xiiSimdVec4d(3);
    b.m_Max = xiiSimdVec4d(5);
    b.Transform(t);

    // reference
    xiiBoundingBoxd referenceBox = xiiBoundingBoxd::MakeFromMinMax(xiiVec3d(3), xiiVec3d(5));
    {
      xiiQuatd q = xiiQuatd::MakeFromAxisAndAngle(xiiVec3d(0, 0, 1), xiiAngled::MakeFromDegree(-30));

      xiiTransformd referenceTransform(xiiVec3d(4, 5, 6), q, xiiVec3d(1, -2, -4));

      referenceBox.TransformFromOrigin(referenceTransform.GetAsMat4());
    }

    XII_TEST_SIMD_VECTOR_EQUAL(3, b.m_Min, xiiSimdConversion::ToVec3(referenceBox.m_vMin), 0.00001);
    XII_TEST_SIMD_VECTOR_EQUAL(3, b.m_Max, xiiSimdConversion::ToVec3(referenceBox.m_vMax), 0.00001);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetClampedPoint")
  {
    xiiSimdBBoxd b(xiiSimdVec4d(-1, -2, -3), xiiSimdVec4d(1, 2, 3));

    XII_TEST_BOOL((b.GetClampedPoint(xiiSimdVec4d(-2, 0, 0)) == xiiSimdVec4d(-1, 0, 0)).AllSet<3>());
    XII_TEST_BOOL((b.GetClampedPoint(xiiSimdVec4d(2, 0, 0)) == xiiSimdVec4d(1, 0, 0)).AllSet<3>());

    XII_TEST_BOOL((b.GetClampedPoint(xiiSimdVec4d(0, -3, 0)) == xiiSimdVec4d(0, -2, 0)).AllSet<3>());
    XII_TEST_BOOL((b.GetClampedPoint(xiiSimdVec4d(0, 3, 0)) == xiiSimdVec4d(0, 2, 0)).AllSet<3>());

    XII_TEST_BOOL((b.GetClampedPoint(xiiSimdVec4d(0, 0, -4)) == xiiSimdVec4d(0, 0, -3)).AllSet<3>());
    XII_TEST_BOOL((b.GetClampedPoint(xiiSimdVec4d(0, 0, 4)) == xiiSimdVec4d(0, 0, 3)).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetDistanceSquaredTo (point)")
  {
    xiiSimdBBoxd b(xiiSimdVec4d(-1, -2, -3), xiiSimdVec4d(1, 2, 3));

    XII_TEST_BOOL(b.GetDistanceSquaredTo(xiiSimdVec4d(-2, 0, 0)) == 1.0);
    XII_TEST_BOOL(b.GetDistanceSquaredTo(xiiSimdVec4d(2, 0, 0)) == 1.0);

    XII_TEST_BOOL(b.GetDistanceSquaredTo(xiiSimdVec4d(0, -4, 0)) == 4.0);
    XII_TEST_BOOL(b.GetDistanceSquaredTo(xiiSimdVec4d(0, 4, 0)) == 4.0);

    XII_TEST_BOOL(b.GetDistanceSquaredTo(xiiSimdVec4d(0, 0, -6)) == 9.0);
    XII_TEST_BOOL(b.GetDistanceSquaredTo(xiiSimdVec4d(0, 0, 6)) == 9.0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetDistanceTo (point)")
  {
    xiiSimdBBoxd b(xiiSimdVec4d(-1, -2, -3), xiiSimdVec4d(1, 2, 3));

    XII_TEST_BOOL(b.GetDistanceTo(xiiSimdVec4d(-2, 0, 0)) == 1.0);
    XII_TEST_BOOL(b.GetDistanceTo(xiiSimdVec4d(2, 0, 0)) == 1.0);

    XII_TEST_BOOL(b.GetDistanceTo(xiiSimdVec4d(0, -4, 0)) == 2.0);
    XII_TEST_BOOL(b.GetDistanceTo(xiiSimdVec4d(0, 4, 0)) == 2.0);

    XII_TEST_BOOL(b.GetDistanceTo(xiiSimdVec4d(0, 0, -6)) == 3.0);
    XII_TEST_BOOL(b.GetDistanceTo(xiiSimdVec4d(0, 0, 6)) == 3.0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Comparison")
  {
    xiiSimdBBoxd b1(xiiSimdVec4d(5, 0, 0), xiiSimdVec4d(1, 2, 3));
    xiiSimdBBoxd b2(xiiSimdVec4d(6, 0, 0), xiiSimdVec4d(1, 2, 3));

    XII_TEST_BOOL(b1 == xiiSimdBBoxd(xiiSimdVec4d(5, 0, 0), xiiSimdVec4d(1, 2, 3)));
    XII_TEST_BOOL(b1 != b2);
  }
}
