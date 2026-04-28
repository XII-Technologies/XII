/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <CoreTest/CoreTestPCH.h>

#include <Core/World/CoordinateSystem.h>

void TestLength(const xiiCoordinateSystemConversion& atoB, const xiiCoordinateSystemConversion& btoA, float fSourceLength, float fTargetLength)
{
  XII_TEST_FLOAT(atoB.ConvertSourceLength(fSourceLength), fTargetLength, xiiMath::DefaultEpsilon<float>());
  XII_TEST_FLOAT(atoB.ConvertTargetLength(fTargetLength), fSourceLength, xiiMath::DefaultEpsilon<float>());

  XII_TEST_FLOAT(btoA.ConvertTargetLength(fSourceLength), fTargetLength, xiiMath::DefaultEpsilon<float>());
  XII_TEST_FLOAT(btoA.ConvertSourceLength(fTargetLength), fSourceLength, xiiMath::DefaultEpsilon<float>());
}

void TestPosition(const xiiCoordinateSystemConversion& atoB, const xiiCoordinateSystemConversion& btoA, const xiiVec3& vSourcePos, const xiiVec3& vTargetPos)
{
  TestLength(atoB, btoA, vSourcePos.GetLength(), vTargetPos.GetLength());

  XII_TEST_VEC3(atoB.ConvertSourcePosition(vSourcePos), vTargetPos, xiiMath::DefaultEpsilon<float>());
  XII_TEST_VEC3(atoB.ConvertTargetPosition(vTargetPos), vSourcePos, xiiMath::DefaultEpsilon<float>());
  XII_TEST_VEC3(btoA.ConvertSourcePosition(vTargetPos), vSourcePos, xiiMath::DefaultEpsilon<float>());
  XII_TEST_VEC3(btoA.ConvertTargetPosition(vSourcePos), vTargetPos, xiiMath::DefaultEpsilon<float>());
}

void TestRotation(const xiiCoordinateSystemConversion& atoB, const xiiCoordinateSystemConversion& btoA, const xiiVec3& vSourceStartDir, const xiiVec3& vSourceEndDir, const xiiQuat& qSourceRot, const xiiVec3& vTargetStartDir, const xiiVec3& vTargetEndDir, const xiiQuat& qTargetRot)
{
  TestPosition(atoB, btoA, vSourceStartDir, vTargetStartDir);
  TestPosition(atoB, btoA, vSourceEndDir, vTargetEndDir);

  XII_TEST_BOOL(atoB.ConvertSourceRotation(qSourceRot).IsEqualRotation(qTargetRot, xiiMath::DefaultEpsilon<float>()));
  XII_TEST_BOOL(atoB.ConvertTargetRotation(qTargetRot).IsEqualRotation(qSourceRot, xiiMath::DefaultEpsilon<float>()));
  XII_TEST_BOOL(btoA.ConvertSourceRotation(qTargetRot).IsEqualRotation(qSourceRot, xiiMath::DefaultEpsilon<float>()));
  XII_TEST_BOOL(btoA.ConvertTargetRotation(qSourceRot).IsEqualRotation(qTargetRot, xiiMath::DefaultEpsilon<float>()));

  XII_TEST_VEC3(qSourceRot * vSourceStartDir, vSourceEndDir, xiiMath::DefaultEpsilon<float>());
  XII_TEST_VEC3(qTargetRot * vTargetStartDir, vTargetEndDir, xiiMath::DefaultEpsilon<float>());
}

xiiQuat FromAxisAndAngle(const xiiVec3& vAxis, xiiAngle angle)
{
  xiiQuat q;
  q = xiiQuat::MakeFromAxisAndAngle(vAxis.GetNormalized(), angle);
  return q;
}

bool IsRightHanded(const xiiCoordinateSystem& cs)
{
  xiiVec3 vF = cs.m_vUpDir.CrossRH(cs.m_vRightDir);

  return vF.Dot(cs.m_vForwardDir) > 0;
}

