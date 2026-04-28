/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/BoundingBox.h>

XII_CREATE_SIMPLE_TEST(Math, BoundingBox)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeFromMinMax")
  {
    xiiBoundingBoxT b = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(-1, -2, -3), xiiVec3T(1, 2, 3));

    XII_TEST_BOOL(b.m_vMin == xiiVec3T(-1, -2, -3));
    XII_TEST_BOOL(b.m_vMax == xiiVec3T(1, 2, 3));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeFromMinMax")
  {
    xiiBoundingBoxT b = xiiBoundingBox::MakeFromMinMax(xiiVec3T(-1, -2, -3), xiiVec3T(1, 2, 3));

    XII_TEST_BOOL(b.m_vMin == xiiVec3T(-1, -2, -3));
    XII_TEST_BOOL(b.m_vMax == xiiVec3T(1, 2, 3));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeFromPoints")
  {
    xiiVec3T p[6] = {
      xiiVec3T(-4, 0, 0),
      xiiVec3T(5, 0, 0),
      xiiVec3T(0, -6, 0),
      xiiVec3T(0, 7, 0),
      xiiVec3T(0, 0, -8),
      xiiVec3T(0, 0, 9),
    };

    xiiBoundingBoxT b = xiiBoundingBox::MakeFromPoints(p, 6);

    XII_TEST_BOOL(b.m_vMin == xiiVec3T(-4, -6, -8));
    XII_TEST_BOOL(b.m_vMax == xiiVec3T(5, 7, 9));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeInvalid")
  {
    xiiBoundingBoxT b;
    b = xiiBoundingBox::MakeInvalid();

    XII_TEST_BOOL(!b.IsValid());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeFromCenterAndHalfExtents")
  {
    xiiBoundingBoxT b = xiiBoundingBox::MakeFromCenterAndHalfExtents(xiiVec3T(1, 2, 3), xiiVec3T(4, 5, 6));

    XII_TEST_BOOL(b.m_vMin == xiiVec3T(-3, -3, -3));
    XII_TEST_BOOL(b.m_vMax == xiiVec3T(5, 7, 9));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetCorners")
  {
    xiiBoundingBoxT b = xiiBoundingBox::MakeFromMinMax(xiiVec3T(-1, -2, -3), xiiVec3T(1, 2, 3));

    xiiVec3T c[8];
    b.GetCorners(c);

    XII_TEST_BOOL(c[0] == xiiVec3T(-1, -2, -3));
    XII_TEST_BOOL(c[1] == xiiVec3T(-1, -2, 3));
    XII_TEST_BOOL(c[2] == xiiVec3T(-1, 2, -3));
    XII_TEST_BOOL(c[3] == xiiVec3T(-1, 2, 3));
    XII_TEST_BOOL(c[4] == xiiVec3T(1, -2, -3));
    XII_TEST_BOOL(c[5] == xiiVec3T(1, -2, 3));
    XII_TEST_BOOL(c[6] == xiiVec3T(1, 2, -3));
    XII_TEST_BOOL(c[7] == xiiVec3T(1, 2, 3));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExpandToInclue (Point)")
  {
    xiiBoundingBoxT b;
    b = xiiBoundingBox::MakeInvalid();
    b.ExpandToInclude(xiiVec3T(1, 2, 3));

    XII_TEST_BOOL(b.m_vMin == xiiVec3T(1, 2, 3));
    XII_TEST_BOOL(b.m_vMax == xiiVec3T(1, 2, 3));


    b.ExpandToInclude(xiiVec3T(2, 3, 4));

    XII_TEST_BOOL(b.m_vMin == xiiVec3T(1, 2, 3));
    XII_TEST_BOOL(b.m_vMax == xiiVec3T(2, 3, 4));

    b.ExpandToInclude(xiiVec3T(0, 1, 2));

    XII_TEST_BOOL(b.m_vMin == xiiVec3T(0, 1, 2));
    XII_TEST_BOOL(b.m_vMax == xiiVec3T(2, 3, 4));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExpandToInclude (Box)")
  {
    xiiBoundingBoxT b1, b2;

    b1 = xiiBoundingBox::MakeFromMinMax(xiiVec3T(-1, -2, -3), xiiVec3T(1, 2, 3));
    b2 = xiiBoundingBox::MakeFromMinMax(xiiVec3T(0), xiiVec3T(4, 5, 6));

    b1.ExpandToInclude(b2);

    XII_TEST_BOOL(b1.m_vMin == xiiVec3T(-1, -2, -3));
    XII_TEST_BOOL(b1.m_vMax == xiiVec3T(4, 5, 6));

    b2 = xiiBoundingBox::MakeFromMinMax(xiiVec3T(-4, -5, -6), xiiVec3T(0));

    b1.ExpandToInclude(b2);

    XII_TEST_BOOL(b1.m_vMin == xiiVec3T(-4, -5, -6));
    XII_TEST_BOOL(b1.m_vMax == xiiVec3T(4, 5, 6));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExpandToInclude (array)")
  {
    xiiVec3T v[4] = {xiiVec3T(1, 1, 1), xiiVec3T(-1, -1, -1), xiiVec3T(2, 2, 2), xiiVec3T(4, 4, 4)};

    xiiBoundingBoxT b;
    b = xiiBoundingBox::MakeInvalid();
    b.ExpandToInclude(v, 2, sizeof(xiiVec3T) * 2);

    XII_TEST_BOOL(b.m_vMin == xiiVec3T(1, 1, 1));
    XII_TEST_BOOL(b.m_vMax == xiiVec3T(2, 2, 2));

    b.ExpandToInclude(v, 4, sizeof(xiiVec3T));

    XII_TEST_BOOL(b.m_vMin == xiiVec3T(-1, -1, -1));
    XII_TEST_BOOL(b.m_vMax == xiiVec3T(4, 4, 4));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExpandToCube")
  {
    xiiBoundingBoxT b = xiiBoundingBox::MakeFromCenterAndHalfExtents(xiiVec3T(1, 2, 3), xiiVec3T(4, 5, 6));

    b.ExpandToCube();

    XII_TEST_VEC3(b.GetCenter(), xiiVec3T(1, 2, 3), xiiMath::DefaultEpsilon<xiiMathTestType>());
    XII_TEST_VEC3(b.GetHalfExtents(), xiiVec3T(6, 6, 6), xiiMath::DefaultEpsilon<xiiMathTestType>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Grow")
  {
    xiiBoundingBoxT b = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(1, 2, 3), xiiVec3T(4, 5, 6));
    b.Grow(xiiVec3T(2, 4, 6));

    XII_TEST_BOOL(b.m_vMin == xiiVec3T(-1, -2, -3));
    XII_TEST_BOOL(b.m_vMax == xiiVec3T(6, 9, 12));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Contains (Point)")
  {
    xiiBoundingBoxT b = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(0), xiiVec3T(0));

    XII_TEST_BOOL(b.Contains(xiiVec3T(0)));
    XII_TEST_BOOL(!b.Contains(xiiVec3T(1, 0, 0)));
    XII_TEST_BOOL(!b.Contains(xiiVec3T(-1, 0, 0)));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Contains (Box)")
  {
    xiiBoundingBoxT b1 = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(-3), xiiVec3T(3));
    xiiBoundingBoxT b2 = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(-1), xiiVec3T(1));
    xiiBoundingBoxT b3 = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(-1), xiiVec3T(4));

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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Contains (Array)")
  {
    xiiBoundingBoxT b = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(1), xiiVec3T(5));

    xiiVec3T v[4] = {xiiVec3T(0), xiiVec3T(1), xiiVec3T(5), xiiVec3T(6)};

    XII_TEST_BOOL(!b.Contains(&v[0], 4, sizeof(xiiVec3T)));
    XII_TEST_BOOL(b.Contains(&v[1], 2, sizeof(xiiVec3T)));
    XII_TEST_BOOL(b.Contains(&v[2], 1, sizeof(xiiVec3T)));

    XII_TEST_BOOL(!b.Contains(&v[1], 2, sizeof(xiiVec3T) * 2));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Contains (Sphere)")
  {
    xiiBoundingBoxT b = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(1), xiiVec3T(5));

    XII_TEST_BOOL(b.Contains(xiiBoundingSphereT::MakeFromCenterAndRadius(xiiVec3T(3), 2)));
    XII_TEST_BOOL(!b.Contains(xiiBoundingSphereT::MakeFromCenterAndRadius(xiiVec3T(3), 2.1f)));
    XII_TEST_BOOL(!b.Contains(xiiBoundingSphereT::MakeFromCenterAndRadius(xiiVec3T(8), 2)));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Overlaps (box)")
  {
    xiiBoundingBoxT b1 = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(-3), xiiVec3T(3));
    xiiBoundingBoxT b2 = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(-1), xiiVec3T(1));
    xiiBoundingBoxT b3 = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(1), xiiVec3T(4));
    xiiBoundingBoxT b4 = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(-4, 1, 1), xiiVec3T(4, 2, 2));

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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Overlaps (Array)")
  {
    xiiBoundingBoxT b = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(1), xiiVec3T(5));

    xiiVec3T v[4] = {xiiVec3T(0), xiiVec3T(1), xiiVec3T(5), xiiVec3T(6)};

    XII_TEST_BOOL(!b.Overlaps(&v[0], 1, sizeof(xiiVec3T)));
    XII_TEST_BOOL(!b.Overlaps(&v[3], 1, sizeof(xiiVec3T)));

    XII_TEST_BOOL(b.Overlaps(&v[0], 4, sizeof(xiiVec3T)));
    XII_TEST_BOOL(b.Overlaps(&v[1], 2, sizeof(xiiVec3T)));
    XII_TEST_BOOL(b.Overlaps(&v[2], 1, sizeof(xiiVec3T)));

    XII_TEST_BOOL(b.Overlaps(&v[1], 2, sizeof(xiiVec3T) * 2));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Overlaps (Sphere)")
  {
    xiiBoundingBoxT b = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(1), xiiVec3T(5));

    XII_TEST_BOOL(b.Overlaps(xiiBoundingSphereT::MakeFromCenterAndRadius(xiiVec3T(3), 2)));
    XII_TEST_BOOL(b.Overlaps(xiiBoundingSphereT::MakeFromCenterAndRadius(xiiVec3T(3), 2.1f)));
    XII_TEST_BOOL(!b.Overlaps(xiiBoundingSphereT::MakeFromCenterAndRadius(xiiVec3T(8), 2)));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsIdentical, ==, !=")
  {
    xiiBoundingBoxT b1, b2, b3;

    b1 = xiiBoundingBox::MakeFromMinMax(xiiVec3T(1), xiiVec3T(2));
    b2 = xiiBoundingBox::MakeFromMinMax(xiiVec3T(1), xiiVec3T(2));
    b3 = xiiBoundingBox::MakeFromMinMax(xiiVec3T(1), xiiVec3T(2.01f));

    XII_TEST_BOOL(b1.IsIdentical(b1));
    XII_TEST_BOOL(b2.IsIdentical(b2));
    XII_TEST_BOOL(b3.IsIdentical(b3));

    XII_TEST_BOOL(b1 == b1);
    XII_TEST_BOOL(b2 == b2);
    XII_TEST_BOOL(b3 == b3);

    XII_TEST_BOOL(b1.IsIdentical(b2));
    XII_TEST_BOOL(b2.IsIdentical(b1));

    XII_TEST_BOOL(!b1.IsIdentical(b3));
    XII_TEST_BOOL(!b2.IsIdentical(b3));
    XII_TEST_BOOL(!b3.IsIdentical(b1));
    XII_TEST_BOOL(!b3.IsIdentical(b1));

    XII_TEST_BOOL(b1 == b2);
    XII_TEST_BOOL(b2 == b1);

    XII_TEST_BOOL(b1 != b3);
    XII_TEST_BOOL(b2 != b3);
    XII_TEST_BOOL(b3 != b1);
    XII_TEST_BOOL(b3 != b1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsEqual")
  {
    xiiBoundingBoxT b1, b2;
    b1 = xiiBoundingBox::MakeFromMinMax(xiiVec3T(-1), xiiVec3T(1));
    b2 = xiiBoundingBox::MakeFromMinMax(xiiVec3T(-1), xiiVec3T(2));

    XII_TEST_BOOL(!b1.IsEqual(b2));
    XII_TEST_BOOL(!b1.IsEqual(b2, 0.5f));
    XII_TEST_BOOL(b1.IsEqual(b2, 1));
    XII_TEST_BOOL(b1.IsEqual(b2, 2));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetCenter")
  {
    xiiBoundingBoxT b = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(3), xiiVec3T(7));

    XII_TEST_BOOL(b.GetCenter() == xiiVec3T(5));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetExtents")
  {
    xiiBoundingBoxT b = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(3), xiiVec3T(7));

    XII_TEST_BOOL(b.GetExtents() == xiiVec3T(4));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetHalfExtents")
  {
    xiiBoundingBoxT b = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(3), xiiVec3T(7));

    XII_TEST_BOOL(b.GetHalfExtents() == xiiVec3T(2));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Translate")
  {
    xiiBoundingBoxT b = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(3), xiiVec3T(5));

    b.Translate(xiiVec3T(1, 2, 3));

    XII_TEST_BOOL(b.m_vMin == xiiVec3T(4, 5, 6));
    XII_TEST_BOOL(b.m_vMax == xiiVec3T(6, 7, 8));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ScaleFromCenter")
  {
    {
      xiiBoundingBoxT b = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(3), xiiVec3T(5));

      b.ScaleFromCenter(xiiVec3T(1, 2, 3));

      XII_TEST_BOOL(b.m_vMin == xiiVec3T(3, 2, 1));
      XII_TEST_BOOL(b.m_vMax == xiiVec3T(5, 6, 7));
    }
    {
      xiiBoundingBoxT b = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(3), xiiVec3T(5));

      b.ScaleFromCenter(xiiVec3T(-1, -2, -3));

      XII_TEST_BOOL(b.m_vMin == xiiVec3T(3, 2, 1));
      XII_TEST_BOOL(b.m_vMax == xiiVec3T(5, 6, 7));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ScaleFromOrigin")
  {
    {
      xiiBoundingBoxT b = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(3), xiiVec3T(5));

      b.ScaleFromOrigin(xiiVec3T(1, 2, 3));

      XII_TEST_BOOL(b.m_vMin == xiiVec3T(3, 6, 9));
      XII_TEST_BOOL(b.m_vMax == xiiVec3T(5, 10, 15));
    }
    {
      xiiBoundingBoxT b = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(3), xiiVec3T(5));

      b.ScaleFromOrigin(xiiVec3T(-1, -2, -3));

      XII_TEST_BOOL(b.m_vMin == xiiVec3T(-5, -10, -15));
      XII_TEST_BOOL(b.m_vMax == xiiVec3T(-3, -6, -9));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TransformFromOrigin")
  {
    xiiBoundingBoxT b = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(3), xiiVec3T(5));

    xiiMat4T m = xiiMat4::MakeScaling(xiiVec3T(2));

    b.TransformFromOrigin(m);

    XII_TEST_BOOL(b.m_vMin == xiiVec3T(6, 6, 6));
    XII_TEST_BOOL(b.m_vMax == xiiVec3T(10, 10, 10));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TransformFromCenter")
  {
    xiiBoundingBoxT b = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(3), xiiVec3T(5));

    xiiMat4T m = xiiMat4::MakeScaling(xiiVec3T(2));

    b.TransformFromCenter(m);

    XII_TEST_BOOL(b.m_vMin == xiiVec3T(2, 2, 2));
    XII_TEST_BOOL(b.m_vMax == xiiVec3T(6, 6, 6));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetClampedPoint")
  {
    xiiBoundingBoxT b = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(-1, -2, -3), xiiVec3T(1, 2, 3));

    XII_TEST_BOOL(b.GetClampedPoint(xiiVec3T(-2, 0, 0)) == xiiVec3T(-1, 0, 0));
    XII_TEST_BOOL(b.GetClampedPoint(xiiVec3T(2, 0, 0)) == xiiVec3T(1, 0, 0));

    XII_TEST_BOOL(b.GetClampedPoint(xiiVec3T(0, -3, 0)) == xiiVec3T(0, -2, 0));
    XII_TEST_BOOL(b.GetClampedPoint(xiiVec3T(0, 3, 0)) == xiiVec3T(0, 2, 0));

    XII_TEST_BOOL(b.GetClampedPoint(xiiVec3T(0, 0, -4)) == xiiVec3T(0, 0, -3));
    XII_TEST_BOOL(b.GetClampedPoint(xiiVec3T(0, 0, 4)) == xiiVec3T(0, 0, 3));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetDistanceTo (point)")
  {
    xiiBoundingBoxT b = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(-1, -2, -3), xiiVec3T(1, 2, 3));

    XII_TEST_BOOL(b.GetDistanceTo(xiiVec3T(-2, 0, 0)) == 1);
    XII_TEST_BOOL(b.GetDistanceTo(xiiVec3T(2, 0, 0)) == 1);

    XII_TEST_BOOL(b.GetDistanceTo(xiiVec3T(0, -4, 0)) == 2);
    XII_TEST_BOOL(b.GetDistanceTo(xiiVec3T(0, 4, 0)) == 2);

    XII_TEST_BOOL(b.GetDistanceTo(xiiVec3T(0, 0, -6)) == 3);
    XII_TEST_BOOL(b.GetDistanceTo(xiiVec3T(0, 0, 6)) == 3);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetDistanceTo (Sphere)")
  {
    xiiBoundingBoxT b = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(1), xiiVec3T(5));

    XII_TEST_BOOL(b.GetDistanceTo(xiiBoundingSphereT::MakeFromCenterAndRadius(xiiVec3T(3), 2)) < 0);
    XII_TEST_BOOL(b.GetDistanceTo(xiiBoundingSphereT::MakeFromCenterAndRadius(xiiVec3T(5), 1)) < 0);
    XII_TEST_FLOAT(b.GetDistanceTo(xiiBoundingSphereT::MakeFromCenterAndRadius(xiiVec3T(8, 2, 2), 2)), 1, 0.001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetDistanceTo (box)")
  {
    xiiBoundingBoxT b = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(1), xiiVec3T(5));

    xiiBoundingBoxT b1, b2, b3;
    b1 = xiiBoundingBox::MakeFromCenterAndHalfExtents(xiiVec3T(3), xiiVec3T(2));
    b2 = xiiBoundingBox::MakeFromCenterAndHalfExtents(xiiVec3T(5), xiiVec3T(1));
    b3 = xiiBoundingBox::MakeFromCenterAndHalfExtents(xiiVec3T(9, 2, 2), xiiVec3T(2));

    XII_TEST_BOOL(b.GetDistanceTo(b1) <= 0);
    XII_TEST_BOOL(b.GetDistanceTo(b2) <= 0);
    XII_TEST_FLOAT(b.GetDistanceTo(b3), 2, 0.001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetDistanceSquaredTo (point)")
  {
    xiiBoundingBoxT b = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(-1, -2, -3), xiiVec3T(1, 2, 3));

    XII_TEST_BOOL(b.GetDistanceSquaredTo(xiiVec3T(-2, 0, 0)) == 1);
    XII_TEST_BOOL(b.GetDistanceSquaredTo(xiiVec3T(2, 0, 0)) == 1);

    XII_TEST_BOOL(b.GetDistanceSquaredTo(xiiVec3T(0, -4, 0)) == 4);
    XII_TEST_BOOL(b.GetDistanceSquaredTo(xiiVec3T(0, 4, 0)) == 4);

    XII_TEST_BOOL(b.GetDistanceSquaredTo(xiiVec3T(0, 0, -6)) == 9);
    XII_TEST_BOOL(b.GetDistanceSquaredTo(xiiVec3T(0, 0, 6)) == 9);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetDistanceSquaredTo (box)")
  {
    xiiBoundingBoxT b = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(1), xiiVec3T(5));

    xiiBoundingBoxT b1, b2, b3;
    b1 = xiiBoundingBox::MakeFromCenterAndHalfExtents(xiiVec3T(3), xiiVec3T(2));
    b2 = xiiBoundingBox::MakeFromCenterAndHalfExtents(xiiVec3T(5), xiiVec3T(1));
    b3 = xiiBoundingBox::MakeFromCenterAndHalfExtents(xiiVec3T(9, 2, 2), xiiVec3T(2));

    XII_TEST_BOOL(b.GetDistanceSquaredTo(b1) <= 0);
    XII_TEST_BOOL(b.GetDistanceSquaredTo(b2) <= 0);
    XII_TEST_FLOAT(b.GetDistanceSquaredTo(b3), 4, 0.001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetBoundingSphere")
  {
    xiiBoundingBoxT b;
    b = xiiBoundingBox::MakeFromCenterAndHalfExtents(xiiVec3T(5, 4, 2), xiiVec3T(3));

    xiiBoundingSphereT s = b.GetBoundingSphere();

    XII_TEST_BOOL(s.m_vCenter == xiiVec3T(5, 4, 2));
    XII_TEST_FLOAT(s.m_fRadius, xiiVec3T(3).GetLength(), 0.001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetRayIntersection")
  {
    if (xiiMath::SupportsInfinity<xiiMathTestType>())
    {
      const xiiVec3T c = xiiVec3T(10);

      xiiBoundingBoxT b;
      b = xiiBoundingBox::MakeFromCenterAndHalfExtents(c, xiiVec3T(2, 4, 8));

      for (xiiMathTestType x = b.m_vMin.x - (xiiMathTestType)1; x < b.m_vMax.x + (xiiMathTestType)1; x += (xiiMathTestType)0.2f)
      {
        for (xiiMathTestType y = b.m_vMin.y - (xiiMathTestType)1; y < b.m_vMax.y + (xiiMathTestType)1; y += (xiiMathTestType)0.2f)
        {
          for (xiiMathTestType z = b.m_vMin.z - (xiiMathTestType)1; z < b.m_vMax.z + (xiiMathTestType)1; z += (xiiMathTestType)0.2f)
          {
            const xiiVec3T v(x, y, z);

            if (b.Contains(v))
              continue;

            const xiiVec3T vTarget = b.GetClampedPoint(v);

            const xiiVec3T vDir = (vTarget - c).GetNormalized();

            const xiiVec3T vSource = vTarget + vDir * (xiiMathTestType)3;

            xiiMathTestType f;
            xiiVec3T        vi;
            XII_TEST_BOOL(b.GetRayIntersection(vSource, -vDir, &f, &vi) == true);
            XII_TEST_FLOAT(f, 3, 0.001f);
            XII_TEST_BOOL(vi.IsEqual(vTarget, 0.0001f));

            XII_TEST_BOOL(b.GetRayIntersection(vSource, vDir, &f, &vi) == false);
            XII_TEST_BOOL(b.GetRayIntersection(vTarget, vDir, &f, &vi) == false);
          }
        }
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetLineSegmentIntersection")
  {
    if (xiiMath::SupportsInfinity<xiiMathTestType>())
    {
      const xiiVec3T c = xiiVec3T(10);

      xiiBoundingBoxT b;
      b = xiiBoundingBox::MakeFromCenterAndHalfExtents(c, xiiVec3T(2, 4, 8));

      for (xiiMathTestType x = b.m_vMin.x - (xiiMathTestType)1; x < b.m_vMax.x + (xiiMathTestType)1; x += (xiiMathTestType)0.2f)
      {
        for (xiiMathTestType y = b.m_vMin.y - (xiiMathTestType)1; y < b.m_vMax.y + (xiiMathTestType)1; y += (xiiMathTestType)0.2f)
        {
          for (xiiMathTestType z = b.m_vMin.z - (xiiMathTestType)1; z < b.m_vMax.z + (xiiMathTestType)1; z += (xiiMathTestType)0.2f)
          {
            const xiiVec3T v(x, y, z);

            if (b.Contains(v))
              continue;

            const xiiVec3T vTarget0 = b.GetClampedPoint(v);

            const xiiVec3T vDir = (vTarget0 - c).GetNormalized();

            const xiiVec3T vTarget = vTarget0 - vDir * (xiiMathTestType)1;
            const xiiVec3T vSource = vTarget0 + vDir * (xiiMathTestType)3;

            xiiMathTestType f;
            xiiVec3T        vi;
            XII_TEST_BOOL(b.GetLineSegmentIntersection(vSource, vTarget, &f, &vi) == true);
            XII_TEST_FLOAT(f, 0.75f, 0.001f);
            XII_TEST_BOOL(vi.IsEqual(vTarget0, 0.0001f));
          }
        }
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsNaN")
  {
    if (xiiMath::SupportsNaN<xiiMathTestType>())
    {
      xiiBoundingBoxT b;

      b = xiiBoundingBox::MakeInvalid();
      XII_TEST_BOOL(!b.IsNaN());

      b          = xiiBoundingBox::MakeInvalid();
      b.m_vMin.x = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(b.IsNaN());

      b          = xiiBoundingBox::MakeInvalid();
      b.m_vMin.y = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(b.IsNaN());

      b          = xiiBoundingBox::MakeInvalid();
      b.m_vMin.z = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(b.IsNaN());

      b          = xiiBoundingBox::MakeInvalid();
      b.m_vMax.x = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(b.IsNaN());

      b          = xiiBoundingBox::MakeInvalid();
      b.m_vMax.y = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(b.IsNaN());

      b          = xiiBoundingBox::MakeInvalid();
      b.m_vMax.z = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(b.IsNaN());
    }
  }
}
