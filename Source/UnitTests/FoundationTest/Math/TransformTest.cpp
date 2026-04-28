/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Transform.h>

XII_CREATE_SIMPLE_TEST(Math, Transform)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructors")
  {
    xiiTransformT t0;

    {
      xiiTransformT t(xiiVec3T(1, 2, 3));
      XII_TEST_VEC3(t.m_vPosition, xiiVec3T(1, 2, 3), 0);
    }

    {
      xiiQuat qRot;
      qRot = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(1, 2, 3).GetNormalized(), xiiAngle::MakeFromDegree(42.0f));

      xiiTransformT t(xiiVec3T(4, 5, 6), qRot);
      XII_TEST_VEC3(t.m_vPosition, xiiVec3T(4, 5, 6), 0);
      XII_TEST_BOOL(t.m_qRotation == qRot);
    }

    {
      xiiMat3 mRot = xiiMat3::MakeAxisRotation(xiiVec3T(1, 2, 3).GetNormalized(), xiiAngle::MakeFromDegree(42.0f));

      xiiQuat q;
      q = xiiQuat::MakeFromMat3(mRot);

      xiiTransformT t(xiiVec3T(4, 5, 6), q);
      XII_TEST_VEC3(t.m_vPosition, xiiVec3T(4, 5, 6), 0);
      XII_TEST_BOOL(t.m_qRotation.GetAsMat3().IsEqual(mRot, 0.0001f));
    }

    {
      xiiQuat qRot;
      qRot.SetIdentity();

      xiiTransformT t(xiiVec3T(4, 5, 6), qRot, xiiVec3T(2, 3, 4));
      XII_TEST_VEC3(t.m_vPosition, xiiVec3T(4, 5, 6), 0);
      XII_TEST_BOOL(t.m_qRotation.GetAsMat3().IsEqual(xiiMat3::MakeFromValues(1, 0, 0, 0, 1, 0, 0, 0, 1), 0.001f));
      XII_TEST_VEC3(t.m_vScale, xiiVec3T(2, 3, 4), 0);
    }

    {
      xiiMat3T mRot = xiiMat3::MakeAxisRotation(xiiVec3T(1, 2, 3).GetNormalized(), xiiAngle::MakeFromDegree(42.0f));
      xiiMat4T mTrans;
      mTrans.SetTransformationMatrix(mRot, xiiVec3T(1, 2, 3));

      xiiTransformT t = xiiTransform::MakeFromMat4(mTrans);
      XII_TEST_VEC3(t.m_vPosition, xiiVec3T(1, 2, 3), 0);
      XII_TEST_BOOL(t.m_qRotation.GetAsMat3().IsEqual(mRot, 0.001f));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetIdentity")
  {
    xiiTransformT t;
    t.SetIdentity();

    XII_TEST_VEC3(t.m_vPosition, xiiVec3T(0), 0);
    XII_TEST_BOOL(t.m_qRotation == xiiQuat::MakeIdentity());
    XII_TEST_BOOL(t.m_vScale == xiiVec3T(1.0f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetAsMat4")
  {
    xiiQuat qRot;
    qRot.SetIdentity();

    xiiTransformT t(xiiVec3T(4, 5, 6), qRot, xiiVec3T(2, 3, 4));
    XII_TEST_BOOL(t.GetAsMat4() == xiiMat4T::MakeFromValues(2, 0, 0, 4, 0, 3, 0, 5, 0, 0, 4, 6, 0, 0, 0, 1));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator + / -")
  {
    xiiTransformT t0, t1;
    t0.SetIdentity();
    t1.SetIdentity();

    t1 = t0 + xiiVec3T(2, 3, 4);
    XII_TEST_VEC3(t1.m_vPosition, xiiVec3T(2, 3, 4), 0.0001f);

    t1 = t1 - xiiVec3T(4, 2, 1);
    XII_TEST_VEC3(t1.m_vPosition, xiiVec3T(-2, 1, 3), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator * (quat)")
  {
    xiiQuat qRotX, qRotY;
    qRotX = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(1, 0, 0), xiiAngle::MakeFromRadian(1.57079637f));
    qRotY = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 1, 0), xiiAngle::MakeFromRadian(1.57079637f));

    xiiTransformT t0, t1;
    t0.SetIdentity();
    t1.SetIdentity();

    t1 = qRotX * t0;
    XII_TEST_VEC3(t1.m_vPosition, xiiVec3T(0, 0, 0), 0.0001f);

    xiiQuat q;
    q = xiiQuat::MakeFromMat3(xiiMat3::MakeFromValues(1, 0, 0, 0, 0, -1, 0, 1, 0));
    XII_TEST_BOOL(t1.m_qRotation.IsEqualRotation(q, 0.0001f));

    t1 = qRotY * t1;
    XII_TEST_VEC3(t1.m_vPosition, xiiVec3T(0, 0, 0), 0.0001f);
    q = xiiQuat::MakeFromMat3(xiiMat3::MakeFromValues(0, 1, 0, 0, 0, -1, -1, 0, 0));
    XII_TEST_BOOL(t1.m_qRotation.IsEqualRotation(q, 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator * (vec3)")
  {
    xiiQuat qRotX, qRotY;
    qRotX = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(1, 0, 0), xiiAngle::MakeFromRadian(1.57079637f));
    qRotY = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 1, 0), xiiAngle::MakeFromRadian(1.57079637f));

    xiiTransformT t;
    t.SetIdentity();

    t = qRotX * t;

    XII_TEST_BOOL(t.m_qRotation.IsEqualRotation(qRotX, 0.0001f));
    XII_TEST_VEC3(t.m_vPosition, xiiVec3T(0, 0, 0), 0.0001f);
    XII_TEST_VEC3(t.m_vScale, xiiVec3T(1, 1, 1), 0.0001f);

    t = t + xiiVec3T(1, 2, 3);

    XII_TEST_BOOL(t.m_qRotation.IsEqualRotation(qRotX, 0.0001f));
    XII_TEST_VEC3(t.m_vPosition, xiiVec3T(1, 2, 3), 0.0001f);
    XII_TEST_VEC3(t.m_vScale, xiiVec3T(1, 1, 1), 0.0001f);

    t = qRotY * t;

    XII_TEST_BOOL(t.m_qRotation.IsEqualRotation(qRotY * qRotX, 0.0001f));
    XII_TEST_VEC3(t.m_vPosition, xiiVec3T(1, 2, 3), 0.0001f);
    XII_TEST_VEC3(t.m_vScale, xiiVec3T(1, 1, 1), 0.0001f);

    xiiQuat q;
    q = xiiQuat::MakeFromMat3(xiiMat3::MakeFromValues(0, 1, 0, 0, 0, -1, -1, 0, 0));
    XII_TEST_BOOL(t.m_qRotation.IsEqualRotation(q, 0.0001f));

    xiiVec3T v;
    v = t * xiiVec3T(4, 5, 6);

    XII_TEST_VEC3(v, xiiVec3T(5 + 1, -6 + 2, -4 + 3), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsIdentical")
  {
    xiiTransformT t(xiiVec3T(1, 2, 3));
    t.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 1, 0), xiiAngle::MakeFromDegree(90));

    XII_TEST_BOOL(t.IsIdentical(t));

    xiiTransformT t2(xiiVec3T(1, 2, 4));
    t2.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 1, 0), xiiAngle::MakeFromDegree(90));

    XII_TEST_BOOL(!t.IsIdentical(t2));

    xiiTransformT t3(xiiVec3T(1, 2, 3));
    t3.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 1, 0), xiiAngle::MakeFromDegree(91));

    XII_TEST_BOOL(!t.IsIdentical(t3));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator == / !=")
  {
    xiiTransformT t(xiiVec3T(1, 2, 3));
    t.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 1, 0), xiiAngle::MakeFromDegree(90));

    XII_TEST_BOOL(t == t);

    xiiTransformT t2(xiiVec3T(1, 2, 4));
    t2.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 1, 0), xiiAngle::MakeFromDegree(90));

    XII_TEST_BOOL(t != t2);

    xiiTransformT t3(xiiVec3T(1, 2, 3));
    t3.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 1, 0), xiiAngle::MakeFromDegree(91));

    XII_TEST_BOOL(t != t3);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsEqual")
  {
    xiiTransformT t(xiiVec3T(1, 2, 3));
    t.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 1, 0), xiiAngle::MakeFromDegree(90));

    XII_TEST_BOOL(t.IsEqual(t, 0.0001f));

    xiiTransformT t2(xiiVec3T(1, 2, 3.0002f));
    t2.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 1, 0), xiiAngle::MakeFromDegree(90));

    XII_TEST_BOOL(t.IsEqual(t2, 0.001f));
    XII_TEST_BOOL(!t.IsEqual(t2, 0.0001f));

    xiiTransformT t3(xiiVec3T(1, 2, 3));
    t3.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 1, 0), xiiAngle::MakeFromDegree(90.01f));

    XII_TEST_BOOL(t.IsEqual(t3, 0.01f));
    XII_TEST_BOOL(!t.IsEqual(t3, 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator*(xiiTransformT, xiiTransformT)")
  {
    xiiTransformT tParent(xiiVec3T(1, 2, 3));
    tParent.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 1, 0), xiiAngle::MakeFromRadian(1.57079637f));
    tParent.m_vScale.Set(2);

    xiiTransformT tToChild(xiiVec3T(4, 5, 6));
    tToChild.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::MakeFromRadian(1.57079637f));
    tToChild.m_vScale.Set(4);

    // this is exactly the same as SetGlobalTransform
    xiiTransformT tChild;
    tChild = tParent * tToChild;

    XII_TEST_VEC3(tChild.m_vPosition, xiiVec3T(13, 12, -5), 0.003f);
    XII_TEST_BOOL(tChild.m_qRotation.GetAsMat3().IsEqual(xiiMat3::MakeFromValues(0, 0, 1, 1, 0, 0, 0, 1, 0), 0.0001f));
    XII_TEST_VEC3(tChild.m_vScale, xiiVec3T(8, 8, 8), 0.0001f);

    // verify that it works exactly like a 4x4 matrix
    const xiiMat4 mParent  = tParent.GetAsMat4();
    const xiiMat4 mToChild = tToChild.GetAsMat4();
    const xiiMat4 mChild   = mParent * mToChild;

    XII_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator*(xiiTransformT, xiiMat4)")
  {
    xiiTransformT tParent(xiiVec3T(1, 2, 3));
    tParent.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 1, 0), xiiAngle::MakeFromDegree(90));
    tParent.m_vScale.Set(2);

    xiiTransformT tToChild(xiiVec3T(4, 5, 6));
    tToChild.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::MakeFromDegree(90));
    tToChild.m_vScale.Set(4);

    // this is exactly the same as SetGlobalTransform
    xiiTransformT tChild;
    tChild = tParent * tToChild;

    XII_TEST_VEC3(tChild.m_vPosition, xiiVec3T(13, 12, -5), 0.0001f);
    XII_TEST_BOOL(tChild.m_qRotation.GetAsMat3().IsEqual(xiiMat3::MakeFromValues(0, 0, 1, 1, 0, 0, 0, 1, 0), 0.0001f));
    XII_TEST_VEC3(tChild.m_vScale, xiiVec3T(8, 8, 8), 0.0001f);

    // verify that it works exactly like a 4x4 matrix
    const xiiMat4 mParent  = tParent.GetAsMat4();
    const xiiMat4 mToChild = tToChild.GetAsMat4();
    const xiiMat4 mChild   = mParent * mToChild;

    XII_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator*(xiiMat4, xiiTransformT)")
  {
    xiiTransformT tParent(xiiVec3T(1, 2, 3));
    tParent.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 1, 0), xiiAngle::MakeFromDegree(90));
    tParent.m_vScale.Set(2);

    xiiTransformT tToChild(xiiVec3T(4, 5, 6));
    tToChild.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::MakeFromDegree(90));
    tToChild.m_vScale.Set(4);

    // this is exactly the same as SetGlobalTransform
    xiiTransformT tChild;
    tChild = tParent * tToChild;

    XII_TEST_VEC3(tChild.m_vPosition, xiiVec3T(13, 12, -5), 0.0001f);
    XII_TEST_BOOL(tChild.m_qRotation.GetAsMat3().IsEqual(xiiMat3::MakeFromValues(0, 0, 1, 1, 0, 0, 0, 1, 0), 0.0001f));
    XII_TEST_VEC3(tChild.m_vScale, xiiVec3T(8, 8, 8), 0.0001f);

    // verify that it works exactly like a 4x4 matrix
    const xiiMat4 mParent  = tParent.GetAsMat4();
    const xiiMat4 mToChild = tToChild.GetAsMat4();
    const xiiMat4 mChild   = mParent * mToChild;

    XII_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Invert / GetInverse")
  {
    xiiTransformT tParent(xiiVec3T(1, 2, 3));
    tParent.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 1, 0), xiiAngle::MakeFromDegree(90));
    tParent.m_vScale.Set(2);

    xiiTransformT tToChild(xiiVec3T(4, 5, 6));
    tToChild.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3T(0, 0, 1), xiiAngle::MakeFromDegree(90));
    tToChild.m_vScale.Set(4);

    xiiTransformT tChild;
    tChild = xiiTransform::MakeGlobalTransform(tParent, tToChild);

    // negate twice -> get back original
    tToChild.Invert();
    tToChild.Invert();

    xiiTransformT tInvToChild = tToChild.GetInverse();

    xiiTransformT tParentFromChild;
    tParentFromChild = xiiTransform::MakeGlobalTransform(tChild, tInvToChild);

    XII_TEST_BOOL(tParent.IsEqual(tParentFromChild, 0.0001f));
  }

  //////////////////////////////////////////////////////////////////////////
  // Tests copied and ported over from xiiSimdTransform
  //////////////////////////////////////////////////////////////////////////

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
    xiiTransform t0;

    {
      xiiQuat qRot;
      qRot = xiiQuat::MakeFromAxisAndAngle(xiiVec3(1, 2, 3).GetNormalized(), xiiAngle::MakeFromDegree(42.0f));

      xiiVec3 pos(4, 5, 6);
      xiiVec3 scale(7, 8, 9);

      xiiTransform t(pos);
      XII_TEST_BOOL((t.m_vPosition == pos));
      XII_TEST_BOOL(t.m_qRotation == xiiQuat::MakeIdentity());
      XII_TEST_BOOL((t.m_vScale == xiiVec3(1)));

      t = xiiTransform(pos, qRot);
      XII_TEST_BOOL((t.m_vPosition == pos));
      XII_TEST_BOOL(t.m_qRotation == qRot);
      XII_TEST_BOOL((t.m_vScale == xiiVec3(1)));

      t = xiiTransform(pos, qRot, scale);
      XII_TEST_BOOL((t.m_vPosition == pos));
      XII_TEST_BOOL(t.m_qRotation == qRot);
      XII_TEST_BOOL((t.m_vScale == scale));

      t = xiiTransform(xiiVec3::MakeZero(), qRot);
      XII_TEST_BOOL(t.m_vPosition.IsZero());
      XII_TEST_BOOL(t.m_qRotation == qRot);
      XII_TEST_BOOL((t.m_vScale == xiiVec3(1)));
    }

    {
      xiiTransform t;
      t.SetIdentity();

      XII_TEST_BOOL(t.m_vPosition.IsZero());
      XII_TEST_BOOL(t.m_qRotation == xiiQuat::MakeIdentity());
      XII_TEST_BOOL((t.m_vScale == xiiVec3(1)));

      XII_TEST_BOOL(t == xiiTransform::MakeIdentity());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Inverse")
  {
    xiiTransform tParent(xiiVec3(1, 2, 3));
    tParent.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::MakeFromDegree(90));
    tParent.m_vScale    = xiiVec3(2);

    xiiTransform tToChild(xiiVec3(4, 5, 6));
    tToChild.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 0, 1), xiiAngle::MakeFromDegree(90));
    tToChild.m_vScale    = xiiVec3(4);

    xiiTransform tChild;
    tChild = tParent * tToChild;

    // invert twice -> get back original
    xiiTransform t2 = tToChild;
    t2.Invert();
    t2.Invert();
    XII_TEST_BOOL(t2.IsEqual(tToChild, 0.0001f));

    xiiTransform tInvToChild = tToChild.GetInverse();

    xiiTransform tParentFromChild;
    tParentFromChild = tChild * tInvToChild;

    XII_TEST_BOOL(tParent.IsEqual(tParentFromChild, 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetLocalTransform")
  {
    xiiQuat q;
    q = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 0, 1), xiiAngle::MakeFromDegree(90));

    xiiTransform tParent(xiiVec3(1, 2, 3));
    tParent.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::MakeFromDegree(90));
    tParent.m_vScale    = xiiVec3(2);

    xiiTransform tChild;
    tChild.m_vPosition = xiiVec3(13, 12, -5);
    tChild.m_qRotation = tParent.m_qRotation * q;
    tChild.m_vScale    = xiiVec3(8);

    xiiTransform tToChild;
    tToChild = xiiTransform::MakeLocalTransform(tParent, tChild);

    XII_TEST_BOOL(tToChild.m_vPosition.IsEqual(xiiVec3(4, 5, 6), 0.0001f));
    XII_TEST_BOOL(tToChild.m_qRotation.IsEqualRotation(q, 0.0001f));
    XII_TEST_BOOL((tToChild.m_vScale == xiiVec3(4)));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetGlobalTransform")
  {
    xiiTransform tParent(xiiVec3(1, 2, 3));
    tParent.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::MakeFromDegree(90));
    tParent.m_vScale    = xiiVec3(2);

    xiiTransform tToChild(xiiVec3(4, 5, 6));
    tToChild.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 0, 1), xiiAngle::MakeFromDegree(90));
    tToChild.m_vScale    = xiiVec3(4);

    xiiTransform tChild;
    tChild = xiiTransform::MakeGlobalTransform(tParent, tToChild);

    XII_TEST_BOOL(tChild.m_vPosition.IsEqual(xiiVec3(13, 12, -5), 0.0001f));
    XII_TEST_BOOL(tChild.m_qRotation.IsEqualRotation(tParent.m_qRotation * tToChild.m_qRotation, 0.0001f));
    XII_TEST_BOOL((tChild.m_vScale == xiiVec3(8)));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetAsMat4")
  {
    xiiTransform t(xiiVec3(1, 2, 3));
    t.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::MakeFromDegree(34));
    t.m_vScale    = xiiVec3(2, -1, 5);

    xiiMat4 m = t.GetAsMat4();

    xiiMat4 refM;
    refM.SetZero();
    {
      xiiQuat q;
      q = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::MakeFromDegree(34));

      xiiTransform referenceTransform(xiiVec3(1, 2, 3), q, xiiVec3(2, -1, 5));
      xiiMat4      tmp = referenceTransform.GetAsMat4();
      refM             = xiiMat4::MakeFromColumnMajorArray(tmp.m_fElementsCM);
    }
    XII_TEST_BOOL(m.IsEqual(refM, 0.00001f));

    xiiVec3 p[8] = {
      xiiVec3(-4, 0, 0), xiiVec3(5, 0, 0), xiiVec3(0, -6, 0), xiiVec3(0, 7, 0), xiiVec3(0, 0, -8), xiiVec3(0, 0, 9), xiiVec3(1, -2, 3), xiiVec3(-4, 5, 7)};

    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(p); ++i)
    {
      xiiVec3 pt = t.TransformPosition(p[i]);
      xiiVec3 pm = m.TransformPosition(p[i]);

      XII_TEST_BOOL(pt.IsEqual(pm, 0.00001f));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TransformPos / Dir / operator*")
  {
    xiiQuat qRotX, qRotY;
    qRotX = xiiQuat::MakeFromAxisAndAngle(xiiVec3(1, 0, 0), xiiAngle::MakeFromDegree(90.0f));
    qRotY = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::MakeFromDegree(90.0f));

    xiiTransform t(xiiVec3(1, 2, 3), qRotY * qRotX, xiiVec3(2, -2, 4));

    xiiVec3 v;
    v = t.TransformPosition(xiiVec3(4, 5, 6));
    XII_TEST_BOOL(v.IsEqual(xiiVec3((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3), 0.0001f));

    v = t.TransformDirection(xiiVec3(4, 5, 6));
    XII_TEST_BOOL(v.IsEqual(xiiVec3((5 * -2), (-6 * 4), (-4 * 2)), 0.0001f));

    v = t * xiiVec3(4, 5, 6);
    XII_TEST_BOOL(v.IsEqual(xiiVec3((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3), 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Operators")
  {
    {
      xiiTransform tParent(xiiVec3(1, 2, 3));
      tParent.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::MakeFromDegree(90));
      tParent.m_vScale    = xiiVec3(2);

      xiiTransform tToChild(xiiVec3(4, 5, 6));
      tToChild.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 0, 1), xiiAngle::MakeFromDegree(90));
      tToChild.m_vScale    = xiiVec3(4);

      // this is exactly the same as SetGlobalTransform
      xiiTransform tChild;
      tChild = tParent * tToChild;

      XII_TEST_BOOL(tChild.m_vPosition.IsEqual(xiiVec3(13, 12, -5), 0.0001f));
      XII_TEST_BOOL(tChild.m_qRotation.IsEqualRotation(tParent.m_qRotation * tToChild.m_qRotation, 0.0001f));
      XII_TEST_BOOL((tChild.m_vScale == xiiVec3(8)));

      tChild = tParent;
      tChild = tChild * tToChild;

      XII_TEST_BOOL(tChild.m_vPosition.IsEqual(xiiVec3(13, 12, -5), 0.0001f));
      XII_TEST_BOOL(tChild.m_qRotation.IsEqualRotation(tParent.m_qRotation * tToChild.m_qRotation, 0.0001f));
      XII_TEST_BOOL((tChild.m_vScale == xiiVec3(8)));

      xiiVec3 a(7, 8, 9);
      xiiVec3 b;
      b = tToChild.TransformPosition(a);
      b = tParent.TransformPosition(b);

      xiiVec3 c;
      c = tChild.TransformPosition(a);

      XII_TEST_BOOL(b.IsEqual(c, 0.0001f));

      // verify that it works exactly like a 4x4 matrix
      const xiiMat4 mParent  = tParent.GetAsMat4();
      const xiiMat4 mToChild = tToChild.GetAsMat4();
      const xiiMat4 mChild   = mParent * mToChild;

      XII_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), 0.0001f));
    }

    {
      xiiTransform t(xiiVec3(1, 2, 3));
      t.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::MakeFromDegree(90));
      t.m_vScale    = xiiVec3(2);

      xiiQuat q;
      q = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 0, 1), xiiAngle::MakeFromDegree(90));

      xiiTransform t2 = t * q;
      xiiTransform t4 = q * t;

      xiiTransform t3 = t;
      t3              = t3 * q;
      XII_TEST_BOOL(t2 == t3);
      XII_TEST_BOOL(t3 != t4);

      xiiVec3 a(7, 8, 9);
      xiiVec3 b;
      b = t2.TransformPosition(a);

      xiiVec3 c = q * a;
      c         = t.TransformPosition(c);

      XII_TEST_BOOL(b.IsEqual(c, 0.0001f));
    }

    {
      xiiTransform t(xiiVec3(1, 2, 3));
      t.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::MakeFromDegree(90));
      t.m_vScale    = xiiVec3(2);

      xiiVec3 p(4, 5, 6);

      xiiTransform t2 = t + p;
      xiiTransform t3 = t;
      t3 += p;
      XII_TEST_BOOL(t2 == t3);

      xiiVec3 a(7, 8, 9);
      xiiVec3 b;
      b = t2.TransformPosition(a);

      xiiVec3 c = t.TransformPosition(a) + p;

      XII_TEST_BOOL(b.IsEqual(c, 0.0001f));
    }

    {
      xiiTransform t(xiiVec3(1, 2, 3));
      t.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::MakeFromDegree(90));
      t.m_vScale    = xiiVec3(2);

      xiiVec3 p(4, 5, 6);

      xiiTransform t2 = t - p;
      xiiTransform t3 = t;
      t3 -= p;
      XII_TEST_BOOL(t2 == t3);

      xiiVec3 a(7, 8, 9);
      xiiVec3 b;
      b = t2.TransformPosition(a);

      xiiVec3 c = t.TransformPosition(a) - p;

      XII_TEST_BOOL(b.IsEqual(c, 0.0001f));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Comparison")
  {
    xiiTransform t(xiiVec3(1, 2, 3));
    t.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::MakeFromDegree(90));

    XII_TEST_BOOL(t == t);

    xiiTransform t2(xiiVec3(1, 2, 4));
    t2.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::MakeFromDegree(90));

    XII_TEST_BOOL(t != t2);

    xiiTransform t3(xiiVec3(1, 2, 3));
    t3.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::MakeFromDegree(91));

    XII_TEST_BOOL(t != t3);
  }
}
