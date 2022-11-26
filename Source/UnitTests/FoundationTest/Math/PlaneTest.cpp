#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Math/Plane.h>
#include <Foundation/Math/Random.h>

XII_CREATE_SIMPLE_TEST(Math, Plane)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Default Constructor")
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    if (xiiMath::SupportsNaN<xiiMat3T::ComponentType>())
    {
      // In debug the default constructor initializes everything with NaN.
      xiiPlaneT p;
      XII_TEST_BOOL(xiiMath::IsNaN(p.m_vNormal.x) && xiiMath::IsNaN(p.m_vNormal.y) && xiiMath::IsNaN(p.m_vNormal.z) && xiiMath::IsNaN(p.m_fNegDistance));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    xiiPlaneT::ComponentType testBlock[4] = {(xiiPlaneT::ComponentType)1, (xiiPlaneT::ComponentType)2, (xiiPlaneT::ComponentType)3, (xiiPlaneT::ComponentType)4};
    xiiPlaneT*               p            = ::new ((void*)&testBlock[0]) xiiPlaneT;
    XII_TEST_BOOL(p->m_vNormal.x == (xiiPlaneT::ComponentType)1 && p->m_vNormal.y == (xiiPlaneT::ComponentType)2 && p->m_vNormal.z == (xiiPlaneT::ComponentType)3 && p->m_fNegDistance == (xiiPlaneT::ComponentType)4);
#endif
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor(Normal, Point)")
  {
    xiiPlaneT p(xiiVec3T(1, 0, 0), xiiVec3T(5, 3, 1));

    XII_TEST_BOOL(p.m_vNormal == xiiVec3T(1, 0, 0));
    XII_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor(Point, Point, Point)")
  {
    xiiPlaneT p(xiiVec3T(-1, 5, 1), xiiVec3T(1, 5, 1), xiiVec3T(0, 5, -5));

    XII_TEST_VEC3(p.m_vNormal, xiiVec3T(0, 1, 0), 0.0001f);
    XII_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor(Points)")
  {
    xiiVec3T v[3] = {xiiVec3T(-1, 5, 1), xiiVec3T(1, 5, 1), xiiVec3T(0, 5, -5)};

    xiiPlaneT p(v);

    XII_TEST_VEC3(p.m_vNormal, xiiVec3T(0, 1, 0), 0.0001f);
    XII_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor(Points, numpoints)")
  {
    xiiVec3T v[6] = {xiiVec3T(-1, 5, 1), xiiVec3T(-1, 5, 1), xiiVec3T(1, 5, 1), xiiVec3T(1, 5, 1), xiiVec3T(0, 5, -5), xiiVec3T(0, 5, -5)};

    xiiPlaneT p(v, 6);

    XII_TEST_VEC3(p.m_vNormal, xiiVec3T(0, 1, 0), 0.0001f);
    XII_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetFromNormalAndPoint")
  {
    xiiPlaneT p;
    p.SetFromNormalAndPoint(xiiVec3T(1, 0, 0), xiiVec3T(5, 3, 1));

    XII_TEST_BOOL(p.m_vNormal == xiiVec3T(1, 0, 0));
    XII_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetFromPoints")
  {
    xiiPlaneT p;
    p.SetFromPoints(xiiVec3T(-1, 5, 1), xiiVec3T(1, 5, 1), xiiVec3T(0, 5, -5)).IgnoreResult();

    XII_TEST_VEC3(p.m_vNormal, xiiVec3T(0, 1, 0), 0.0001f);
    XII_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetFromPoints")
  {
    xiiVec3T v[3] = {xiiVec3T(-1, 5, 1), xiiVec3T(1, 5, 1), xiiVec3T(0, 5, -5)};

    xiiPlaneT p;
    p.SetFromPoints(v).IgnoreResult();

    XII_TEST_VEC3(p.m_vNormal, xiiVec3T(0, 1, 0), 0.0001f);
    XII_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetFromPoints")
  {
    xiiVec3T v[6] = {xiiVec3T(-1, 5, 1), xiiVec3T(-1, 5, 1), xiiVec3T(1, 5, 1), xiiVec3T(1, 5, 1), xiiVec3T(0, 5, -5), xiiVec3T(0, 5, -5)};

    xiiPlaneT p;
    p.SetFromPoints(v, 6).IgnoreResult();

    XII_TEST_VEC3(p.m_vNormal, xiiVec3T(0, 1, 0), 0.0001f);
    XII_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetFromDirections")
  {
    xiiPlaneT p;
    p.SetFromDirections(xiiVec3T(1, 0, 0), xiiVec3T(1, 0, -1), xiiVec3T(3, 5, 9)).IgnoreResult();

    XII_TEST_VEC3(p.m_vNormal, xiiVec3T(0, 1, 0), 0.0001f);
    XII_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetInvalid")
  {
    xiiPlaneT p;
    p.SetFromDirections(xiiVec3T(1, 0, 0), xiiVec3T(1, 0, -1), xiiVec3T(3, 5, 9)).IgnoreResult();

    p.SetInvalid();

    XII_TEST_VEC3(p.m_vNormal, xiiVec3T(0, 0, 0), 0.0001f);
    XII_TEST_FLOAT(p.m_fNegDistance, 0.0f, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetDistanceTo")
  {
    xiiPlaneT p(xiiVec3T(1, 0, 0), xiiVec3T(5, 0, 0));

    XII_TEST_FLOAT(p.GetDistanceTo(xiiVec3T(10, 3, 5)), 5.0f, 0.0001f);
    XII_TEST_FLOAT(p.GetDistanceTo(xiiVec3T(0, 7, 123)), -5.0f, 0.0001f);
    XII_TEST_FLOAT(p.GetDistanceTo(xiiVec3T(5, 12, 23)), 0.0f, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetMinimumDistanceTo")
  {
    xiiVec3T v1[3] = {xiiVec3T(15, 3, 5), xiiVec3T(6, 7, 123), xiiVec3T(10, 12, 23)};
    xiiVec3T v2[3] = {xiiVec3T(3, 3, 5), xiiVec3T(5, 7, 123), xiiVec3T(10, 12, 23)};

    xiiPlaneT p(xiiVec3T(1, 0, 0), xiiVec3T(5, 0, 0));

    XII_TEST_FLOAT(p.GetMinimumDistanceTo(v1, 3), 1.0f, 0.0001f);
    XII_TEST_FLOAT(p.GetMinimumDistanceTo(v2, 3), -2.0f, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetMinMaxDistanceTo")
  {
    xiiVec3T v1[3] = {xiiVec3T(15, 3, 5), xiiVec3T(5, 7, 123), xiiVec3T(0, 12, 23)};
    xiiVec3T v2[3] = {xiiVec3T(8, 3, 5), xiiVec3T(6, 7, 123), xiiVec3T(10, 12, 23)};

    xiiPlaneT p(xiiVec3T(1, 0, 0), xiiVec3T(5, 0, 0));

    xiiMathTestType fmin, fmax;

    p.GetMinMaxDistanceTo(fmin, fmax, v1, 3);
    XII_TEST_FLOAT(fmin, -5.0f, 0.0001f);
    XII_TEST_FLOAT(fmax, 10.0f, 0.0001f);

    p.GetMinMaxDistanceTo(fmin, fmax, v2, 3);
    XII_TEST_FLOAT(fmin, 1, 0.0001f);
    XII_TEST_FLOAT(fmax, 5, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetPointPosition")
  {
    xiiPlaneT p(xiiVec3T(0, 1, 0), xiiVec3T(0, 10, 0));

    XII_TEST_BOOL(p.GetPointPosition(xiiVec3T(0, 15, 0)) == xiiPositionOnPlane::Front);
    XII_TEST_BOOL(p.GetPointPosition(xiiVec3T(0, 5, 0)) == xiiPositionOnPlane::Back);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetPointPosition(planewidth)")
  {
    xiiPlaneT p(xiiVec3T(0, 1, 0), xiiVec3T(0, 10, 0));

    XII_TEST_BOOL(p.GetPointPosition(xiiVec3T(0, 15, 0), 0.01f) == xiiPositionOnPlane::Front);
    XII_TEST_BOOL(p.GetPointPosition(xiiVec3T(0, 5, 0), 0.01f) == xiiPositionOnPlane::Back);
    XII_TEST_BOOL(p.GetPointPosition(xiiVec3T(0, 10, 0), 0.01f) == xiiPositionOnPlane::OnPlane);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetObjectPosition")
  {
    xiiPlaneT p(xiiVec3T(1, 0, 0), xiiVec3T(10, 0, 0));

    xiiVec3T v0[3] = {xiiVec3T(12, 0, 0), xiiVec3T(15, 0, 0), xiiVec3T(20, 0, 0)};
    xiiVec3T v1[3] = {xiiVec3T(8, 0, 0), xiiVec3T(6, 0, 0), xiiVec3T(4, 0, 0)};
    xiiVec3T v2[3] = {xiiVec3T(12, 0, 0), xiiVec3T(6, 0, 0), xiiVec3T(4, 0, 0)};

    XII_TEST_BOOL(p.GetObjectPosition(v0, 3) == xiiPositionOnPlane::Front);
    XII_TEST_BOOL(p.GetObjectPosition(v1, 3) == xiiPositionOnPlane::Back);
    XII_TEST_BOOL(p.GetObjectPosition(v2, 3) == xiiPositionOnPlane::Spanning);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetObjectPosition(fPlaneHalfWidth)")
  {
    xiiPlaneT p(xiiVec3T(1, 0, 0), xiiVec3T(10, 0, 0));

    xiiVec3T v0[3] = {xiiVec3T(12, 0, 0), xiiVec3T(15, 0, 0), xiiVec3T(20, 0, 0)};
    xiiVec3T v1[3] = {xiiVec3T(8, 0, 0), xiiVec3T(6, 0, 0), xiiVec3T(4, 0, 0)};
    xiiVec3T v2[3] = {xiiVec3T(12, 0, 0), xiiVec3T(6, 0, 0), xiiVec3T(4, 0, 0)};
    xiiVec3T v3[3] = {xiiVec3T(10, 1, 0), xiiVec3T(10, 5, 7), xiiVec3T(10, 3, -5)};

    XII_TEST_BOOL(p.GetObjectPosition(v0, 3, 0.001f) == xiiPositionOnPlane::Front);
    XII_TEST_BOOL(p.GetObjectPosition(v1, 3, 0.001f) == xiiPositionOnPlane::Back);
    XII_TEST_BOOL(p.GetObjectPosition(v2, 3, 0.001f) == xiiPositionOnPlane::Spanning);
    XII_TEST_BOOL(p.GetObjectPosition(v3, 3, 0.001f) == xiiPositionOnPlane::OnPlane);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetObjectPosition(sphere)")
  {
    xiiPlaneT p(xiiVec3T(1, 0, 0), xiiVec3T(10, 0, 0));

    XII_TEST_BOOL(p.GetObjectPosition(xiiBoundingSphereT(xiiVec3T(15, 2, 3), 3.0f)) == xiiPositionOnPlane::Front);
    XII_TEST_BOOL(p.GetObjectPosition(xiiBoundingSphereT(xiiVec3T(5, 2, 3), 3.0f)) == xiiPositionOnPlane::Back);
    XII_TEST_BOOL(p.GetObjectPosition(xiiBoundingSphereT(xiiVec3T(15, 2, 4.999f), 3.0f)) == xiiPositionOnPlane::Front);
    XII_TEST_BOOL(p.GetObjectPosition(xiiBoundingSphereT(xiiVec3T(5, 2, 3), 4.999f)) == xiiPositionOnPlane::Back);
    XII_TEST_BOOL(p.GetObjectPosition(xiiBoundingSphereT(xiiVec3T(8, 2, 3), 3.0f)) == xiiPositionOnPlane::Spanning);
    XII_TEST_BOOL(p.GetObjectPosition(xiiBoundingSphereT(xiiVec3T(12, 2, 3), 3.0f)) == xiiPositionOnPlane::Spanning);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetObjectPosition(box)")
  {
    {
      xiiPlaneT p(xiiVec3T(1, 0, 0), xiiVec3T(10, 0, 0));
      XII_TEST_BOOL(p.GetObjectPosition(xiiBoundingBoxT(xiiVec3T(10.1f), xiiVec3T(15))) == xiiPositionOnPlane::Front);
      XII_TEST_BOOL(p.GetObjectPosition(xiiBoundingBoxT(xiiVec3T(7), xiiVec3T(9.9f))) == xiiPositionOnPlane::Back);
      XII_TEST_BOOL(p.GetObjectPosition(xiiBoundingBoxT(xiiVec3T(7), xiiVec3T(15))) == xiiPositionOnPlane::Spanning);
    }
    {
      xiiPlaneT p(xiiVec3T(0, 1, 0), xiiVec3T(0, 10, 0));
      XII_TEST_BOOL(p.GetObjectPosition(xiiBoundingBoxT(xiiVec3T(10.1f), xiiVec3T(15))) == xiiPositionOnPlane::Front);
      XII_TEST_BOOL(p.GetObjectPosition(xiiBoundingBoxT(xiiVec3T(7), xiiVec3T(9.9f))) == xiiPositionOnPlane::Back);
      XII_TEST_BOOL(p.GetObjectPosition(xiiBoundingBoxT(xiiVec3T(7), xiiVec3T(15))) == xiiPositionOnPlane::Spanning);
    }
    {
      xiiPlaneT p(xiiVec3T(0, 0, 1), xiiVec3T(0, 0, 10));
      XII_TEST_BOOL(p.GetObjectPosition(xiiBoundingBoxT(xiiVec3T(10.1f), xiiVec3T(15))) == xiiPositionOnPlane::Front);
      XII_TEST_BOOL(p.GetObjectPosition(xiiBoundingBoxT(xiiVec3T(7), xiiVec3T(9.9f))) == xiiPositionOnPlane::Back);
      XII_TEST_BOOL(p.GetObjectPosition(xiiBoundingBoxT(xiiVec3T(7), xiiVec3T(15))) == xiiPositionOnPlane::Spanning);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ProjectOntoPlane")
  {
    xiiPlaneT p(xiiVec3T(0, 1, 0), xiiVec3T(0, 10, 0));

    XII_TEST_VEC3(p.ProjectOntoPlane(xiiVec3T(3, 15, 2)), xiiVec3T(3, 10, 2), 0.001f);
    XII_TEST_VEC3(p.ProjectOntoPlane(xiiVec3T(-1, 5, -5)), xiiVec3T(-1, 10, -5), 0.001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Mirror")
  {
    xiiPlaneT p(xiiVec3T(0, 1, 0), xiiVec3T(0, 10, 0));

    XII_TEST_VEC3(p.Mirror(xiiVec3T(3, 15, 2)), xiiVec3T(3, 5, 2), 0.001f);
    XII_TEST_VEC3(p.Mirror(xiiVec3T(-1, 5, -5)), xiiVec3T(-1, 15, -5), 0.001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetCoplanarDirection")
  {
    xiiPlaneT p(xiiVec3T(0, 1, 0), xiiVec3T(0, 10, 0));

    XII_TEST_VEC3(p.GetCoplanarDirection(xiiVec3T(0, 1, 0)), xiiVec3T(0, 0, 0), 0.001f);
    XII_TEST_VEC3(p.GetCoplanarDirection(xiiVec3T(1, 1, 0)).GetNormalized(), xiiVec3T(1, 0, 0), 0.001f);
    XII_TEST_VEC3(p.GetCoplanarDirection(xiiVec3T(-1, 1, 0)).GetNormalized(), xiiVec3T(-1, 0, 0), 0.001f);
    XII_TEST_VEC3(p.GetCoplanarDirection(xiiVec3T(0, 1, 1)).GetNormalized(), xiiVec3T(0, 0, 1), 0.001f);
    XII_TEST_VEC3(p.GetCoplanarDirection(xiiVec3T(0, 1, -1)).GetNormalized(), xiiVec3T(0, 0, -1), 0.001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsIdentical / operator== / operator!=")
  {
    xiiPlaneT p1(xiiVec3T(0, 1, 0), xiiVec3T(0, 10, 0));
    xiiPlaneT p2(xiiVec3T(0, 1, 0), xiiVec3T(0, 10, 0));
    xiiPlaneT p3(xiiVec3T(0, 1, 0), xiiVec3T(0, 10.00001f, 0));

    XII_TEST_BOOL(p1.IsIdentical(p1));
    XII_TEST_BOOL(p2.IsIdentical(p2));
    XII_TEST_BOOL(p3.IsIdentical(p3));

    XII_TEST_BOOL(p1.IsIdentical(p2));
    XII_TEST_BOOL(p2.IsIdentical(p1));

    XII_TEST_BOOL(!p1.IsIdentical(p3));
    XII_TEST_BOOL(!p2.IsIdentical(p3));


    XII_TEST_BOOL(p1 == p2);
    XII_TEST_BOOL(p2 == p1);

    XII_TEST_BOOL(p1 != p3);
    XII_TEST_BOOL(p2 != p3);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsEqual")
  {
    xiiPlaneT p1(xiiVec3T(0, 1, 0), xiiVec3T(0, 10, 0));
    xiiPlaneT p2(xiiVec3T(0, 1, 0), xiiVec3T(0, 10, 0));
    xiiPlaneT p3(xiiVec3T(0, 1, 0), xiiVec3T(0, 10.00001f, 0));

    XII_TEST_BOOL(p1.IsEqual(p1));
    XII_TEST_BOOL(p2.IsEqual(p2));
    XII_TEST_BOOL(p3.IsEqual(p3));

    XII_TEST_BOOL(p1.IsEqual(p2));
    XII_TEST_BOOL(p2.IsEqual(p1));

    XII_TEST_BOOL(p1.IsEqual(p3));
    XII_TEST_BOOL(p2.IsEqual(p3));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsValid")
  {
    xiiPlaneT p1(xiiVec3T(0, 1, 0), xiiVec3T(0, 10, 0));

    XII_TEST_BOOL(p1.IsValid());

    p1.SetInvalid();
    XII_TEST_BOOL(!p1.IsValid());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Transform(Mat3)")
  {
    xiiPlaneT p(xiiVec3T(0, 1, 0), xiiVec3T(0, 10, 0));

    xiiMat3T m;
    m.SetRotationMatrixX(xiiAngle::Degree(90));

    p.Transform(m);

    XII_TEST_VEC3(p.m_vNormal, xiiVec3T(0, 0, 1), 0.0001f);
    XII_TEST_FLOAT(p.m_fNegDistance, -10.0f, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Transform(Mat4)")
  {
    {
      xiiPlaneT p(xiiVec3T(0, 1, 0), xiiVec3T(0, 10, 0));

      xiiMat4T m;
      m.SetRotationMatrixX(xiiAngle::Degree(90));
      m.SetTranslationVector(xiiVec3T(0, 5, 0));

      p.Transform(m);

      XII_TEST_VEC3(p.m_vNormal, xiiVec3T(0, 0, 1), 0.0001f);
      XII_TEST_FLOAT(p.m_fNegDistance, -10.0f, 0.0001f);
    }

    {
      xiiPlaneT p(xiiVec3T(0, 1, 0), xiiVec3T(0, 10, 0));

      xiiMat4T m;
      m.SetRotationMatrixX(xiiAngle::Degree(90));
      m.SetTranslationVector(xiiVec3T(0, 0, 5));

      p.Transform(m);

      XII_TEST_VEC3(p.m_vNormal, xiiVec3T(0, 0, 1), 0.0001f);
      XII_TEST_FLOAT(p.m_fNegDistance, -15.0f, 0.0001f);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Flip")
  {
    xiiPlaneT p(xiiVec3T(0, 1, 0), xiiVec3T(0, 10, 0));

    XII_TEST_VEC3(p.m_vNormal, xiiVec3T(0, 1, 0), 0.0001f);
    XII_TEST_FLOAT(p.m_fNegDistance, -10.0f, 0.0001f);

    p.Flip();

    XII_TEST_VEC3(p.m_vNormal, xiiVec3T(0, -1, 0), 0.0001f);
    XII_TEST_FLOAT(p.m_fNegDistance, 10.0f, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FlipIfNecessary")
  {
    {
      xiiPlaneT p(xiiVec3T(0, 1, 0), xiiVec3T(0, 10, 0));

      XII_TEST_VEC3(p.m_vNormal, xiiVec3T(0, 1, 0), 0.0001f);
      XII_TEST_FLOAT(p.m_fNegDistance, -10.0f, 0.0001f);

      XII_TEST_BOOL(p.FlipIfNecessary(xiiVec3T(0, 11, 0), true) == false);

      XII_TEST_VEC3(p.m_vNormal, xiiVec3T(0, 1, 0), 0.0001f);
      XII_TEST_FLOAT(p.m_fNegDistance, -10.0f, 0.0001f);
    }

    {
      xiiPlaneT p(xiiVec3T(0, 1, 0), xiiVec3T(0, 10, 0));

      XII_TEST_VEC3(p.m_vNormal, xiiVec3T(0, 1, 0), 0.0001f);
      XII_TEST_FLOAT(p.m_fNegDistance, -10.0f, 0.0001f);

      XII_TEST_BOOL(p.FlipIfNecessary(xiiVec3T(0, 11, 0), false) == true);

      XII_TEST_VEC3(p.m_vNormal, xiiVec3T(0, -1, 0), 0.0001f);
      XII_TEST_FLOAT(p.m_fNegDistance, 10.0f, 0.0001f);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetRayIntersection")
  {
    xiiPlaneT p(xiiVec3T(0, 1, 0), xiiVec3T(0, 10, 0));

    xiiMathTestType f;
    xiiVec3T        v;

    XII_TEST_BOOL(p.GetRayIntersection(xiiVec3T(3, 1, 7), xiiVec3T(0, 1, 0), &f, &v));
    XII_TEST_FLOAT(f, 9, 0.0001f);
    XII_TEST_VEC3(v, xiiVec3T(3, 10, 7), 0.0001f);

    XII_TEST_BOOL(p.GetRayIntersection(xiiVec3T(3, 20, 7), xiiVec3T(0, -1, 0), &f, &v));
    XII_TEST_FLOAT(f, 10, 0.0001f);
    XII_TEST_VEC3(v, xiiVec3T(3, 10, 7), 0.0001f);

    XII_TEST_BOOL(!p.GetRayIntersection(xiiVec3T(3, 1, 7), xiiVec3T(1, 0, 0), &f, &v));
    XII_TEST_BOOL(!p.GetRayIntersection(xiiVec3T(3, 1, 7), xiiVec3T(0, -1, 0), &f, &v));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetRayIntersectionBiDirectional")
  {
    xiiPlaneT p(xiiVec3T(0, 1, 0), xiiVec3T(0, 10, 0));

    xiiMathTestType f;
    xiiVec3T        v;

    XII_TEST_BOOL(p.GetRayIntersectionBiDirectional(xiiVec3T(3, 1, 7), xiiVec3T(0, 1, 0), &f, &v));
    XII_TEST_FLOAT(f, 9, 0.0001f);
    XII_TEST_VEC3(v, xiiVec3T(3, 10, 7), 0.0001f);

    XII_TEST_BOOL(!p.GetRayIntersectionBiDirectional(xiiVec3T(3, 1, 7), xiiVec3T(1, 0, 0), &f, &v));

    XII_TEST_BOOL(p.GetRayIntersectionBiDirectional(xiiVec3T(3, 1, 7), xiiVec3T(0, -1, 0), &f, &v));
    XII_TEST_FLOAT(f, -9, 0.0001f);
    XII_TEST_VEC3(v, xiiVec3T(3, 10, 7), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetLineSegmentIntersection")
  {
    xiiPlaneT p(xiiVec3T(0, 1, 0), xiiVec3T(0, 10, 0));

    xiiMathTestType f;
    xiiVec3T        v;

    XII_TEST_BOOL(p.GetLineSegmentIntersection(xiiVec3T(3, 5, 7), xiiVec3T(3, 15, 7), &f, &v));
    XII_TEST_FLOAT(f, 0.5f, 0.0001f);
    XII_TEST_VEC3(v, xiiVec3T(3, 10, 7), 0.0001f);

    XII_TEST_BOOL(!p.GetLineSegmentIntersection(xiiVec3T(3, 5, 7), xiiVec3T(13, 5, 7), &f, &v));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetPlanesIntersectionPoint")
  {
    xiiPlaneT p1(xiiVec3T(1, 0, 0), xiiVec3T(0, 10, 0));
    xiiPlaneT p2(xiiVec3T(0, 1, 0), xiiVec3T(0, 10, 0));
    xiiPlaneT p3(xiiVec3T(0, 0, 1), xiiVec3T(0, 10, 0));

    xiiVec3T r;

    XII_TEST_BOOL(xiiPlaneT::GetPlanesIntersectionPoint(p1, p2, p3, r) == XII_SUCCESS);
    XII_TEST_VEC3(r, xiiVec3T(0, 10, 0), 0.0001f);

    XII_TEST_BOOL(xiiPlaneT::GetPlanesIntersectionPoint(p1, p1, p3, r) == XII_FAILURE);
    XII_TEST_BOOL(xiiPlaneT::GetPlanesIntersectionPoint(p1, p2, p2, r) == XII_FAILURE);
    XII_TEST_BOOL(xiiPlaneT::GetPlanesIntersectionPoint(p3, p2, p3, r) == XII_FAILURE);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FindSupportPoints")
  {
    xiiVec3T v[6] = {xiiVec3T(-1, 5, 1), xiiVec3T(-1, 5, 1), xiiVec3T(1, 5, 1), xiiVec3T(1, 5, 1), xiiVec3T(0, 5, -5), xiiVec3T(0, 5, -5)};

    xiiInt32 i1, i2, i3;

    xiiPlaneT::FindSupportPoints(v, 6, i1, i2, i3).IgnoreResult();

    XII_TEST_INT(i1, 0);
    XII_TEST_INT(i2, 2);
    XII_TEST_INT(i3, 4);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsNaN")
  {
    if (xiiMath::SupportsNaN<xiiMathTestType>())
    {
      xiiPlaneT p;

      p.SetInvalid();
      XII_TEST_BOOL(!p.IsNaN());

      p.SetInvalid();
      p.m_fNegDistance = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(p.IsNaN());

      p.SetInvalid();
      p.m_vNormal.x = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(p.IsNaN());

      p.SetInvalid();
      p.m_vNormal.y = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(p.IsNaN());

      p.SetInvalid();
      p.m_vNormal.z = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(p.IsNaN());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsFinite")
  {
    if (xiiMath::SupportsInfinity<xiiMathTestType>())
    {
      xiiPlaneT p;

      p.m_vNormal      = xiiVec3(1, 2, 3).GetNormalized();
      p.m_fNegDistance = 42;
      XII_TEST_BOOL(p.IsValid());
      XII_TEST_BOOL(p.IsFinite());

      p.SetInvalid();
      p.m_vNormal      = xiiVec3(1, 2, 3).GetNormalized();
      p.m_fNegDistance = xiiMath::Infinity<xiiMathTestType>();
      XII_TEST_BOOL(p.IsValid());
      XII_TEST_BOOL(!p.IsFinite());

      p.SetInvalid();
      p.m_vNormal.x    = xiiMath::NaN<xiiMathTestType>();
      p.m_fNegDistance = xiiMath::Infinity<xiiMathTestType>();
      XII_TEST_BOOL(!p.IsValid());
      XII_TEST_BOOL(!p.IsFinite());

      p.SetInvalid();
      p.m_vNormal      = xiiVec3(1, 2, 3);
      p.m_fNegDistance = 42;
      XII_TEST_BOOL(!p.IsValid());
      XII_TEST_BOOL(p.IsFinite());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetMinimumDistanceTo/GetMaximumDistanceTo")
  {
    const xiiUInt32 numTestLoops = 1000 * 1000;

    xiiRandom randomGenerator;
    randomGenerator.Initialize(0x83482343);

    const auto randomNonZeroVec3T = [&randomGenerator]() -> xiiVec3T {
      const float    extent = 1000.f;
      const xiiVec3T v(randomGenerator.FloatMinMax(-extent, extent), randomGenerator.FloatMinMax(-extent, extent), randomGenerator.FloatMinMax(-extent, extent));
      return v.GetLength() > 0.001f ? v : xiiVec3T::UnitXAxis();
    };

    for (xiiUInt32 loopIndex = 0; loopIndex < numTestLoops; ++loopIndex)
    {
      const xiiPlaneT plane(randomNonZeroVec3T().GetNormalized(), randomNonZeroVec3T());

      xiiVec3T        boxCorners[8];
      xiiBoundingBoxT box;
      {
        const xiiVec3T boxPoint0 = randomNonZeroVec3T();
        const xiiVec3T boxPoint1 = randomNonZeroVec3T();
        const xiiVec3T boxMins(xiiMath::Min(boxPoint0.x, boxPoint1.x), xiiMath::Min(boxPoint0.y, boxPoint1.y), xiiMath::Min(boxPoint0.z, boxPoint1.z));
        const xiiVec3T boxMaxs(xiiMath::Max(boxPoint0.x, boxPoint1.x), xiiMath::Max(boxPoint0.y, boxPoint1.y), xiiMath::Max(boxPoint0.z, boxPoint1.z));
        box = xiiBoundingBoxT(boxMins, boxMaxs);
        box.GetCorners(boxCorners);
      }

      float distanceMin;
      float distanceMax;
      {
        distanceMin = plane.GetMinimumDistanceTo(box);
        distanceMax = plane.GetMaximumDistanceTo(box);
      }

      float referenceDistanceMin = FLT_MAX;
      float referenceDistanceMax = -FLT_MAX;
      {
        for (xiiUInt32 cornerIndex = 0; cornerIndex < XII_ARRAY_SIZE(boxCorners); ++cornerIndex)
        {
          const float cornerDist = plane.GetDistanceTo(boxCorners[cornerIndex]);
          referenceDistanceMin   = xiiMath::Min(referenceDistanceMin, cornerDist);
          referenceDistanceMax   = xiiMath::Max(referenceDistanceMax, cornerDist);
        }
      }

      // Break at first error to not spam the log with other potential error (the loop here is very long)
      {
        bool currIterSucceeded = true;
        currIterSucceeded      = currIterSucceeded && XII_TEST_FLOAT(distanceMin, referenceDistanceMin, 0.0001f);
        currIterSucceeded      = currIterSucceeded && XII_TEST_FLOAT(distanceMax, referenceDistanceMax, 0.0001f);
        if (!currIterSucceeded)
        {
          break;
        }
      }
    }
  }
}
