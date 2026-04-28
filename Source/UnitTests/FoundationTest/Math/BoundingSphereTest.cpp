/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/BoundingSphere.h>

XII_CREATE_SIMPLE_TEST(Math, BoundingSphere)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
    xiiBoundingSphereT s = xiiBoundingSphereT::MakeFromCenterAndRadius(xiiVec3T(1, 2, 3), 4);

    XII_TEST_BOOL(s.m_vCenter == xiiVec3T(1, 2, 3));
    XII_TEST_BOOL(s.m_fRadius == 4.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetInvalid / IsValid")
  {
    xiiBoundingSphereT s = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(1, 2, 3), 4);

    XII_TEST_BOOL(s.IsValid());

    s = xiiBoundingSphereT::MakeInvalid();

    XII_TEST_BOOL(!s.IsValid());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeZero / IsZero")
  {
    xiiBoundingSphereT s = xiiBoundingSphereT::MakeZero();

    XII_TEST_BOOL(s.IsValid());
    XII_TEST_BOOL(s.m_vCenter.IsZero());
    XII_TEST_BOOL(s.m_fRadius == 0.0f);
    XII_TEST_BOOL(s.IsZero());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetElements")
  {
    xiiBoundingSphereT s = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(1, 2, 3), 4);

    XII_TEST_BOOL(s.m_vCenter == xiiVec3T(1, 2, 3));
    XII_TEST_BOOL(s.m_fRadius == 4);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetFromPoints")
  {
    xiiVec3T p[4] = {xiiVec3T(2, 6, 0), xiiVec3T(4, 2, 0), xiiVec3T(2, 0, 0), xiiVec3T(0, 4, 0)};

    xiiBoundingSphereT s = xiiBoundingSphereT::MakeFromPoints(p, 4);

    XII_TEST_BOOL(s.m_vCenter == xiiVec3T(2, 3, 0));
    XII_TEST_BOOL(s.m_fRadius == 3);

    for (int i = 0; i < XII_ARRAY_SIZE(p); ++i)
    {
      XII_TEST_BOOL(s.Contains(p[i]));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExpandToInclude(Point)")
  {
    xiiBoundingSphereT s = xiiBoundingSphereT::MakeZero();

    s.ExpandToInclude(xiiVec3T(3, 0, 0));

    XII_TEST_BOOL(s.m_vCenter == xiiVec3T(0, 0, 0));
    XII_TEST_BOOL(s.m_fRadius == 3);

    s = xiiBoundingSphereT::MakeInvalid();

    s.ExpandToInclude(xiiVec3T(0.25, 0.0, 0.0));

    XII_TEST_BOOL(s.m_vCenter == xiiVec3T(0, 0, 0));
    XII_TEST_BOOL(s.m_fRadius == 0.25);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExpandToInclude(array)")
  {
    xiiBoundingSphereT s = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(2, 2, 0), 0.0f);

    xiiVec3T p[4] = {xiiVec3T(0, 2, 0), xiiVec3T(4, 2, 0), xiiVec3T(2, 0, 0), xiiVec3T(2, 4, 0)};

    s.ExpandToInclude(p, 4);

    XII_TEST_BOOL(s.m_vCenter == xiiVec3T(2, 2, 0));
    XII_TEST_BOOL(s.m_fRadius == 2);

    for (int i = 0; i < XII_ARRAY_SIZE(p); ++i)
    {
      XII_TEST_BOOL(s.Contains(p[i]));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExpandToInclude (sphere)")
  {
    xiiBoundingSphereT s1, s2, s3;
    s1 = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(5, 0, 0), 1);
    s2 = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(6, 0, 0), 1);
    s3 = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(5, 0, 0), 2);

    s1.ExpandToInclude(s2);
    XII_TEST_BOOL(s1.m_vCenter == xiiVec3T(5, 0, 0));
    XII_TEST_BOOL(s1.m_fRadius == 2);

    s1.ExpandToInclude(s3);
    XII_TEST_BOOL(s1.m_vCenter == xiiVec3T(5, 0, 0));
    XII_TEST_BOOL(s1.m_fRadius == 2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExpandToInclude (box)")
  {
    xiiBoundingSphereT s = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(1, 2, 3), 1);

    xiiBoundingBoxT b = xiiBoundingBox::MakeFromCenterAndHalfExtents(xiiVec3T(1, 2, 3), xiiVec3T(2.0f));

    s.ExpandToInclude(b);

    XII_TEST_BOOL(s.m_vCenter == xiiVec3T(1, 2, 3));
    XII_TEST_FLOAT(s.m_fRadius, xiiMath::Sqrt((xiiMathTestType)12), 0.000001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Grow")
  {
    xiiBoundingSphereT s = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(1, 2, 3), 4);

    s.Grow(5);

    XII_TEST_BOOL(s.m_vCenter == xiiVec3T(1, 2, 3));
    XII_TEST_BOOL(s.m_fRadius == 9);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsIdentical, ==, !=")
  {
    xiiBoundingSphereT s1, s2, s3;

    s1 = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(1, 2, 3), 4);
    s2 = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(1, 2, 3), 4);
    s3 = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(1.001f, 2.001f, 3.001f), 4.001f);

    XII_TEST_BOOL(s1 == s1);
    XII_TEST_BOOL(s2 == s2);
    XII_TEST_BOOL(s3 == s3);

    XII_TEST_BOOL(s1 == s2);
    XII_TEST_BOOL(s2 == s1);

    XII_TEST_BOOL(s1 != s3);
    XII_TEST_BOOL(s2 != s3);
    XII_TEST_BOOL(s3 != s1);
    XII_TEST_BOOL(s3 != s2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsEqual")
  {
    xiiBoundingSphereT s1, s2, s3;

    s1 = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(1, 2, 3), 4);
    s2 = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(1, 2, 3), 4);
    s3 = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(1.001f, 2.001f, 3.001f), 4.001f);

    XII_TEST_BOOL(s1.IsEqual(s1));
    XII_TEST_BOOL(s2.IsEqual(s2));
    XII_TEST_BOOL(s3.IsEqual(s3));

    XII_TEST_BOOL(s1.IsEqual(s2));
    XII_TEST_BOOL(s2.IsEqual(s1));

    XII_TEST_BOOL(!s1.IsEqual(s3, 0.0001f));
    XII_TEST_BOOL(!s2.IsEqual(s3, 0.0001f));
    XII_TEST_BOOL(!s3.IsEqual(s1, 0.0001f));
    XII_TEST_BOOL(!s3.IsEqual(s2, 0.0001f));

    XII_TEST_BOOL(s1.IsEqual(s3, 0.002f));
    XII_TEST_BOOL(s2.IsEqual(s3, 0.002f));
    XII_TEST_BOOL(s3.IsEqual(s1, 0.002f));
    XII_TEST_BOOL(s3.IsEqual(s2, 0.002f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Translate")
  {
    xiiBoundingSphereT s = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(1, 2, 3), 4);

    s.Translate(xiiVec3T(4, 5, 6));

    XII_TEST_BOOL(s.m_vCenter == xiiVec3T(5, 7, 9));
    XII_TEST_BOOL(s.m_fRadius == 4);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ScaleFromCenter")
  {
    xiiBoundingSphereT s = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(1, 2, 3), 4);

    s.ScaleFromCenter(5.0f);

    XII_TEST_BOOL(s.m_vCenter == xiiVec3T(1, 2, 3));
    XII_TEST_BOOL(s.m_fRadius == 20);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ScaleFromOrigin")
  {
    xiiBoundingSphereT s = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(1, 2, 3), 4);

    s.ScaleFromOrigin(xiiVec3T(2, 3, 4));

    XII_TEST_BOOL(s.m_vCenter == xiiVec3T(2, 6, 12));
    XII_TEST_BOOL(s.m_fRadius == 16);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetDistanceTo (point)")
  {
    xiiBoundingSphereT s = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(5, 0, 0), 2);

    XII_TEST_BOOL(s.GetDistanceTo(xiiVec3T(5, 0, 0)) == -2.0f);
    XII_TEST_BOOL(s.GetDistanceTo(xiiVec3T(7, 0, 0)) == 0.0f);
    XII_TEST_BOOL(s.GetDistanceTo(xiiVec3T(9, 0, 0)) == 2.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetDistanceTo (sphere)")
  {
    xiiBoundingSphereT s1, s2, s3;
    s1 = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(5, 0, 0), 2);
    s2 = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(10, 0, 0), 3);
    s3 = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(10, 0, 0), 1);

    XII_TEST_BOOL(s1.GetDistanceTo(s2) == 0.0f);
    XII_TEST_BOOL(s1.GetDistanceTo(s3) == 2.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetDistanceTo (array)")
  {
    xiiBoundingSphereT s = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(0.0f), 0.0f);

    xiiVec3T p[4] = {
      xiiVec3T(5),
      xiiVec3T(10),
      xiiVec3T(15),
      xiiVec3T(7),
    };

    XII_TEST_FLOAT(s.GetDistanceTo(p, 4), xiiVec3T(5).GetLength(), 0.001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Contains (point)")
  {
    xiiBoundingSphereT s = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(5, 0, 0), 2.0f);

    XII_TEST_BOOL(s.Contains(xiiVec3T(3, 0, 0)));
    XII_TEST_BOOL(s.Contains(xiiVec3T(5, 0, 0)));
    XII_TEST_BOOL(s.Contains(xiiVec3T(6, 0, 0)));
    XII_TEST_BOOL(s.Contains(xiiVec3T(7, 0, 0)));

    XII_TEST_BOOL(!s.Contains(xiiVec3T(2, 0, 0)));
    XII_TEST_BOOL(!s.Contains(xiiVec3T(8, 0, 0)));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Contains (array)")
  {
    xiiBoundingSphereT s = xiiBoundingSphereT::MakeFromCenterAndRadius(xiiVec3T(0.0f), 6.0f);

    xiiVec3T p[4] = {
      xiiVec3T(3),
      xiiVec3T(10),
      xiiVec3T(2),
      xiiVec3T(7),
    };

    XII_TEST_BOOL(s.Contains(p, 2, sizeof(xiiVec3T) * 2));
    XII_TEST_BOOL(!s.Contains(p + 1, 2, sizeof(xiiVec3T) * 2));
    XII_TEST_BOOL(!s.Contains(p, 4, sizeof(xiiVec3T)));
  }

  // Disabled because MSVC 2017 has code generation issues in Release builds
  XII_TEST_BLOCK(xiiTestBlock::Disabled, "Contains (sphere)")
  {
    xiiBoundingSphereT s1, s2, s3;
    s1 = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(5, 0, 0), 2);
    s2 = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(6, 0, 0), 1);
    s3 = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(6, 0, 0), 2);

    XII_TEST_BOOL(s1.Contains(s1));
    XII_TEST_BOOL(s2.Contains(s2));
    XII_TEST_BOOL(s3.Contains(s3));

    XII_TEST_BOOL(s1.Contains(s2));
    XII_TEST_BOOL(!s1.Contains(s3));

    XII_TEST_BOOL(!s2.Contains(s3));
    XII_TEST_BOOL(s3.Contains(s2));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Contains (box)")
  {
    xiiBoundingSphereT s  = xiiBoundingSphereT::MakeFromCenterAndRadius(xiiVec3T(1, 2, 3), 4);
    xiiBoundingBoxT    b1 = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(1, 2, 3) - xiiVec3T(1), xiiVec3T(1, 2, 3) + xiiVec3T(1));
    xiiBoundingBoxT    b2 = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(1, 2, 3) - xiiVec3T(1), xiiVec3T(1, 2, 3) + xiiVec3T(3));

    XII_TEST_BOOL(s.Contains(b1));
    XII_TEST_BOOL(!s.Contains(b2));

    xiiVec3T vDir(1, 1, 1);
    vDir.SetLength(3.99f).IgnoreResult();
    xiiBoundingBoxT b3 = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(1, 2, 3) - xiiVec3T(1), xiiVec3T(1, 2, 3) + vDir);

    XII_TEST_BOOL(s.Contains(b3));

    vDir.SetLength(4.01f).IgnoreResult();
    xiiBoundingBoxT b4 = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(1, 2, 3) - xiiVec3T(1), xiiVec3T(1, 2, 3) + vDir);

    XII_TEST_BOOL(!s.Contains(b4));
  }


  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Overlaps (array)")
  {
    xiiBoundingSphereT s = xiiBoundingSphereT::MakeFromCenterAndRadius(xiiVec3T(0.0f), 6.0f);

    xiiVec3T p[4] = {
      xiiVec3T(3),
      xiiVec3T(10),
      xiiVec3T(2),
      xiiVec3T(7),
    };

    XII_TEST_BOOL(s.Overlaps(p, 2, sizeof(xiiVec3T) * 2));
    XII_TEST_BOOL(!s.Overlaps(p + 1, 2, sizeof(xiiVec3T) * 2));
    XII_TEST_BOOL(s.Overlaps(p, 4, sizeof(xiiVec3T)));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Overlaps (sphere)")
  {
    xiiBoundingSphereT s1, s2, s3;
    s1 = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(5, 0, 0), 2);
    s2 = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(6, 0, 0), 2);
    s3 = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(8, 0, 0), 1);

    XII_TEST_BOOL(s1.Overlaps(s1));
    XII_TEST_BOOL(s2.Overlaps(s2));
    XII_TEST_BOOL(s3.Overlaps(s3));

    XII_TEST_BOOL(s1.Overlaps(s2));
    XII_TEST_BOOL(!s1.Overlaps(s3));

    XII_TEST_BOOL(s2.Overlaps(s3));
    XII_TEST_BOOL(s3.Overlaps(s2));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Overlaps (box)")
  {
    xiiBoundingSphereT s  = xiiBoundingSphereT::MakeFromCenterAndRadius(xiiVec3T(1, 2, 3), 2);
    xiiBoundingBoxT    b1 = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(1, 2, 3), xiiVec3T(1, 2, 3) + xiiVec3T(2));
    xiiBoundingBoxT    b2 = xiiBoundingBoxT::MakeFromMinMax(xiiVec3T(1, 2, 3) + xiiVec3T(2), xiiVec3T(1, 2, 3) + xiiVec3T(3));

    XII_TEST_BOOL(s.Overlaps(b1));
    XII_TEST_BOOL(!s.Overlaps(b2));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetBoundingBox")
  {
    xiiBoundingSphereT s = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3T(1, 2, 3), 2.0f);

    xiiBoundingBoxT b = s.GetBoundingBox();

    XII_TEST_BOOL(b.m_vMin == xiiVec3T(-1, 0, 1));
    XII_TEST_BOOL(b.m_vMax == xiiVec3T(3, 4, 5));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetClampedPoint")
  {
    xiiBoundingSphereT s = xiiBoundingSphereT::MakeFromCenterAndRadius(xiiVec3T(1, 2, 3), 2.0f);

    XII_TEST_VEC3(s.GetClampedPoint(xiiVec3T(2, 2, 3)), xiiVec3T(2, 2, 3), 0.001);
    XII_TEST_VEC3(s.GetClampedPoint(xiiVec3T(5, 2, 3)), xiiVec3T(3, 2, 3), 0.001);
    XII_TEST_VEC3(s.GetClampedPoint(xiiVec3T(1, 7, 3)), xiiVec3T(1, 4, 3), 0.001);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetRayIntersection")
  {
    xiiBoundingSphereT s = xiiBoundingSphereT::MakeFromCenterAndRadius(xiiVec3T(1, 2, 3), 4);

    for (xiiUInt32 i = 0; i < 10000; ++i)
    {
      const xiiVec3T vDir =
        xiiVec3T(xiiMath::Sin(xiiAngle::MakeFromDegree(i * 1.0f)), xiiMath::Cos(xiiAngle::MakeFromDegree(i * 3.0f)), xiiMath::Cos(xiiAngle::MakeFromDegree(i * 1.0f)))
          .GetNormalized();
      const xiiVec3T vTarget = vDir * s.m_fRadius + s.m_vCenter;
      const xiiVec3T vSource = vTarget + vDir * (xiiMathTestType)5;

      XII_TEST_FLOAT((vSource - vTarget).GetLength(), 5.0f, 0.001f);

      xiiMathTestType fIntersection;
      xiiVec3T        vIntersection;
      XII_TEST_BOOL(s.GetRayIntersection(vSource, -vDir, &fIntersection, &vIntersection) == true);
      XII_TEST_FLOAT(fIntersection, (vSource - vTarget).GetLength(), 0.0001f);
      XII_TEST_BOOL(vIntersection.IsEqual(vTarget, 0.0001f));

      XII_TEST_BOOL(s.GetRayIntersection(vSource, vDir, &fIntersection, &vIntersection) == false);

      XII_TEST_BOOL(s.GetRayIntersection(vTarget - vDir, vDir, &fIntersection, &vIntersection) == true);
      XII_TEST_FLOAT(fIntersection, 1, 0.0001f);
      XII_TEST_BOOL(vIntersection.IsEqual(vTarget, 0.0001f));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetLineSegmentIntersection")
  {
    xiiBoundingSphereT s = xiiBoundingSphereT::MakeFromCenterAndRadius(xiiVec3T(1, 2, 3), 4);

    for (xiiUInt32 i = 0; i < 10000; ++i)
    {
      const xiiVec3T vDir    = xiiVec3T(xiiMath::Sin(xiiAngle::MakeFromDegree(i * (xiiMathTestType)1)), xiiMath::Cos(xiiAngle::MakeFromDegree(i * (xiiMathTestType)3)), xiiMath::Cos(xiiAngle::MakeFromDegree(i * (xiiMathTestType)1))).GetNormalized();
      const xiiVec3T vTarget = vDir * s.m_fRadius + s.m_vCenter - vDir;
      const xiiVec3T vSource = vTarget + vDir * (xiiMathTestType)5;

      xiiMathTestType fIntersection;
      xiiVec3T        vIntersection;
      XII_TEST_BOOL(s.GetLineSegmentIntersection(vSource, vTarget, &fIntersection, &vIntersection) == true);
      XII_TEST_FLOAT(fIntersection, 4.0f / 5.0f, 0.0001f);
      XII_TEST_BOOL(vIntersection.IsEqual(vTarget + vDir, 0.0001f));

      XII_TEST_BOOL(s.GetLineSegmentIntersection(vTarget, vSource, &fIntersection, &vIntersection) == true);
      XII_TEST_FLOAT(fIntersection, 1.0f / 5.0f, 0.0001f);
      XII_TEST_BOOL(vIntersection.IsEqual(vTarget + vDir, 0.0001f));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TransformFromOrigin")
  {
    xiiBoundingSphereT s = xiiBoundingSphereT::MakeFromCenterAndRadius(xiiVec3T(1, 2, 3), 4);
    xiiMat4T           mTransform;

    mTransform = xiiMat4::MakeTranslation(xiiVec3T(5, 6, 7));
    mTransform.SetScalingFactors(xiiVec3T(4, 3, 2)).IgnoreResult();

    s.TransformFromOrigin(mTransform);

    XII_TEST_BOOL(s.m_vCenter == xiiVec3T(9, 12, 13));
    XII_TEST_BOOL(s.m_fRadius == 16);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TransformFromCenter")
  {
    xiiBoundingSphereT s = xiiBoundingSphereT::MakeFromCenterAndRadius(xiiVec3T(1, 2, 3), 4);
    xiiMat4T           mTransform;

    mTransform = xiiMat4::MakeTranslation(xiiVec3T(5, 6, 7));
    mTransform.SetScalingFactors(xiiVec3T(4, 3, 2)).IgnoreResult();

    s.TransformFromCenter(mTransform);

    XII_TEST_BOOL(s.m_vCenter == xiiVec3T(6, 8, 10));
    XII_TEST_BOOL(s.m_fRadius == 16);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsNaN")
  {
    if (xiiMath::SupportsNaN<xiiMathTestType>())
    {
      xiiBoundingSphereT s;

      s = xiiBoundingSphereT::MakeInvalid();
      XII_TEST_BOOL(!s.IsNaN());

      s           = xiiBoundingSphereT::MakeInvalid();
      s.m_fRadius = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(s.IsNaN());

      s             = xiiBoundingSphereT::MakeInvalid();
      s.m_vCenter.x = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(s.IsNaN());

      s             = xiiBoundingSphereT::MakeInvalid();
      s.m_vCenter.y = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(s.IsNaN());

      s             = xiiBoundingSphereT::MakeInvalid();
      s.m_vCenter.z = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(s.IsNaN());
    }
  }
}
