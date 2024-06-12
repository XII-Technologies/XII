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
      XII_TEST_BOOL(xiiMath::IsNaN(p.x) && xiiMath::IsNaN(p.y) && xiiMath::IsNaN(p.z) && xiiMath::IsNaN(p.w));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    xiiQuatT::ComponentType testBlock[4] = {
      (xiiQuatT::ComponentType)1, (xiiQuatT::ComponentType)2, (xiiQuatT::ComponentType)3, (xiiQuatT::ComponentType)4};
    xiiQuatT* p = ::new ((void*)&testBlock[0]) xiiQuatT;
    XII_TEST_BOOL(p->x == (xiiMat3T::ComponentType)1 && p->y == (xiiMat3T::ComponentType)2 && p->z == (xiiMat3T::ComponentType)3 &&
                  p->w == (xiiMat3T::ComponentType)4);
#endif
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor(x,y,z,w)")
  {
    xiiQuatT q(1, 2, 3, 4);

    XII_TEST_VEC3(q.GetVectorPart(), xiiVec3T(1, 2, 3), 0.0001f);
    XII_TEST_FLOAT(q.w, 4, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeIdentity")
  {
    xiiQuatT q = xiiQuatT::MakeIdentity();

    XII_TEST_VEC3(q.GetVectorPart(), xiiVec3T(0, 0, 0), 0.0001f);
    XII_TEST_FLOAT(q.w, 1, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetIdentity")
  {
    xiiQuatT q(1, 2, 3, 4);

    q.SetIdentity();

    XII_TEST_VEC3(q.GetVectorPart(), xiiVec3T(0, 0, 0), 0.0001f);
    XII_TEST_FLOAT(q.w, 1, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetElements")
  {
    xiiQuatT q(5, 6, 7, 8);

    q = xiiQuat(1, 2, 3, 4);

    XII_TEST_VEC3(q.GetVectorPart(), xiiVec3T(1, 2, 3), 0.0001f);
    XII_TEST_FLOAT(q.w, 4, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetFromAxisAndAngle / operator* (quat, vec)")
  {
    {
      xiiQuatT q;
      q = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(1, 0, 0), xiiAngle::MakeFromDegree(90));

      XII_TEST_VEC3(q * xiiVec3T(0, 1, 0), xiiVec3T(0, 0, 1), 0.0001f);
    }

    {
      xiiQuatT q;
      q = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 1, 0), xiiAngle::MakeFromDegree(90));

      XII_TEST_VEC3(q * xiiVec3T(1, 0, 0), xiiVec3T(0, 0, -1), 0.0001f);
    }

    {
      xiiQuatT q;
      q = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::MakeFromDegree(90));

      XII_TEST_VEC3(q * xiiVec3T(0, 1, 0), xiiVec3T(-1, 0, 0), 0.0001f);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetShortestRotation / IsEqualRotation")
  {
    xiiQuatT q1, q2, q3;
    q1 = xiiQuat::MakeShortestRotation(xiiVec3T(0, 1, 0), xiiVec3T(1, 0, 0));
    q2 = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 0, -1), xiiAngle::MakeFromDegree(90));
    q3 = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::MakeFromDegree(-90));

    XII_TEST_BOOL(q1.IsEqualRotation(q2, xiiMath::LargeEpsilon<float>()));
    XII_TEST_BOOL(q1.IsEqualRotation(q3, xiiMath::LargeEpsilon<float>()));

    XII_TEST_BOOL(xiiQuatT::MakeIdentity().IsEqualRotation(xiiQuatT::MakeIdentity(), xiiMath::LargeEpsilon<float>()));
    XII_TEST_BOOL(xiiQuatT::MakeIdentity().IsEqualRotation(xiiQuatT(0, 0, 0, -1), xiiMath::LargeEpsilon<float>()));

    xiiQuatT q4{0, 0, 0, 1.00000012f};
    xiiQuatT q5{0, 0, 0, 1.00000023f};
    XII_TEST_BOOL(q4.IsEqualRotation(q5, xiiMath::LargeEpsilon<float>()));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetFromMat3")
  {
    xiiMat3T m;
    m = xiiMat3::MakeRotationZ(xiiAngle::MakeFromDegree(-90));

    xiiQuatT q1, q2, q3;
    q1 = xiiQuat::MakeFromMat3(m);
    q2 = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 0, -1), xiiAngle::MakeFromDegree(90));
    q3 = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::MakeFromDegree(-90));

    XII_TEST_BOOL(q1.IsEqualRotation(q2, xiiMath::LargeEpsilon<float>()));
    XII_TEST_BOOL(q1.IsEqualRotation(q3, xiiMath::LargeEpsilon<float>()));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetSlerp")
  {
    xiiQuatT q1, q2, q3, qr;
    q1 = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::MakeFromDegree(45));
    q2 = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::MakeFromDegree(0));
    q3 = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::MakeFromDegree(90));

    qr = xiiQuat::MakeSlerp(q2, q3, 0.5f);

    XII_TEST_BOOL(q1.IsEqualRotation(qr, 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetRotationAxisAndAngle")
  {
    xiiQuatT q1, q2, q3;
    q1 = xiiQuat::MakeShortestRotation(xiiVec3T(0, 1, 0), xiiVec3T(1, 0, 0));
    q2 = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 0, -1), xiiAngle::MakeFromDegree(90));
    q3 = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::MakeFromDegree(-90));

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

    xiiQuatT::MakeIdentity().GetRotationAxisAndAngle(axis, angle);
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
    q = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::MakeFromDegree(90));

    xiiMat3T mr;
    mr = xiiMat3::MakeRotationZ(xiiAngle::MakeFromDegree(90));

    xiiMat3T m = q.GetAsMat3();

    XII_TEST_BOOL(mr.IsEqual(m, xiiMath::DefaultEpsilon<xiiMat3T::ComponentType>()));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetAsMat4")
  {
    xiiQuatT q;
    q = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::MakeFromDegree(90));

    xiiMat4T mr;
    mr = xiiMat4::MakeRotationZ(xiiAngle::MakeFromDegree(90));

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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetInverse / Invert")
  {
    xiiQuatT q, q1;
    q  = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::MakeFromDegree(90));
    q1 = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::MakeFromDegree(-90));

    xiiQuatT q2 = q.GetInverse();
    XII_TEST_BOOL(q1.IsEqualRotation(q2, 0.0001f));

    xiiQuatT q3 = q;
    q3.Invert();
    XII_TEST_BOOL(q1.IsEqualRotation(q3, 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Dot")
  {
    xiiQuatT q, q1, q2;
    q  = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::MakeFromDegree(90));
    q1 = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::MakeFromDegree(-90));
    q2 = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 1, 0), xiiAngle::MakeFromDegree(45));

    XII_TEST_FLOAT(q.Dot(q), 1.0f, 0.0001f);
    XII_TEST_FLOAT(q.Dot(xiiQuat::MakeIdentity()), cos(xiiAngle::DegToRad(90.0f / 2)), 0.0001f);
    XII_TEST_FLOAT(q.Dot(q1), 0.0f, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator*(quat, quat)")
  {
    xiiQuatT q1, q2, qr, q3;
    q1 = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::MakeFromDegree(60));
    q2 = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::MakeFromDegree(30));
    q3 = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::MakeFromDegree(90));

    qr = q1 * q2;

    XII_TEST_BOOL(qr.IsEqualRotation(q3, 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator==/!=")
  {
    xiiQuatT q1, q2;
    q1 = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::MakeFromDegree(60));
    q2 = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::MakeFromDegree(30));
    XII_TEST_BOOL(q1 != q2);

    q2 = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(1, 0, 0), xiiAngle::MakeFromDegree(60));
    XII_TEST_BOOL(q1 != q2);

    q2 = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::MakeFromDegree(60));
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
      q.x = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(q.IsNaN());

      q.SetIdentity();
      q.y = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(q.IsNaN());

      q.SetIdentity();
      q.z = xiiMath::NaN<xiiMathTestType>();
      XII_TEST_BOOL(q.IsNaN());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "rotation direction")
  {
    xiiMat3T m;
    m = xiiMat3::MakeRotationZ(xiiAngle::MakeFromDegree(90.0f));

    xiiQuatT q;
    q = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::MakeFromDegree(90.0f));

    xiiVec3T xAxis(1, 0, 0);

    xiiVec3T temp1 = m.TransformDirection(xAxis);
    xiiVec3T temp2 = q.GetAsMat3().TransformDirection(xAxis);

    XII_TEST_BOOL(temp1.IsEqual(temp2, 0.01f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetAsEulerAngles / SetFromEulerAngles")
  {
    xiiAngle ax, ay, az;

    for (xiiUInt32 x = 0; x < 360; x += 15)
    {
      xiiQuat q = xiiQuat::MakeFromEulerAngles(xiiAngle::MakeFromDegree(x), {}, {});

      xiiMat3 m;
      m = xiiMat3::MakeRotationX(xiiAngle::MakeFromDegree(x));
      xiiQuat qm;
      qm = xiiQuat::MakeFromMat3(m);
      XII_TEST_BOOL(q.IsEqualRotation(qm, 0.01f));

      xiiVec3  axis;
      xiiAngle angle;
      q.GetRotationAxisAndAngle(axis, angle, 0.01f);

      XII_TEST_VEC3(axis, xiiVec3::MakeAxisX(), 0.001f);
      XII_TEST_FLOAT(angle.GetDegree(), (float)x, 0.1f);

      q.GetAsEulerAngles(ax, ay, az);
      XII_TEST_BOOL(ax.IsEqualNormalized(xiiAngle::MakeFromDegree(x), xiiAngle::MakeFromDegree(0.1f)));
    }

    for (xiiInt32 y = -90; y < 360; y += 15)
    {
      xiiQuat q = xiiQuat::MakeFromEulerAngles({}, xiiAngle::MakeFromDegree(y), {});

      xiiMat3 m;
      m = xiiMat3::MakeRotationY(xiiAngle::MakeFromDegree(y));
      xiiQuat qm;
      qm = xiiQuat::MakeFromMat3(m);
      XII_TEST_BOOL(q.IsEqualRotation(qm, 0.01f));

      xiiVec3  axis;
      xiiAngle angle;
      q.GetRotationAxisAndAngle(axis, angle, 0.01f);

      if (y < 0)
      {
        XII_TEST_VEC3(axis, -xiiVec3::MakeAxisY(), 0.001f);
        XII_TEST_FLOAT(angle.GetDegree(), (float)-y, 0.1f);
      }
      else if (y > 0)
      {
        XII_TEST_VEC3(axis, xiiVec3::MakeAxisY(), 0.001f);
        XII_TEST_FLOAT(angle.GetDegree(), (float)y, 0.1f);
      }

      // pitch is only defined in -90..90 range
      if (y >= -90 && y <= 90)
      {
        q.GetAsEulerAngles(ax, ay, az);
        XII_TEST_FLOAT(ay.GetDegree(), (float)y, 0.1f);
      }
    }

    for (xiiUInt32 z = 15; z < 360; z += 15)
    {
      xiiQuat q = xiiQuat::MakeFromEulerAngles({}, {}, xiiAngle::MakeFromDegree(z));

      xiiMat3 m;
      m = xiiMat3::MakeRotationZ(xiiAngle::MakeFromDegree(z));
      xiiQuat qm;
      qm = xiiQuat::MakeFromMat3(m);
      XII_TEST_BOOL(q.IsEqualRotation(qm, 0.01f));

      xiiVec3  axis;
      xiiAngle angle;
      q.GetRotationAxisAndAngle(axis, angle, 0.01f);

      XII_TEST_VEC3(axis, xiiVec3::MakeAxisZ(), 0.001f);
      XII_TEST_FLOAT(angle.GetDegree(), (float)z, 0.1f);

      q.GetAsEulerAngles(ax, ay, az);
      XII_TEST_BOOL(az.IsEqualNormalized(xiiAngle::MakeFromDegree(z), xiiAngle::MakeFromDegree(0.1f)));
    }

    for (xiiUInt32 x = 0; x < 360; x += 15)
    {
      for (xiiUInt32 y = 0; y < 360; y += 15)
      {
        for (xiiUInt32 z = 0; z < 360; z += 30)
        {
          xiiQuat q1 = xiiQuat::MakeFromEulerAngles(xiiAngle::MakeFromDegree(x), xiiAngle::MakeFromDegree(y), xiiAngle::MakeFromDegree(z));

          q1.GetAsEulerAngles(ax, ay, az);

          xiiQuat q2 = xiiQuat::MakeFromEulerAngles(ax, ay, az);

          XII_TEST_BOOL(q1.IsEqualRotation(q2, 0.1f));

          // Check that euler order is ZYX aka 3-2-1
          xiiQuat q3;
          {
            xiiQuat xRot, yRot, zRot;
            xRot = xiiQuat::MakeFromAxisAndAngle(xiiVec3::MakeAxisX(), xiiAngle::MakeFromDegree(x));
            yRot = xiiQuat::MakeFromAxisAndAngle(xiiVec3::MakeAxisY(), xiiAngle::MakeFromDegree(y));
            zRot = xiiQuat::MakeFromAxisAndAngle(xiiVec3::MakeAxisZ(), xiiAngle::MakeFromDegree(z));

            q3 = zRot * yRot * xRot;
          }
          XII_TEST_BOOL(q1.IsEqualRotation(q3, 0.01f));
        }
      }
    }
  }
}
