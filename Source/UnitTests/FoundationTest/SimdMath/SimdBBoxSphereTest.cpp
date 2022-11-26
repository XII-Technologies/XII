#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/SimdMath/SimdBBoxSphere.h>
#include <Foundation/SimdMath/SimdConversion.h>

XII_CREATE_SIMPLE_TEST(SimdMath, SimdBBoxSphere)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
    xiiSimdBBoxSphere b(xiiSimdVec4f(-1, -2, -3), xiiSimdVec4f(1, 2, 3), 2);

    XII_TEST_BOOL((b.m_CenterAndRadius == xiiSimdVec4f(-1, -2, -3, 2)).AllSet<4>());
    XII_TEST_BOOL((b.m_BoxHalfExtents == xiiSimdVec4f(1, 2, 3)).AllSet<3>());

    xiiSimdBBox    box(xiiSimdVec4f(1, 1, 1), xiiSimdVec4f(3, 3, 3));
    xiiSimdBSphere sphere(xiiSimdVec4f(2, 2, 2), 1);

    b = xiiSimdBBoxSphere(box, sphere);

    XII_TEST_BOOL((b.m_CenterAndRadius == xiiSimdVec4f(2, 2, 2, 1)).AllSet<4>());
    XII_TEST_BOOL((b.m_BoxHalfExtents == xiiSimdVec4f(1, 1, 1)).AllSet<3>());
    XII_TEST_BOOL(b.GetBox() == box);
    XII_TEST_BOOL(b.GetSphere() == sphere);

    b = xiiSimdBBoxSphere(box);

    XII_TEST_BOOL(b.m_CenterAndRadius.IsEqual(xiiSimdVec4f(2, 2, 2, xiiMath::Sqrt(3.0f)), 0.00001f).AllSet<4>());
    XII_TEST_BOOL((b.m_BoxHalfExtents == xiiSimdVec4f(1, 1, 1)).AllSet<3>());
    XII_TEST_BOOL(b.GetBox() == box);

    b = xiiSimdBBoxSphere(sphere);

    XII_TEST_BOOL((b.m_CenterAndRadius == xiiSimdVec4f(2, 2, 2, 1)).AllSet<4>());
    XII_TEST_BOOL((b.m_BoxHalfExtents == xiiSimdVec4f(1, 1, 1)).AllSet<3>());
    XII_TEST_BOOL(b.GetSphere() == sphere);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetInvalid")
  {
    xiiSimdBBoxSphere b;
    b.SetInvalid();

    XII_TEST_BOOL(!b.IsValid());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsNaN")
  {
    if (xiiMath::SupportsNaN<float>())
    {
      xiiSimdBBoxSphere b;

      b.SetInvalid();
      XII_TEST_BOOL(!b.IsNaN());

      b.SetInvalid();
      b.m_CenterAndRadius.SetX(xiiMath::NaN<float>());
      XII_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_CenterAndRadius.SetY(xiiMath::NaN<float>());
      XII_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_CenterAndRadius.SetZ(xiiMath::NaN<float>());
      XII_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_CenterAndRadius.SetW(xiiMath::NaN<float>());
      XII_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_BoxHalfExtents.SetX(xiiMath::NaN<float>());
      XII_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_BoxHalfExtents.SetY(xiiMath::NaN<float>());
      XII_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_BoxHalfExtents.SetZ(xiiMath::NaN<float>());
      XII_TEST_BOOL(b.IsNaN());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetFromPoints")
  {
    xiiSimdVec4f p[6] = {
      xiiSimdVec4f(-4, 0, 0),
      xiiSimdVec4f(5, 0, 0),
      xiiSimdVec4f(0, -6, 0),
      xiiSimdVec4f(0, 7, 0),
      xiiSimdVec4f(0, 0, -8),
      xiiSimdVec4f(0, 0, 9),
    };

    xiiSimdBBoxSphere b;
    b.SetFromPoints(p, 6);

    XII_TEST_BOOL((b.m_CenterAndRadius == xiiSimdVec4f(0.5, 0.5, 0.5)).AllSet<3>());
    XII_TEST_BOOL((b.m_BoxHalfExtents == xiiSimdVec4f(4.5, 6.5, 8.5)).AllSet<3>());
    XII_TEST_BOOL(b.m_CenterAndRadius.w().IsEqual(xiiSimdVec4f(0.5, 0.5, 8.5).GetLength<3>(), 0.00001f));
    XII_TEST_BOOL(b.m_CenterAndRadius.w() <= b.m_BoxHalfExtents.GetLength<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExpandToInclude")
  {
    xiiSimdBBoxSphere b1;
    b1.SetInvalid();
    xiiSimdBBoxSphere b2(xiiSimdBBox(xiiSimdVec4f(2, 2, 2), xiiSimdVec4f(4, 4, 4)));

    b1.ExpandToInclude(b2);
    XII_TEST_BOOL(b1 == b2);

    xiiSimdBSphere sphere(xiiSimdVec4f(2, 2, 2), 2);
    b2 = xiiSimdBBoxSphere(sphere);

    b1.ExpandToInclude(b2);
    XII_TEST_BOOL(b1 != b2);

    XII_TEST_BOOL((b1.m_CenterAndRadius == xiiSimdVec4f(2, 2, 2)).AllSet<3>());
    XII_TEST_BOOL((b1.m_BoxHalfExtents == xiiSimdVec4f(2, 2, 2)).AllSet<3>());
    XII_TEST_FLOAT(b1.m_CenterAndRadius.w(), xiiMath::Sqrt(3.0f) * 2.0f, 0.00001f);
    XII_TEST_BOOL(b1.m_CenterAndRadius.w() <= b1.m_BoxHalfExtents.GetLength<3>());

    b1.SetInvalid();
    b2 = xiiSimdBBox(xiiSimdVec4f(0.25, 0.25, 0.25), xiiSimdVec4f(0.5, 0.5, 0.5));

    b1.ExpandToInclude(b2);
    XII_TEST_BOOL(b1 == b2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Transform")
  {
    xiiSimdBBoxSphere b(xiiSimdVec4f(1), xiiSimdVec4f(5), 5);

    xiiSimdTransform t(xiiSimdVec4f(1, 1, 1), xiiSimdQuat::IdentityQuaternion(), xiiSimdVec4f(2, 3, -2));

    b.Transform(t);

    XII_TEST_BOOL((b.m_CenterAndRadius == xiiSimdVec4f(3, 4, -1, 15)).AllSet<4>());
    XII_TEST_BOOL((b.m_BoxHalfExtents == xiiSimdVec4f(10, 15, 10)).AllSet<3>());

    // verification
    xiiRandom rnd;
    rnd.Initialize(0x736454);

    xiiDynamicArray<xiiSimdVec4f, xiiAlignedAllocatorWrapper> points;
    points.SetCountUninitialized(10);
    float fSize = 10;

    for (xiiUInt32 i = 0; i < points.GetCount(); ++i)
    {
      float x   = (float)rnd.DoubleMinMax(-fSize, fSize);
      float y   = (float)rnd.DoubleMinMax(-fSize, fSize);
      float z   = (float)rnd.DoubleMinMax(-fSize, fSize);
      points[i] = xiiSimdVec4f(x, y, z);
    }

    b.SetFromPoints(points.GetData(), points.GetCount());

    t.m_Rotation.SetFromAxisAndAngle(xiiSimdVec4f(0, 0, 1), xiiAngle::Degree(-30));
    b.Transform(t);

    for (xiiUInt32 i = 0; i < points.GetCount(); ++i)
    {
      xiiSimdVec4f tp = t.TransformPosition(points[i]);

      xiiSimdFloat boxDist = b.GetBox().GetDistanceTo(tp);
      XII_TEST_BOOL(boxDist < xiiMath::DefaultEpsilon<float>());

      xiiSimdFloat sphereDist = b.GetSphere().GetDistanceTo(tp);
      XII_TEST_BOOL(sphereDist < xiiMath::DefaultEpsilon<float>());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Comparison")
  {
    xiiSimdBBoxSphere b1(xiiSimdBBox(xiiSimdVec4f(5, 0, 0), xiiSimdVec4f(1, 2, 3)));
    xiiSimdBBoxSphere b2(xiiSimdBBox(xiiSimdVec4f(6, 0, 0), xiiSimdVec4f(1, 2, 3)));

    XII_TEST_BOOL(b1 == xiiSimdBBoxSphere(xiiSimdBBox(xiiSimdVec4f(5, 0, 0), xiiSimdVec4f(1, 2, 3))));
    XII_TEST_BOOL(b1 != b2);
  }
}
