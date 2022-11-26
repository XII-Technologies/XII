#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdBSphere.h>

XII_CREATE_SIMPLE_TEST(SimdMath, SimdBSphere)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
    xiiSimdBSphere s(xiiSimdVec4f(1, 2, 3), 4);

    XII_TEST_BOOL((s.m_CenterAndRadius == xiiSimdVec4f(1, 2, 3, 4)).AllSet());

    XII_TEST_BOOL((s.GetCenter() == xiiSimdVec4f(1, 2, 3)).AllSet<3>());
    XII_TEST_BOOL(s.GetRadius() == 4.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetInvalid / IsValid")
  {
    xiiSimdBSphere s(xiiSimdVec4f(1, 2, 3), 4);

    XII_TEST_BOOL(s.IsValid());

    s.SetInvalid();

    XII_TEST_BOOL(!s.IsValid());
    XII_TEST_BOOL(!s.IsNaN());

    s = xiiSimdBSphere(xiiSimdVec4f(1, 2, 3), xiiMath::NaN<float>());
    XII_TEST_BOOL(s.IsNaN());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExpandToInclude(Point)")
  {
    xiiSimdBSphere s(xiiSimdVec4f::ZeroVector(), 0.0f);

    s.ExpandToInclude(xiiSimdVec4f(3, 0, 0));

    XII_TEST_BOOL((s.m_CenterAndRadius == xiiSimdVec4f(0, 0, 0, 3)).AllSet());

    s.SetInvalid();

    s.ExpandToInclude(xiiSimdVec4f(0.25, 0, 0));

    XII_TEST_BOOL((s.m_CenterAndRadius == xiiSimdVec4f(0, 0, 0, 0.25)).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExpandToInclude(array)")
  {
    xiiSimdBSphere s(xiiSimdVec4f(2, 2, 0), 0.0f);

    xiiSimdVec4f p[4] = {xiiSimdVec4f(0, 2, 0), xiiSimdVec4f(4, 2, 0), xiiSimdVec4f(2, 0, 0), xiiSimdVec4f(2, 4, 0)};

    s.ExpandToInclude(p, 4);

    XII_TEST_BOOL((s.m_CenterAndRadius == xiiSimdVec4f(2, 2, 0, 2)).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExpandToInclude (sphere)")
  {
    xiiSimdBSphere s1(xiiSimdVec4f(5, 0, 0), 1);
    xiiSimdBSphere s2(xiiSimdVec4f(6, 0, 0), 1);
    xiiSimdBSphere s3(xiiSimdVec4f(5, 0, 0), 2);

    s1.ExpandToInclude(s2);
    XII_TEST_BOOL((s1.m_CenterAndRadius == xiiSimdVec4f(5, 0, 0, 2)).AllSet());

    s1.ExpandToInclude(s3);
    XII_TEST_BOOL((s1.m_CenterAndRadius == xiiSimdVec4f(5, 0, 0, 2)).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Transform")
  {
    xiiSimdBSphere s(xiiSimdVec4f(5, 0, 0), 2);

    xiiSimdTransform t(xiiSimdVec4f(4, 5, 6));
    t.m_Rotation.SetFromAxisAndAngle(xiiSimdVec4f(0, 0, 1), xiiAngle::Degree(90));
    t.m_Scale = xiiSimdVec4f(1, -2, -4);

    s.Transform(t);
    XII_TEST_BOOL(s.m_CenterAndRadius.IsEqual(xiiSimdVec4f(4, 10, 6, 8), xiiSimdFloat(xiiMath::SmallEpsilon<float>())).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetDistanceTo (point)")
  {
    xiiSimdBSphere s(xiiSimdVec4f(5, 0, 0), 2);

    XII_TEST_BOOL(s.GetDistanceTo(xiiSimdVec4f(5, 0, 0)) == -2.0f);
    XII_TEST_BOOL(s.GetDistanceTo(xiiSimdVec4f(7, 0, 0)) == 0.0f);
    XII_TEST_BOOL(s.GetDistanceTo(xiiSimdVec4f(9, 0, 0)) == 2.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetDistanceTo (sphere)")
  {
    xiiSimdBSphere s1(xiiSimdVec4f(5, 0, 0), 2);
    xiiSimdBSphere s2(xiiSimdVec4f(10, 0, 0), 3);
    xiiSimdBSphere s3(xiiSimdVec4f(10, 0, 0), 1);

    XII_TEST_BOOL(s1.GetDistanceTo(s2) == 0.0f);
    XII_TEST_BOOL(s1.GetDistanceTo(s3) == 2.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Contains (point)")
  {
    xiiSimdBSphere s(xiiSimdVec4f(5, 0, 0), 2.0f);

    XII_TEST_BOOL(s.Contains(xiiSimdVec4f(3, 0, 0)));
    XII_TEST_BOOL(s.Contains(xiiSimdVec4f(5, 0, 0)));
    XII_TEST_BOOL(s.Contains(xiiSimdVec4f(6, 0, 0)));
    XII_TEST_BOOL(s.Contains(xiiSimdVec4f(7, 0, 0)));

    XII_TEST_BOOL(!s.Contains(xiiSimdVec4f(2, 0, 0)));
    XII_TEST_BOOL(!s.Contains(xiiSimdVec4f(8, 0, 0)));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Contains (sphere)")
  {
    xiiSimdBSphere s1(xiiSimdVec4f(5, 0, 0), 2);
    xiiSimdBSphere s2(xiiSimdVec4f(6, 0, 0), 1);
    xiiSimdBSphere s3(xiiSimdVec4f(6, 0, 0), 2);

    XII_TEST_BOOL(s1.Contains(s1));
    XII_TEST_BOOL(s2.Contains(s2));
    XII_TEST_BOOL(s3.Contains(s3));

    XII_TEST_BOOL(s1.Contains(s2));
    XII_TEST_BOOL(!s1.Contains(s3));

    XII_TEST_BOOL(!s2.Contains(s3));
    XII_TEST_BOOL(s3.Contains(s2));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Overlaps (sphere)")
  {
    xiiSimdBSphere s1(xiiSimdVec4f(5, 0, 0), 2);
    xiiSimdBSphere s2(xiiSimdVec4f(6, 0, 0), 2);
    xiiSimdBSphere s3(xiiSimdVec4f(8, 0, 0), 1);

    XII_TEST_BOOL(s1.Overlaps(s1));
    XII_TEST_BOOL(s2.Overlaps(s2));
    XII_TEST_BOOL(s3.Overlaps(s3));

    XII_TEST_BOOL(s1.Overlaps(s2));
    XII_TEST_BOOL(!s1.Overlaps(s3));

    XII_TEST_BOOL(s2.Overlaps(s3));
    XII_TEST_BOOL(s3.Overlaps(s2));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetClampedPoint")
  {
    xiiSimdBSphere s(xiiSimdVec4f(1, 2, 3), 2.0f);

    XII_TEST_BOOL(s.GetClampedPoint(xiiSimdVec4f(2, 2, 3)).IsEqual(xiiSimdVec4f(2, 2, 3), 0.001f).AllSet<3>());
    XII_TEST_BOOL(s.GetClampedPoint(xiiSimdVec4f(5, 2, 3)).IsEqual(xiiSimdVec4f(3, 2, 3), 0.001f).AllSet<3>());
    XII_TEST_BOOL(s.GetClampedPoint(xiiSimdVec4f(1, 7, 3)).IsEqual(xiiSimdVec4f(1, 4, 3), 0.001f).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Comparison")
  {
    xiiSimdBSphere s1(xiiSimdVec4f(5, 0, 0), 2);
    xiiSimdBSphere s2(xiiSimdVec4f(6, 0, 0), 1);

    XII_TEST_BOOL(s1 == xiiSimdBSphere(xiiSimdVec4f(5, 0, 0), 2));
    XII_TEST_BOOL(s1 != s2);
  }
}
