#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/SimdMath/SimdConversion.h>

#define XII_TEST_SIMD_VECTOR_EQUAL(NUM_COMPONENTS, A, B, EPSILON)                                                                                                  \
  do                                                                                                                                                               \
  {                                                                                                                                                                \
    auto _xiiDiff = B - A;                                                                                                                                         \
    xiiTestBool((A).IsEqual((B), EPSILON).AllSet<NUM_COMPONENTS>(), "Test failed: " XII_STRINGIZE(A) ".IsEqual(" XII_STRINGIZE(B) ", " XII_STRINGIZE(EPSILON) ")", \
                XII_SOURCE_FILE, XII_SOURCE_LINE, XII_SOURCE_FUNCTION,                                                                                             \
                "Difference %lf %lf %lf %lf", _xiiDiff.x(), _xiiDiff.y(), _xiiDiff.z(), _xiiDiff.w());                                                             \
  } while (false)


XII_CREATE_SIMPLE_TEST(SimdMath, SimdBBox)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
    xiiSimdBBox b(xiiSimdVec4f(-1, -2, -3), xiiSimdVec4f(1, 2, 3));

    XII_TEST_BOOL((b.m_Min == xiiSimdVec4f(-1, -2, -3)).AllSet<3>());
    XII_TEST_BOOL((b.m_Max == xiiSimdVec4f(1, 2, 3)).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeInvalid")
  {
    xiiSimdBBox b = xiiSimdBBox::MakeInvalid();

    XII_TEST_BOOL(!b.IsValid());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsNaN")
  {
    xiiSimdBBox b = xiiSimdBBox::MakeInvalid();

    b = xiiSimdBBox::MakeInvalid();
    XII_TEST_BOOL(!b.IsNaN());

    b = xiiSimdBBox::MakeInvalid();
    b.m_Min.SetX(xiiMath::NaN<float>());
    XII_TEST_BOOL(b.IsNaN());

    b = xiiSimdBBox::MakeInvalid();
    b.m_Min.SetY(xiiMath::NaN<float>());
    XII_TEST_BOOL(b.IsNaN());

    b = xiiSimdBBox::MakeInvalid();
    b.m_Min.SetZ(xiiMath::NaN<float>());
    XII_TEST_BOOL(b.IsNaN());

    b = xiiSimdBBox::MakeInvalid();
    b.m_Max.SetX(xiiMath::NaN<float>());
    XII_TEST_BOOL(b.IsNaN());

    b = xiiSimdBBox::MakeInvalid();
    b.m_Max.SetY(xiiMath::NaN<float>());
    XII_TEST_BOOL(b.IsNaN());

    b = xiiSimdBBox::MakeInvalid();
    b.m_Max.SetZ(xiiMath::NaN<float>());
    XII_TEST_BOOL(b.IsNaN());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeFromCenterAndHalfExtents")
  {
    const xiiSimdBBox b = xiiSimdBBox::MakeFromCenterAndHalfExtents(xiiSimdVec4f(1, 2, 3), xiiSimdVec4f(4, 5, 6));

    XII_TEST_BOOL((b.m_Min == xiiSimdVec4f(-3, -3, -3)).AllSet<3>());
    XII_TEST_BOOL((b.m_Max == xiiSimdVec4f(5, 7, 9)).AllSet<3>());

    XII_TEST_BOOL((b.GetCenter() == xiiSimdVec4f(1, 2, 3)).AllSet<3>());
    XII_TEST_BOOL((b.GetExtents() == xiiSimdVec4f(8, 10, 12)).AllSet<3>());
    XII_TEST_BOOL((b.GetHalfExtents() == xiiSimdVec4f(4, 5, 6)).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeFromPoints")
  {
    xiiSimdVec4f p[6] = {
      xiiSimdVec4f(-4, 0, 0),
      xiiSimdVec4f(5, 0, 0),
      xiiSimdVec4f(0, -6, 0),
      xiiSimdVec4f(0, 7, 0),
      xiiSimdVec4f(0, 0, -8),
      xiiSimdVec4f(0, 0, 9),
    };

    const xiiSimdBBox b = xiiSimdBBox::MakeFromPoints(p, 6);

    XII_TEST_BOOL((b.m_Min == xiiSimdVec4f(-4, -6, -8)).AllSet<3>());
    XII_TEST_BOOL((b.m_Max == xiiSimdVec4f(5, 7, 9)).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExpandToInclude (Point)")
  {
    xiiSimdBBox b = xiiSimdBBox::MakeInvalid();
    b.ExpandToInclude(xiiSimdVec4f(1, 2, 3));

    XII_TEST_BOOL((b.m_Min == xiiSimdVec4f(1, 2, 3)).AllSet<3>());
    XII_TEST_BOOL((b.m_Max == xiiSimdVec4f(1, 2, 3)).AllSet<3>());


    b.ExpandToInclude(xiiSimdVec4f(2, 3, 4));

    XII_TEST_BOOL((b.m_Min == xiiSimdVec4f(1, 2, 3)).AllSet<3>());
    XII_TEST_BOOL((b.m_Max == xiiSimdVec4f(2, 3, 4)).AllSet<3>());

    b.ExpandToInclude(xiiSimdVec4f(0, 1, 2));

    XII_TEST_BOOL((b.m_Min == xiiSimdVec4f(0, 1, 2)).AllSet<3>());
    XII_TEST_BOOL((b.m_Max == xiiSimdVec4f(2, 3, 4)).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExpandToInclude (array)")
  {
    xiiSimdVec4f v[4] = {xiiSimdVec4f(1, 1, 1), xiiSimdVec4f(-1, -1, -1), xiiSimdVec4f(2, 2, 2), xiiSimdVec4f(4, 4, 4)};

    xiiSimdBBox b = xiiSimdBBox::MakeInvalid();
    b.ExpandToInclude(v, 2, sizeof(xiiSimdVec4f) * 2);

    XII_TEST_BOOL((b.m_Min == xiiSimdVec4f(1, 1, 1)).AllSet<3>());
    XII_TEST_BOOL((b.m_Max == xiiSimdVec4f(2, 2, 2)).AllSet<3>());

    b.ExpandToInclude(v, 4);

    XII_TEST_BOOL((b.m_Min == xiiSimdVec4f(-1, -1, -1)).AllSet<3>());
    XII_TEST_BOOL((b.m_Max == xiiSimdVec4f(4, 4, 4)).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExpandToInclude (Box)")
  {
    xiiSimdBBox b1(xiiSimdVec4f(-1, -2, -3), xiiSimdVec4f(1, 2, 3));
    xiiSimdBBox b2(xiiSimdVec4f(0), xiiSimdVec4f(4, 5, 6));

    b1.ExpandToInclude(b2);

    XII_TEST_BOOL((b1.m_Min == xiiSimdVec4f(-1, -2, -3)).AllSet<3>());
    XII_TEST_BOOL((b1.m_Max == xiiSimdVec4f(4, 5, 6)).AllSet<3>());

    xiiSimdBBox b3 = xiiSimdBBox::MakeInvalid();
    b3.ExpandToInclude(b1);
    XII_TEST_BOOL(b3 == b1);

    b2.m_Min = xiiSimdVec4f(-4, -5, -6);
    b2.m_Max.SetZero();

    b1.ExpandToInclude(b2);

    XII_TEST_BOOL((b1.m_Min == xiiSimdVec4f(-4, -5, -6)).AllSet<3>());
    XII_TEST_BOOL((b1.m_Max == xiiSimdVec4f(4, 5, 6)).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExpandToCube")
  {
    xiiSimdBBox b = xiiSimdBBox::MakeFromCenterAndHalfExtents(xiiSimdVec4f(1, 2, 3), xiiSimdVec4f(4, 5, 6));

    b.ExpandToCube();

    XII_TEST_BOOL((b.GetCenter() == xiiSimdVec4f(1, 2, 3)).AllSet<3>());
    XII_TEST_BOOL((b.GetHalfExtents() == xiiSimdVec4f(6, 6, 6)).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Contains (Point)")
  {
    xiiSimdBBox b(xiiSimdVec4f(0), xiiSimdVec4f(0));

    XII_TEST_BOOL(b.Contains(xiiSimdVec4f(0)));
    XII_TEST_BOOL(!b.Contains(xiiSimdVec4f(1, 0, 0)));
    XII_TEST_BOOL(!b.Contains(xiiSimdVec4f(-1, 0, 0)));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Contains (Box)")
  {
    xiiSimdBBox b1(xiiSimdVec4f(-3), xiiSimdVec4f(3));
    xiiSimdBBox b2(xiiSimdVec4f(-1), xiiSimdVec4f(1));
    xiiSimdBBox b3(xiiSimdVec4f(-1), xiiSimdVec4f(4));

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
    xiiSimdBBox b(xiiSimdVec4f(1), xiiSimdVec4f(5));

    XII_TEST_BOOL(b.Contains(xiiSimdBSphere(xiiSimdVec4f(3), 2)));
    XII_TEST_BOOL(!b.Contains(xiiSimdBSphere(xiiSimdVec4f(3), 2.1f)));
    XII_TEST_BOOL(!b.Contains(xiiSimdBSphere(xiiSimdVec4f(8), 2)));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Overlaps (box)")
  {
    xiiSimdBBox b1(xiiSimdVec4f(-3), xiiSimdVec4f(3));
    xiiSimdBBox b2(xiiSimdVec4f(-1), xiiSimdVec4f(1));
    xiiSimdBBox b3(xiiSimdVec4f(1), xiiSimdVec4f(4));
    xiiSimdBBox b4(xiiSimdVec4f(-4, 1, 1), xiiSimdVec4f(4, 2, 2));

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
    xiiSimdBBox b(xiiSimdVec4f(1), xiiSimdVec4f(5));

    XII_TEST_BOOL(b.Overlaps(xiiSimdBSphere(xiiSimdVec4f(3), 2)));
    XII_TEST_BOOL(b.Overlaps(xiiSimdBSphere(xiiSimdVec4f(3), 2.1f)));
    XII_TEST_BOOL(!b.Overlaps(xiiSimdBSphere(xiiSimdVec4f(8), 2)));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Grow")
  {
    xiiSimdBBox b(xiiSimdVec4f(1, 2, 3), xiiSimdVec4f(4, 5, 6));
    b.Grow(xiiSimdVec4f(2, 4, 6));

    XII_TEST_BOOL((b.m_Min == xiiSimdVec4f(-1, -2, -3)).AllSet<3>());
    XII_TEST_BOOL((b.m_Max == xiiSimdVec4f(6, 9, 12)).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Transform")
  {
    xiiSimdBBox b(xiiSimdVec4f(3), xiiSimdVec4f(5));

    xiiSimdTransform t(xiiSimdVec4f(4, 5, 6));
    t.m_Rotation = xiiSimdQuat::MakeFromAxisAndAngle(xiiSimdVec4f(0, 0, 1), xiiAngle::MakeFromDegree(90));
    t.m_Scale    = xiiSimdVec4f(1, -2, -4);

    b.Transform(t);

    XII_TEST_SIMD_VECTOR_EQUAL(3, b.m_Min, xiiSimdVec4f(10, 8, -14), 0.00001f);
    XII_TEST_SIMD_VECTOR_EQUAL(3, b.m_Max, xiiSimdVec4f(14, 10, -6), 0.00001f);

    t.m_Rotation = xiiSimdQuat::MakeFromAxisAndAngle(xiiSimdVec4f(0, 0, 1), xiiAngle::MakeFromDegree(-30));

    b.m_Min = xiiSimdVec4f(3);
    b.m_Max = xiiSimdVec4f(5);
    b.Transform(t);

    // reference
    xiiBoundingBox referenceBox = xiiBoundingBox::MakeFromMinMax(xiiVec3(3), xiiVec3(5));
    {
      xiiQuat q = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 0, 1), xiiAngle::MakeFromDegree(-30));

      xiiTransform referenceTransform(xiiVec3(4, 5, 6), q, xiiVec3(1, -2, -4));

      referenceBox.TransformFromOrigin(referenceTransform.GetAsMat4());
    }

    XII_TEST_SIMD_VECTOR_EQUAL(3, b.m_Min, xiiSimdConversion::ToVec3(referenceBox.m_vMin), 0.00001f);
    XII_TEST_SIMD_VECTOR_EQUAL(3, b.m_Max, xiiSimdConversion::ToVec3(referenceBox.m_vMax), 0.00001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetClampedPoint")
  {
    xiiSimdBBox b(xiiSimdVec4f(-1, -2, -3), xiiSimdVec4f(1, 2, 3));

    XII_TEST_BOOL((b.GetClampedPoint(xiiSimdVec4f(-2, 0, 0)) == xiiSimdVec4f(-1, 0, 0)).AllSet<3>());
    XII_TEST_BOOL((b.GetClampedPoint(xiiSimdVec4f(2, 0, 0)) == xiiSimdVec4f(1, 0, 0)).AllSet<3>());

    XII_TEST_BOOL((b.GetClampedPoint(xiiSimdVec4f(0, -3, 0)) == xiiSimdVec4f(0, -2, 0)).AllSet<3>());
    XII_TEST_BOOL((b.GetClampedPoint(xiiSimdVec4f(0, 3, 0)) == xiiSimdVec4f(0, 2, 0)).AllSet<3>());

    XII_TEST_BOOL((b.GetClampedPoint(xiiSimdVec4f(0, 0, -4)) == xiiSimdVec4f(0, 0, -3)).AllSet<3>());
    XII_TEST_BOOL((b.GetClampedPoint(xiiSimdVec4f(0, 0, 4)) == xiiSimdVec4f(0, 0, 3)).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetDistanceSquaredTo (point)")
  {
    xiiSimdBBox b(xiiSimdVec4f(-1, -2, -3), xiiSimdVec4f(1, 2, 3));

    XII_TEST_BOOL(b.GetDistanceSquaredTo(xiiSimdVec4f(-2, 0, 0)) == 1.0f);
    XII_TEST_BOOL(b.GetDistanceSquaredTo(xiiSimdVec4f(2, 0, 0)) == 1.0f);

    XII_TEST_BOOL(b.GetDistanceSquaredTo(xiiSimdVec4f(0, -4, 0)) == 4.0f);
    XII_TEST_BOOL(b.GetDistanceSquaredTo(xiiSimdVec4f(0, 4, 0)) == 4.0f);

    XII_TEST_BOOL(b.GetDistanceSquaredTo(xiiSimdVec4f(0, 0, -6)) == 9.0f);
    XII_TEST_BOOL(b.GetDistanceSquaredTo(xiiSimdVec4f(0, 0, 6)) == 9.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetDistanceTo (point)")
  {
    xiiSimdBBox b(xiiSimdVec4f(-1, -2, -3), xiiSimdVec4f(1, 2, 3));

    XII_TEST_BOOL(b.GetDistanceTo(xiiSimdVec4f(-2, 0, 0)) == 1.0f);
    XII_TEST_BOOL(b.GetDistanceTo(xiiSimdVec4f(2, 0, 0)) == 1.0f);

    XII_TEST_BOOL(b.GetDistanceTo(xiiSimdVec4f(0, -4, 0)) == 2.0f);
    XII_TEST_BOOL(b.GetDistanceTo(xiiSimdVec4f(0, 4, 0)) == 2.0f);

    XII_TEST_BOOL(b.GetDistanceTo(xiiSimdVec4f(0, 0, -6)) == 3.0f);
    XII_TEST_BOOL(b.GetDistanceTo(xiiSimdVec4f(0, 0, 6)) == 3.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Comparison")
  {
    xiiSimdBBox b1(xiiSimdVec4f(5, 0, 0), xiiSimdVec4f(1, 2, 3));
    xiiSimdBBox b2(xiiSimdVec4f(6, 0, 0), xiiSimdVec4f(1, 2, 3));

    XII_TEST_BOOL(b1 == xiiSimdBBox(xiiSimdVec4f(5, 0, 0), xiiSimdVec4f(1, 2, 3)));
    XII_TEST_BOOL(b1 != b2);
  }
}