void TestCoordinateSystemConversion(const xiiCoordinateSystem& a, const xiiCoordinateSystem& b)
{
  const bool     bAisRH  = IsRightHanded(a);
  const bool     bBisRH  = IsRightHanded(b);
  const xiiAngle A_CWRot = bAisRH ? xiiAngle::MakeFromDegree(-90.0f) : xiiAngle::MakeFromDegree(90.0f);
  const xiiAngle B_CWRot = bBisRH ? xiiAngle::MakeFromDegree(-90.0f) : xiiAngle::MakeFromDegree(90.0f);

  xiiCoordinateSystemConversion AtoB;
  AtoB.SetConversion(a, b);

  xiiCoordinateSystemConversion BtoA;
  BtoA.SetConversion(b, a);

  TestPosition(AtoB, BtoA, a.m_vForwardDir, b.m_vForwardDir);
  TestPosition(AtoB, BtoA, a.m_vRightDir, b.m_vRightDir);
  TestPosition(AtoB, BtoA, a.m_vUpDir, b.m_vUpDir);

  TestRotation(AtoB, BtoA, a.m_vForwardDir, a.m_vRightDir, FromAxisAndAngle(a.m_vUpDir, A_CWRot), b.m_vForwardDir, b.m_vRightDir, FromAxisAndAngle(b.m_vUpDir, B_CWRot));
  TestRotation(AtoB, BtoA, a.m_vUpDir, a.m_vForwardDir, FromAxisAndAngle(a.m_vRightDir, A_CWRot), b.m_vUpDir, b.m_vForwardDir, FromAxisAndAngle(b.m_vRightDir, B_CWRot));
  TestRotation(AtoB, BtoA, a.m_vUpDir, a.m_vRightDir, FromAxisAndAngle(a.m_vForwardDir, -A_CWRot), b.m_vUpDir, b.m_vRightDir, FromAxisAndAngle(b.m_vForwardDir, -B_CWRot));
}


