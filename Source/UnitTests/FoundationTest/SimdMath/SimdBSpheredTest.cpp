/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdBSphered.h>

XII_CREATE_SIMPLE_TEST(SimdMath, SimdBSphered)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeFromCenterAndRadius")
  {
    xiiSimdBSphered s = xiiSimdBSphered::MakeFromCenterAndRadius(xiiSimdVec4d(1, 2, 3), 4);

    XII_TEST_BOOL((s.m_CenterAndRadius == xiiSimdVec4d(1, 2, 3, 4)).AllSet());

    XII_TEST_BOOL((s.GetCenter() == xiiSimdVec4d(1, 2, 3)).AllSet<3>());
    XII_TEST_BOOL(s.GetRadius() == 4.0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeInvalid / IsValid")
  {
    xiiSimdBSphered s(xiiSimdVec4d(1, 2, 3), 4);

    XII_TEST_BOOL(s.IsValid());

    s = xiiSimdBSphered::MakeInvalid();

    XII_TEST_BOOL(!s.IsValid());
    XII_TEST_BOOL(!s.IsNaN());

    s = xiiSimdBSphered(xiiSimdVec4d(1, 2, 3), xiiMath::NaN<double>());
    XII_TEST_BOOL(s.IsNaN());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExpandToInclude(Point)")
  {
    xiiSimdBSphered s(xiiSimdVec4d::MakeZero(), 0.0);

    s.ExpandToInclude(xiiSimdVec4d(3, 0, 0));

    XII_TEST_BOOL((s.m_CenterAndRadius == xiiSimdVec4d(0, 0, 0, 3)).AllSet());

    s = xiiSimdBSphered::MakeInvalid();

    s.ExpandToInclude(xiiSimdVec4d(0.25, 0, 0));

    XII_TEST_BOOL((s.m_CenterAndRadius == xiiSimdVec4d(0, 0, 0, 0.25)).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExpandToInclude(array)")
  {
    xiiSimdBSphered s(xiiSimdVec4d(2, 2, 0), 0.0);

    xiiSimdVec4d p[4] = {xiiSimdVec4d(0, 2, 0), xiiSimdVec4d(4, 2, 0), xiiSimdVec4d(2, 0, 0), xiiSimdVec4d(2, 4, 0)};

    s.ExpandToInclude(p, 4);

    XII_TEST_BOOL((s.m_CenterAndRadius == xiiSimdVec4d(2, 2, 0, 2)).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExpandToInclude (sphere)")
  {
    xiiSimdBSphered s1(xiiSimdVec4d(5, 0, 0), 1);
    xiiSimdBSphered s2(xiiSimdVec4d(6, 0, 0), 1);
    xiiSimdBSphered s3(xiiSimdVec4d(5, 0, 0), 2);

    s1.ExpandToInclude(s2);
    XII_TEST_BOOL((s1.m_CenterAndRadius == xiiSimdVec4d(5, 0, 0, 2)).AllSet());

    s1.ExpandToInclude(s3);
    XII_TEST_BOOL((s1.m_CenterAndRadius == xiiSimdVec4d(5, 0, 0, 2)).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Transform")
  {
    xiiSimdBSphered s(xiiSimdVec4d(5, 0, 0), 2);

    xiiSimdTransformd t(xiiSimdVec4d(4, 5, 6));
    t.m_Rotation = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 0, 1), xiiAngled::MakeFromDegree(90));
    t.m_Scale    = xiiSimdVec4d(1, -2, -4);

    s.Transform(t);
    XII_TEST_BOOL(s.m_CenterAndRadius.IsEqual(xiiSimdVec4d(4, 10, 6, 8), xiiSimdDouble(xiiMath::SmallEpsilon<double>())).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetDistanceTo (point)")
  {
    xiiSimdBSphered s(xiiSimdVec4d(5, 0, 0), 2);

    XII_TEST_BOOL(s.GetDistanceTo(xiiSimdVec4d(5, 0, 0)) == -2.0);
    XII_TEST_BOOL(s.GetDistanceTo(xiiSimdVec4d(7, 0, 0)) == 0.0);
    XII_TEST_BOOL(s.GetDistanceTo(xiiSimdVec4d(9, 0, 0)) == 2.0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetDistanceTo (sphere)")
  {
    xiiSimdBSphered s1(xiiSimdVec4d(5, 0, 0), 2);
    xiiSimdBSphered s2(xiiSimdVec4d(10, 0, 0), 3);
    xiiSimdBSphered s3(xiiSimdVec4d(10, 0, 0), 1);

    XII_TEST_BOOL(s1.GetDistanceTo(s2) == 0.0);
    XII_TEST_BOOL(s1.GetDistanceTo(s3) == 2.0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Contains (point)")
  {
    xiiSimdBSphered s(xiiSimdVec4d(5, 0, 0), 2.0);

    XII_TEST_BOOL(s.Contains(xiiSimdVec4d(3, 0, 0)));
    XII_TEST_BOOL(s.Contains(xiiSimdVec4d(5, 0, 0)));
    XII_TEST_BOOL(s.Contains(xiiSimdVec4d(6, 0, 0)));
    XII_TEST_BOOL(s.Contains(xiiSimdVec4d(7, 0, 0)));

    XII_TEST_BOOL(!s.Contains(xiiSimdVec4d(2, 0, 0)));
    XII_TEST_BOOL(!s.Contains(xiiSimdVec4d(8, 0, 0)));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Contains (sphere)")
  {
    xiiSimdBSphered s1(xiiSimdVec4d(5, 0, 0), 2);
    xiiSimdBSphered s2(xiiSimdVec4d(6, 0, 0), 1);
    xiiSimdBSphered s3(xiiSimdVec4d(6, 0, 0), 2);

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
    xiiSimdBSphered s1(xiiSimdVec4d(5, 0, 0), 2);
    xiiSimdBSphered s2(xiiSimdVec4d(6, 0, 0), 2);
    xiiSimdBSphered s3(xiiSimdVec4d(8, 0, 0), 1);

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
    xiiSimdBSphered s(xiiSimdVec4d(1, 2, 3), 2.0);

    XII_TEST_BOOL(s.GetClampedPoint(xiiSimdVec4d(2, 2, 3)).IsEqual(xiiSimdVec4d(2, 2, 3), 0.001).AllSet<3>());
    XII_TEST_BOOL(s.GetClampedPoint(xiiSimdVec4d(5, 2, 3)).IsEqual(xiiSimdVec4d(3, 2, 3), 0.001).AllSet<3>());
    XII_TEST_BOOL(s.GetClampedPoint(xiiSimdVec4d(1, 7, 3)).IsEqual(xiiSimdVec4d(1, 4, 3), 0.001).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Comparison")
  {
    xiiSimdBSphered s1(xiiSimdVec4d(5, 0, 0), 2);
    xiiSimdBSphered s2(xiiSimdVec4d(6, 0, 0), 1);

    XII_TEST_BOOL(s1 == xiiSimdBSphered(xiiSimdVec4d(5, 0, 0), 2));
    XII_TEST_BOOL(s1 != s2);
  }
}
