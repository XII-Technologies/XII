#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/SimdMath/SimdBBoxSphered.h>
#include <Foundation/SimdMath/SimdConversion.h>

XII_CREATE_SIMPLE_TEST(SimdMath, SimdBBoxSphered)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
    xiiSimdBBoxSphered b(xiiSimdVec4d(-1, -2, -3), xiiSimdVec4d(1, 2, 3), 2);

    XII_TEST_BOOL((b.m_CenterAndRadius == xiiSimdVec4d(-1, -2, -3, 2)).AllSet<4>());
    XII_TEST_BOOL((b.m_BoxHalfExtents == xiiSimdVec4d(1, 2, 3)).AllSet<3>());

    xiiSimdBBoxd    box(xiiSimdVec4d(1, 1, 1), xiiSimdVec4d(3, 3, 3));
    xiiSimdBSphered sphere(xiiSimdVec4d(2, 2, 2), 1);

    b = xiiSimdBBoxSphered(box, sphere);

    XII_TEST_BOOL((b.m_CenterAndRadius == xiiSimdVec4d(2, 2, 2, 1)).AllSet<4>());
    XII_TEST_BOOL((b.m_BoxHalfExtents == xiiSimdVec4d(1, 1, 1)).AllSet<3>());
    XII_TEST_BOOL(b.GetBox() == box);
    XII_TEST_BOOL(b.GetSphere() == sphere);

    b = xiiSimdBBoxSphered(box);

    XII_TEST_BOOL(b.m_CenterAndRadius.IsEqual(xiiSimdVec4d(2, 2, 2, xiiMath::Sqrt(3.0)), 0.00001).AllSet<4>());
    XII_TEST_BOOL((b.m_BoxHalfExtents == xiiSimdVec4d(1, 1, 1)).AllSet<3>());
    XII_TEST_BOOL(b.GetBox() == box);

    b = xiiSimdBBoxSphered(sphere);

    XII_TEST_BOOL((b.m_CenterAndRadius == xiiSimdVec4d(2, 2, 2, 1)).AllSet<4>());
    XII_TEST_BOOL((b.m_BoxHalfExtents == xiiSimdVec4d(1, 1, 1)).AllSet<3>());
    XII_TEST_BOOL(b.GetSphere() == sphere);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetInvalid")
  {
    xiiSimdBBoxSphered b;
    b.SetInvalid();

    XII_TEST_BOOL(!b.IsValid());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsNaN")
  {
    if (xiiMath::SupportsNaN<double>())
    {
      xiiSimdBBoxSphered b;

      b.SetInvalid();
      XII_TEST_BOOL(!b.IsNaN());

      b.SetInvalid();
      b.m_CenterAndRadius.SetX(xiiMath::NaN<double>());
      XII_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_CenterAndRadius.SetY(xiiMath::NaN<double>());
      XII_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_CenterAndRadius.SetZ(xiiMath::NaN<double>());
      XII_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_CenterAndRadius.SetW(xiiMath::NaN<double>());
      XII_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_BoxHalfExtents.SetX(xiiMath::NaN<double>());
      XII_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_BoxHalfExtents.SetY(xiiMath::NaN<double>());
      XII_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_BoxHalfExtents.SetZ(xiiMath::NaN<double>());
      XII_TEST_BOOL(b.IsNaN());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetFromPoints")
  {
    xiiSimdVec4d p[6] = {
      xiiSimdVec4d(-4, 0, 0),
      xiiSimdVec4d(5, 0, 0),
      xiiSimdVec4d(0, -6, 0),
      xiiSimdVec4d(0, 7, 0),
      xiiSimdVec4d(0, 0, -8),
      xiiSimdVec4d(0, 0, 9),
    };

    xiiSimdBBoxSphered b;
    b.SetFromPoints(p, 6);

    XII_TEST_BOOL((b.m_CenterAndRadius == xiiSimdVec4d(0.5, 0.5, 0.5)).AllSet<3>());
    XII_TEST_BOOL((b.m_BoxHalfExtents == xiiSimdVec4d(4.5, 6.5, 8.5)).AllSet<3>());
    XII_TEST_BOOL(b.m_CenterAndRadius.w().IsEqual(xiiSimdVec4d(0.5, 0.5, 8.5).GetLength<3>(), 0.00001));
    XII_TEST_BOOL(b.m_CenterAndRadius.w() <= b.m_BoxHalfExtents.GetLength<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExpandToInclude")
  {
    xiiSimdBBoxSphered b1;
    b1.SetInvalid();
    xiiSimdBBoxSphered b2(xiiSimdBBoxd(xiiSimdVec4d(2, 2, 2), xiiSimdVec4d(4, 4, 4)));

    b1.ExpandToInclude(b2);
    XII_TEST_BOOL(b1 == b2);

    xiiSimdBSphered sphere(xiiSimdVec4d(2, 2, 2), 2);
    b2 = xiiSimdBBoxSphered(sphere);

    b1.ExpandToInclude(b2);
    XII_TEST_BOOL(b1 != b2);

    XII_TEST_BOOL((b1.m_CenterAndRadius == xiiSimdVec4d(2, 2, 2)).AllSet<3>());
    XII_TEST_BOOL((b1.m_BoxHalfExtents == xiiSimdVec4d(2, 2, 2)).AllSet<3>());
    XII_TEST_FLOAT(b1.m_CenterAndRadius.w(), xiiMath::Sqrt(3.0) * 2.0, 0.00001);
    XII_TEST_BOOL(b1.m_CenterAndRadius.w() <= b1.m_BoxHalfExtents.GetLength<3>());

    b1.SetInvalid();
    b2 = xiiSimdBBoxd(xiiSimdVec4d(0.25, 0.25, 0.25), xiiSimdVec4d(0.5, 0.5, 0.5));

    b1.ExpandToInclude(b2);
    XII_TEST_BOOL(b1 == b2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Transform")
  {
    xiiSimdBBoxSphered b(xiiSimdVec4d(1), xiiSimdVec4d(5), 5);

    xiiSimdTransformd t(xiiSimdVec4d(1, 1, 1), xiiSimdQuatd::IdentityQuaternion(), xiiSimdVec4d(2, 3, -2));

    b.Transform(t);

    XII_TEST_BOOL((b.m_CenterAndRadius == xiiSimdVec4d(3, 4, -1, 15)).AllSet<4>());
    XII_TEST_BOOL((b.m_BoxHalfExtents == xiiSimdVec4d(10, 15, 10)).AllSet<3>());

    // verification
    xiiRandom rnd;
    rnd.Initialize(0x736454);

    xiiDynamicArray<xiiSimdVec4d, xiiAlignedAllocatorWrapper> points;
    points.SetCountUninitialized(10);
    double fSize = 10;

    for (xiiUInt32 i = 0; i < points.GetCount(); ++i)
    {
      double x  = (double)rnd.DoubleMinMax(-fSize, fSize);
      double y  = (double)rnd.DoubleMinMax(-fSize, fSize);
      double z  = (double)rnd.DoubleMinMax(-fSize, fSize);
      points[i] = xiiSimdVec4d(x, y, z);
    }

    b.SetFromPoints(points.GetData(), points.GetCount());

    t.m_Rotation.SetFromAxisAndAngle(xiiSimdVec4d(0, 0, 1), xiiAngled::Degree(-30));
    b.Transform(t);

    for (xiiUInt32 i = 0; i < points.GetCount(); ++i)
    {
      xiiSimdVec4d tp = t.TransformPosition(points[i]);

      xiiSimdDouble boxDist = b.GetBox().GetDistanceTo(tp);
      XII_TEST_BOOL(boxDist < xiiMath::DefaultEpsilon<double>());

      xiiSimdDouble sphereDist = b.GetSphere().GetDistanceTo(tp);
      XII_TEST_BOOL(sphereDist < xiiMath::DefaultEpsilon<double>());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Comparison")
  {
    xiiSimdBBoxSphered b1(xiiSimdBBoxd(xiiSimdVec4d(5, 0, 0), xiiSimdVec4d(1, 2, 3)));
    xiiSimdBBoxSphered b2(xiiSimdBBoxd(xiiSimdVec4d(6, 0, 0), xiiSimdVec4d(1, 2, 3)));

    XII_TEST_BOOL(b1 == xiiSimdBBoxSphered(xiiSimdBBoxd(xiiSimdVec4d(5, 0, 0), xiiSimdVec4d(1, 2, 3))));
    XII_TEST_BOOL(b1 != b2);
  }
}
