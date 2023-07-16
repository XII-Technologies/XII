#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Quat.h>

XII_CREATE_SIMPLE_TEST(Math, Quaternion)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Default Constructor")
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    if (xiiMath::SupportsNaN<xiiMat3T::ComponentType>())
    {
      // In debug the default constructor initializes everything with NaN.
      xiiQuatT p;
      XII_TEST_BOOL(xiiMath::IsNaN(p.v.x) && xiiMath::IsNaN(p.v.y) && xiiMath::IsNaN(p.v.z) && xiiMath::IsNaN(p.w));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    xiiQuatT::ComponentType testBlock[4] = {
      (xiiQuatT::ComponentType)1, (xiiQuatT::ComponentType)2, (xiiQuatT::ComponentType)3, (xiiQuatT::ComponentType)4};
    xiiQuatT* p = ::new ((void*)&testBlock[0]) xiiQuatT;
    XII_TEST_BOOL(p->v.x == (xiiMat3T::ComponentType)1 && p->v.y == (xiiMat3T::ComponentType)2 && p->v.z == (xiiMat3T::ComponentType)3 &&
                  p->w == (xiiMat3T::ComponentType)4);
#endif
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor(x,y,z,w)")
  {
    xiiQuatT q(1, 2, 3, 4);

    XII_TEST_VEC3(q.v, xiiVec3T(1, 2, 3), 0.0001f);
    XII_TEST_FLOAT(q.w, 4, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IdentityQuaternion")
  {
    xiiQuatT q = xiiQuatT::IdentityQuaternion();

    XII_TEST_VEC3(q.v, xiiVec3T(0, 0, 0), 0.0001f);
    XII_TEST_FLOAT(q.w, 1, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetIdentity")
  {
    xiiQuatT q(1, 2, 3, 4);

    q.SetIdentity();

    XII_TEST_VEC3(q.v, xiiVec3T(0, 0, 0), 0.0001f);
    XII_TEST_FLOAT(q.w, 1, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetElements")
  {
    xiiQuatT q(5, 6, 7, 8);

    q.SetElements(1, 2, 3, 4);

    XII_TEST_VEC3(q.v, xiiVec3T(1, 2, 3), 0.0001f);
    XII_TEST_FLOAT(q.w, 4, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetFromAxisAndAngle / operator* (quat, vec)")
  {
    {
      xiiQuatT q;
      q.SetFromAxisAndAngle(xiiVec3T(1, 0, 0), xiiAngle::Degree(90));

      XII_TEST_VEC3(q * xiiVec3T(0, 1, 0), xiiVec3T(0, 0, 1), 0.0001f);
    }

    {
      xiiQuatT q;
      q.SetFromAxisAndAngle(xiiVec3T(0, 1, 0), xiiAngle::Degree(90));

      XII_TEST_VEC3(q * xiiVec3T(1, 0, 0), xiiVec3T(0, 0, -1), 0.0001f);
    }

    {
      xiiQuatT q;
      q.SetFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::Degree(90));

      XII_TEST_VEC3(q * xiiVec3T(0, 1, 0), xiiVec3T(-1, 0, 0), 0.0001f);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetShortestRotation / IsEqualRotation")
  {
    xiiQuatT q1, q2, q3;
    q1.SetShortestRotation(xiiVec3T(0, 1, 0), xiiVec3T(1, 0, 0));
    q2.SetFromAxisAndAngle(xiiVec3T(0, 0, -1), xiiAngle::Degree(90));
    q3.SetFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::Degree(-90));

    XII_TEST_BOOL(q1.IsEqualRotation(q2, xiiMath::LargeEpsilon<float>()));
    XII_TEST_BOOL(q1.IsEqualRotation(q3, xiiMath::LargeEpsilon<float>()));

    XII_TEST_BOOL(xiiQuatT::IdentityQuaternion().IsEqualRotation(xiiQuatT::IdentityQuaternion(), xiiMath::LargeEpsilon<float>()));
    XII_TEST_BOOL(xiiQuatT::IdentityQuaternion().IsEqualRotation(xiiQuatT(0, 0, 0, -1), xiiMath::LargeEpsilon<float>()));

    xiiQuatT q4{0, 0, 0, 1.00000012f};
    xiiQuatT q5{0, 0, 0, 1.00000023f};
    XII_TEST_BOOL(q4.IsEqualRotation(q5, xiiMath::LargeEpsilon<float>()));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetFromMat3")
  {
    xiiMat3T m;
    m.SetRotationMatrixZ(xiiAngle::Degree(-90));

    xiiQuatT q1, q2, q3;
    q1.SetFromMat3(m);
    q2.SetFromAxisAndAngle(xiiVec3T(0, 0, -1), xiiAngle::Degree(90));
    q3.SetFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::Degree(-90));

    XII_TEST_BOOL(q1.IsEqualRotation(q2, xiiMath::LargeEpsilon<float>()));
    XII_TEST_BOOL(q1.IsEqualRotation(q3, xiiMath::LargeEpsilon<float>()));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetSlerp")
  {
    xiiQuatT q1, q2, q3, qr;
    q1.SetFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::Degree(45));
    q2.SetFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::Degree(0));
    q3.SetFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::Degree(90));

    qr.SetSlerp(q2, q3, 0.5f);

    XII_TEST_BOOL(q1.IsEqualRotation(qr, 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetRotationAxisAndAngle")
  {
    xiiQuatT q1, q2, q3;
    q1.SetShortestRotation(xiiVec3T(0, 1, 0), xiiVec3T(1, 0, 0));
    q2.SetFromAxisAndAngle(xiiVec3T(0, 0, -1), xiiAngle::Degree(90));
    q3.SetFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::Degree(-90));

    xiiVec3T axis;
    xiiAngle angle;

    q1.GetRotationAxisAndAngle(axis, angle);
    XII_TEST_VEC3(axis, xiiVec3T(0, 0, -1), 0.001f);
    XII_TEST_FLOAT(angle.GetDegree(), 90, xiiMath::LargeEpsilon<xiiMat3T::ComponentType>());

    q2.GetRotationAxisAndAngle(axis, angle);
    XII_TEST_VEC3(axis, xiiVec3T(0, 0, -1), 0.001f);
    XII_TEST_FLOAT(angle.GetDegree(), 90, xiiMath::LargeEpsilon<xiiMat3T::ComponentType>());

    q3.GetRotationAxisAndAngle(axis, angle);
    XII_TEST_VEC3(axis, xiiVec3T(0, 0, -1), 0.001f);
    XII_TEST_FLOAT(angle.GetDegree(), 90, xiiMath::LargeEpsilon<xiiMat3T::ComponentType>());

    xiiQuatT::IdentityQuaternion().GetRotationAxisAndAngle(axis, angle);
    XII_TEST_VEC3(axis, xiiVec3T(1, 0, 0), 0.001f);
    XII_TEST_FLOAT(angle.GetDegree(), 0, xiiMath::LargeEpsilon<xiiMat3T::ComponentType>());

    xiiQuatT otherIdentity(0, 0, 0, -1);
    otherIdentity.GetRotationAxisAndAngle(axis, angle);
    XII_TEST_VEC3(axis, xiiVec3T(1, 0, 0), 0.001f);
    XII_TEST_FLOAT(angle.GetDegree(), 360, xiiMath::LargeEpsilon<xiiMat3T::ComponentType>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetAsMat3")
  {
    xiiQuatT q;
    q.SetFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::Degree(90));

    xiiMat3T mr;
    mr.SetRotationMatrixZ(xiiAngle::Degree(90));

    xiiMat3T m = q.GetAsMat3();

    XII_TEST_BOOL(mr.IsEqual(m, xiiMath::DefaultEpsilon<xiiMat3T::ComponentType>()));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetAsMat4")
  {
    xiiQuatT q;
    q.SetFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::Degree(90));

    xiiMat4T mr;
    mr.SetRotationMatrixZ(xiiAngle::Degree(90));

    xiiMat4T m = q.GetAsMat4();

    XII_TEST_BOOL(mr.IsEqual(m, xiiMath::DefaultEpsilon<xiiMat3T::ComponentType>()));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsValid / Normalize")
  {
    xiiQuatT q(1, 2, 3, 4);
    XII_TEST_BOOL(!q.IsValid(0.001f));

    q.Normalize();
    XII_TEST_BOOL(q.IsValid(0.001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator- / Invert")
  {
    xiiQuatT q, q1;
    q.SetFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::Degree(90));
    q1.SetFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::Degree(-90));

    xiiQuatT q2 = -q;
    XII_TEST_BOOL(q1.IsEqualRotation(q2, 0.0001f));

    xiiQuatT q3 = q;
    q3.Invert();
    XII_TEST_BOOL(q1.IsEqualRotation(q3, 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Dot")
  {
    xiiQuatT q, q1, q2;
    q.SetFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::Degree(90));
    q1.SetFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::Degree(-90));
    q2.SetFromAxisAndAngle(xiiVec3T(0, 1, 0), xiiAngle::Degree(45));

    XII_TEST_FLOAT(q.Dot(q), 1.0f, 0.0001f);
    XII_TEST_FLOAT(q.Dot(xiiQuat::IdentityQuaternion()), cos(xiiAngle::DegToRad(90.0f / 2)), 0.0001f);
    XII_TEST_FLOAT(q.Dot(q1), 0.0f, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator*(quat, quat)")
  {
    xiiQuatT q1, q2, qr, q3;
    q1.SetFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::Degree(60));
    q2.SetFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::Degree(30));
    q3.SetFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::Degree(90));

    qr = q1 * q2;

    XII_TEST_BOOL(qr.IsEqualRotation(q3, 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator==/!=")
  {
    xiiQuatT q1, q2;
    q1.SetFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::Degree(60));
    q2.SetFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::Degree(30));
    XII_TEST_BOOL(q1 != q2);

    q2.SetFromAxisAndAngle(xiiVec3T(1, 0, 0), xiiAngle::Degree(60));
    XII_TEST_BOOL(q1 != q2);

    q2.SetFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::Degree(60));
    XII_TEST_BOOL(q1 == q2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsNaN")
  {
    if (xiiMath::SupportsNaN<xiiMathTestType>())
    {
      xiiQuatT q;

      q.SetIdentity();
      XII_TEST_BOOL(!q.IsNaN());

      q.SetIdentity();
      q.w = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(q.IsNaN());

      q.SetIdentity();
      q.v.x = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(q.IsNaN());

      q.SetIdentity();
      q.v.y = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(q.IsNaN());

      q.SetIdentity();
      q.v.z = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(q.IsNaN());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "rotation direction")
  {
    xiiMat3T m;
    m.SetRotationMatrixZ(xiiAngle::Degree(90.0f));

    xiiQuatT q;
    q.SetFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::Degree(90.0f));

    xiiVec3T xAxis(1, 0, 0);

    xiiVec3T temp1 = m.TransformDirection(xAxis);
    xiiVec3T temp2 = q.GetAsMat3().TransformDirection(xAxis);

    XII_TEST_BOOL(temp1.IsEqual(temp2, 0.01f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetAsEulerAngles / SetFromEulerAngles")
  {
    for (xiiUInt32 x = 0; x < 360; x += 15)
    {
      xiiQuat q;
      q.SetFromEulerAngles(xiiAngle::Degree(x), {}, {});

      xiiMat3 m;
      m.SetRotationMatrixX(xiiAngle::Degree(x));
      xiiQuat qm;
      qm.SetFromMat3(m);
      XII_TEST_BOOL(q.IsEqualRotation(qm, 0.01f));

      xiiVec3  axis;
      xiiAngle angle;
      q.GetRotationAxisAndAngle(axis, angle, 0.01f);

      XII_TEST_VEC3(axis, xiiVec3::UnitXAxis(), 0.001f);
      XII_TEST_FLOAT(angle.GetDegree(), (float)x, 0.1f);
    }

    for (xiiUInt32 y = 15; y < 360; y += 15)
    {
      xiiQuat q;
      q.SetFromEulerAngles({}, xiiAngle::Degree(y), {});

      xiiMat3 m;
      m.SetRotationMatrixY(xiiAngle::Degree(y));
      xiiQuat qm;
      qm.SetFromMat3(m);
      XII_TEST_BOOL(q.IsEqualRotation(qm, 0.01f));

      xiiVec3  axis;
      xiiAngle angle;
      q.GetRotationAxisAndAngle(axis, angle, 0.01f);

      XII_TEST_VEC3(axis, xiiVec3::UnitYAxis(), 0.001f);
      XII_TEST_FLOAT(angle.GetDegree(), (float)y, 0.1f);
    }

    for (xiiUInt32 z = 15; z < 360; z += 15)
    {
      xiiQuat q;
      q.SetFromEulerAngles({}, {}, xiiAngle::Degree(z));

      xiiMat3 m;
      m.SetRotationMatrixZ(xiiAngle::Degree(z));
      xiiQuat qm;
      qm.SetFromMat3(m);
      XII_TEST_BOOL(q.IsEqualRotation(qm, 0.01f));

      xiiVec3  axis;
      xiiAngle angle;
      q.GetRotationAxisAndAngle(axis, angle, 0.01f);

      XII_TEST_VEC3(axis, xiiVec3::UnitZAxis(), 0.001f);
      XII_TEST_FLOAT(angle.GetDegree(), (float)z, 0.1f);
    }

    for (xiiUInt32 x = 5; x < 360; x += 20)
    {
      for (xiiUInt32 y = 5; y < 360; y += 20)
      {
        for (xiiUInt32 z = 5; z < 360; z += 30)
        {
          xiiQuat q1;
          q1.SetFromEulerAngles(xiiAngle::Degree(x), xiiAngle::Degree(y), xiiAngle::Degree(z));

          xiiAngle ax, ay, az;
          q1.GetAsEulerAngles(ax, ay, az);

          xiiQuat q2;
          q2.SetFromEulerAngles(ax, ay, az);

          XII_TEST_BOOL(q1.IsEqualRotation(q2, 0.01f));
        }
      }
    }
  }
}
