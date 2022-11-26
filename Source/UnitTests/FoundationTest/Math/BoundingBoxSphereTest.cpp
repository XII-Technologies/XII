#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/BoundingBoxSphere.h>

XII_CREATE_SIMPLE_TEST(Math, BoundingBoxSphere)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
    xiiBoundingBoxSphereT b(xiiVec3T(-1, -2, -3), xiiVec3T(1, 2, 3), 2);

    XII_TEST_BOOL(b.m_vCenter == xiiVec3T(-1, -2, -3));
    XII_TEST_BOOL(b.m_vBoxHalfExtends == xiiVec3T(1, 2, 3));
    XII_TEST_BOOL(b.m_fSphereRadius == 2);

    xiiBoundingBoxT    box(xiiVec3T(1, 1, 1), xiiVec3T(3, 3, 3));
    xiiBoundingSphereT sphere(xiiVec3T(2, 2, 2), 1);

    b = xiiBoundingBoxSphereT(box, sphere);

    XII_TEST_BOOL(b.m_vCenter == xiiVec3T(2, 2, 2));
    XII_TEST_BOOL(b.m_vBoxHalfExtends == xiiVec3T(1, 1, 1));
    XII_TEST_BOOL(b.m_fSphereRadius == 1);
    XII_TEST_BOOL(b.GetBox() == box);
    XII_TEST_BOOL(b.GetSphere() == sphere);

    b = xiiBoundingBoxSphereT(box);

    XII_TEST_BOOL(b.m_vCenter == xiiVec3T(2, 2, 2));
    XII_TEST_BOOL(b.m_vBoxHalfExtends == xiiVec3T(1, 1, 1));
    XII_TEST_FLOAT(b.m_fSphereRadius, xiiMath::Sqrt(xiiMathTestType(3)), 0.00001f);
    XII_TEST_BOOL(b.GetBox() == box);

    b = xiiBoundingBoxSphereT(sphere);

    XII_TEST_BOOL(b.m_vCenter == xiiVec3T(2, 2, 2));
    XII_TEST_BOOL(b.m_vBoxHalfExtends == xiiVec3T(1, 1, 1));
    XII_TEST_BOOL(b.m_fSphereRadius == 1);
    XII_TEST_BOOL(b.GetSphere() == sphere);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetFromPoints")
  {
    xiiVec3T p[6] = {
      xiiVec3T(-4, 0, 0),
      xiiVec3T(5, 0, 0),
      xiiVec3T(0, -6, 0),
      xiiVec3T(0, 7, 0),
      xiiVec3T(0, 0, -8),
      xiiVec3T(0, 0, 9),
    };

    xiiBoundingBoxSphereT b;
    b.SetFromPoints(p, 6);

    XII_TEST_BOOL(b.m_vCenter == xiiVec3T(0.5, 0.5, 0.5));
    XII_TEST_BOOL(b.m_vBoxHalfExtends == xiiVec3T(4.5, 6.5, 8.5));
    XII_TEST_FLOAT(b.m_fSphereRadius, xiiVec3T(0.5, 0.5, 8.5).GetLength(), 0.00001f);
    XII_TEST_BOOL(b.m_fSphereRadius <= b.m_vBoxHalfExtends.GetLength());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetInvalid")
  {
    xiiBoundingBoxSphereT b;
    b.SetInvalid();

    XII_TEST_BOOL(!b.IsValid());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExpandToInclude")
  {
    xiiBoundingBoxSphereT b1;
    b1.SetInvalid();
    xiiBoundingBoxSphereT b2(xiiBoundingBoxT(xiiVec3T(2, 2, 2), xiiVec3T(4, 4, 4)));

    b1.ExpandToInclude(b2);
    XII_TEST_BOOL(b1 == b2);

    xiiBoundingSphereT sphere(xiiVec3T(2, 2, 2), 2);
    b2 = xiiBoundingBoxSphereT(sphere);

    b1.ExpandToInclude(b2);
    XII_TEST_BOOL(b1 != b2);

    XII_TEST_BOOL(b1.m_vCenter == xiiVec3T(2, 2, 2));
    XII_TEST_BOOL(b1.m_vBoxHalfExtends == xiiVec3T(2, 2, 2));
    XII_TEST_FLOAT(b1.m_fSphereRadius, xiiMath::Sqrt(xiiMathTestType(3)) * 2, 0.00001f);
    XII_TEST_BOOL(b1.m_fSphereRadius <= b1.m_vBoxHalfExtends.GetLength());

    b1.SetInvalid();
    b2 = xiiBoundingBoxT(xiiVec3T(0.25, 0.25, 0.25), xiiVec3T(0.5, 0.5, 0.5));

    b1.ExpandToInclude(b2);
    XII_TEST_BOOL(b1 == b2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Transform")
  {
    xiiBoundingBoxSphereT b(xiiVec3T(1), xiiVec3T(5), 5);

    xiiMat4T m;
    m.SetScalingMatrix(xiiVec3T(-2, -3, -2));
    m.SetTranslationVector(xiiVec3T(1, 1, 1));

    b.Transform(m);

    XII_TEST_BOOL(b.m_vCenter == xiiVec3T(-1, -2, -1));
    XII_TEST_BOOL(b.m_vBoxHalfExtends == xiiVec3T(10, 15, 10));
    XII_TEST_BOOL(b.m_fSphereRadius == 15);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsNaN")
  {
    if (xiiMath::SupportsNaN<xiiMathTestType>())
    {
      xiiBoundingBoxSphereT b;

      b.SetInvalid();
      XII_TEST_BOOL(!b.IsNaN());

      b.SetInvalid();
      b.m_vCenter.x = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_vCenter.y = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_vCenter.z = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_vBoxHalfExtends.x = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_vBoxHalfExtends.y = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_vBoxHalfExtends.z = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_fSphereRadius = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(b.IsNaN());
    }
  }
}