XII_CREATE_SIMPLE_TEST(World, CoordinateSystem)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "XII / OpenXR")
  {
    xiiCoordinateSystem xiiCoordSysLH;
    xiiCoordSysLH.m_vForwardDir = xiiVec3(1.0f, 0.0f, 0.0f);
    xiiCoordSysLH.m_vRightDir   = xiiVec3(0.0f, 1.0f, 0.0f);
    xiiCoordSysLH.m_vUpDir      = xiiVec3(0.0f, 0.0f, 1.0f);

    xiiCoordinateSystem openXrRH;
    openXrRH.m_vForwardDir = xiiVec3(0.0f, 0.0f, -1.0f);
    openXrRH.m_vRightDir   = xiiVec3(1.0f, 0.0f, 0.0f);
    openXrRH.m_vUpDir      = xiiVec3(0.0f, 1.0f, 0.0f);

    TestCoordinateSystemConversion(xiiCoordSysLH, openXrRH);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Scaled XII / Scaled OpenXR")
  {
    xiiCoordinateSystem xiiCoordSysLH;
    xiiCoordSysLH.m_vForwardDir = xiiVec3(0.1f, 0.0f, 0.0f);
    xiiCoordSysLH.m_vRightDir   = xiiVec3(0.0f, 0.1f, 0.0f);
    xiiCoordSysLH.m_vUpDir      = xiiVec3(0.0f, 0.0f, 0.1f);

    xiiCoordinateSystem openXrRH;
    openXrRH.m_vForwardDir = xiiVec3(0.0f, 0.0f, -20.0f);
    openXrRH.m_vRightDir   = xiiVec3(20.0f, 0.0f, 0.0f);
    openXrRH.m_vUpDir      = xiiVec3(0.0f, 20.0f, 0.0f);

    TestCoordinateSystemConversion(xiiCoordSysLH, openXrRH);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "XII / Flipped XII")
  {
    xiiCoordinateSystem xiiCoordSysLH;
    xiiCoordSysLH.m_vForwardDir = xiiVec3(1.0f, 0.0f, 0.0f);
    xiiCoordSysLH.m_vRightDir   = xiiVec3(0.0f, 1.0f, 0.0f);
    xiiCoordSysLH.m_vUpDir      = xiiVec3(0.0f, 0.0f, 1.0f);

    xiiCoordinateSystem xiiCoordSysFlippedLH;
    xiiCoordSysFlippedLH.m_vForwardDir = xiiVec3(-1.0f, 0.0f, 0.0f);
    xiiCoordSysFlippedLH.m_vRightDir   = xiiVec3(0.0f, -1.0f, 0.0f);
    xiiCoordSysFlippedLH.m_vUpDir      = xiiVec3(0.0f, 0.0f, 1.0f);

    TestCoordinateSystemConversion(xiiCoordSysLH, xiiCoordSysFlippedLH);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "OpenXR / Flipped OpenXR")
  {
    xiiCoordinateSystem openXrRH;
    openXrRH.m_vForwardDir = xiiVec3(0.0f, 0.0f, -1.0f);
    openXrRH.m_vRightDir   = xiiVec3(1.0f, 0.0f, 0.0f);
    openXrRH.m_vUpDir      = xiiVec3(0.0f, 1.0f, 0.0f);

    xiiCoordinateSystem openXrFlippedRH;
    openXrFlippedRH.m_vForwardDir = xiiVec3(0.0f, 0.0f, 1.0f);
    openXrFlippedRH.m_vRightDir   = xiiVec3(-1.0f, 0.0f, 0.0f);
    openXrFlippedRH.m_vUpDir      = xiiVec3(0.0f, 1.0f, 0.0f);

    TestCoordinateSystemConversion(openXrRH, openXrFlippedRH);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Identity")
  {
    xiiCoordinateSystem xiiCoordSysLH;
    xiiCoordSysLH.m_vForwardDir = xiiVec3(1.0f, 0.0f, 0.0f);
    xiiCoordSysLH.m_vRightDir   = xiiVec3(0.0f, 1.0f, 0.0f);
    xiiCoordSysLH.m_vUpDir      = xiiVec3(0.0f, 0.0f, 1.0f);

    TestCoordinateSystemConversion(xiiCoordSysLH, xiiCoordSysLH);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Default Constructed")
  {
    xiiCoordinateSystem xiiCoordSysLH;
    xiiCoordSysLH.m_vForwardDir = xiiVec3(1.0f, 0.0f, 0.0f);
    xiiCoordSysLH.m_vRightDir   = xiiVec3(0.0f, 1.0f, 0.0f);
    xiiCoordSysLH.m_vUpDir      = xiiVec3(0.0f, 0.0f, 1.0f);

    const xiiAngle rot = xiiAngle::MakeFromDegree(90.0f);

    xiiCoordinateSystemConversion defaultConstucted;

    TestPosition(defaultConstucted, defaultConstucted, xiiCoordSysLH.m_vForwardDir, xiiCoordSysLH.m_vForwardDir);
    TestPosition(defaultConstucted, defaultConstucted, xiiCoordSysLH.m_vRightDir, xiiCoordSysLH.m_vRightDir);
    TestPosition(defaultConstucted, defaultConstucted, xiiCoordSysLH.m_vUpDir, xiiCoordSysLH.m_vUpDir);

    TestRotation(defaultConstucted, defaultConstucted, xiiCoordSysLH.m_vForwardDir, xiiCoordSysLH.m_vRightDir, FromAxisAndAngle(xiiCoordSysLH.m_vUpDir, rot), xiiCoordSysLH.m_vForwardDir, xiiCoordSysLH.m_vRightDir, FromAxisAndAngle(xiiCoordSysLH.m_vUpDir, rot));
    TestRotation(defaultConstucted, defaultConstucted, xiiCoordSysLH.m_vUpDir, xiiCoordSysLH.m_vForwardDir, FromAxisAndAngle(xiiCoordSysLH.m_vRightDir, rot), xiiCoordSysLH.m_vUpDir, xiiCoordSysLH.m_vForwardDir, FromAxisAndAngle(xiiCoordSysLH.m_vRightDir, rot));
    TestRotation(defaultConstucted, defaultConstucted, xiiCoordSysLH.m_vUpDir, xiiCoordSysLH.m_vRightDir, FromAxisAndAngle(xiiCoordSysLH.m_vForwardDir, -rot), xiiCoordSysLH.m_vUpDir, xiiCoordSysLH.m_vRightDir, FromAxisAndAngle(xiiCoordSysLH.m_vForwardDir, -rot));
  }
}
