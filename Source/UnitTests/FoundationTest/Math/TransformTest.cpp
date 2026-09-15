/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Transform.h>

template <typename Type>
void TestTransform()
{
  using xiiTransformType = xiiTransformTemplate<Type>;
  using xiiVec3Type      = xiiVec3Template<Type>;
  using xiiMat3Type      = xiiMat3Template<Type>;
  using xiiMat4Type      = xiiMat4Template<Type>;
  using xiiQuatType      = xiiQuatTemplate<Type>;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructors")
  {
    xiiTransformType t0;

    {
      xiiTransformType t(xiiVec3Type(1, 2, 3));
      XII_TEST_VEC3(t.m_vPosition, xiiVec3Type(1, 2, 3), 0);
    }

    {
      xiiQuatType qRot;
      qRot = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(1, 2, 3).GetNormalized(), xiiAngleTemplate<Type>::MakeFromDegree(42.0f));

      xiiTransformType t(xiiVec3Type(4, 5, 6), qRot);
      XII_TEST_VEC3(t.m_vPosition, xiiVec3Type(4, 5, 6), 0);
      XII_TEST_BOOL(t.m_qRotation == qRot);
    }

    {
      xiiMat3Type mRot = xiiMat3Type::MakeAxisRotation(xiiVec3Type(1, 2, 3).GetNormalized(), xiiAngleTemplate<Type>::MakeFromDegree(42.0f));

      xiiQuatType q;
      q = xiiQuatType::MakeFromMat3(mRot);

      xiiTransformType t(xiiVec3Type(4, 5, 6), q);
      XII_TEST_VEC3(t.m_vPosition, xiiVec3Type(4, 5, 6), 0);
      XII_TEST_BOOL(t.m_qRotation.GetAsMat3().IsEqual(mRot, (Type)0.0001));
    }

    {
      xiiQuatType qRot;
      qRot.SetIdentity();

      xiiTransformType t(xiiVec3Type(4, 5, 6), qRot, xiiVec3Type(2, 3, 4));
      XII_TEST_VEC3(t.m_vPosition, xiiVec3Type(4, 5, 6), 0);
      XII_TEST_BOOL(t.m_qRotation.GetAsMat3().IsEqual(xiiMat3Type::MakeFromValues(1, 0, 0, 0, 1, 0, 0, 0, 1), (Type)0.001));
      XII_TEST_VEC3(t.m_vScale, xiiVec3Type(2, 3, 4), 0);
    }

    {
      xiiMat3Type mRot = xiiMat3Type::MakeAxisRotation(xiiVec3Type(1, 2, 3).GetNormalized(), xiiAngleTemplate<Type>::MakeFromDegree(42.0f));
      xiiMat4Type mTrans;
      mTrans.SetTransformationMatrix(mRot, xiiVec3Type(1, 2, 3));

      xiiTransformType t = xiiTransformType::MakeFromMat4(mTrans);
      XII_TEST_VEC3(t.m_vPosition, xiiVec3Type(1, 2, 3), 0);
      XII_TEST_BOOL(t.m_qRotation.GetAsMat3().IsEqual(mRot, (Type)0.001));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetIdentity")
  {
    xiiTransformType t;
    t.SetIdentity();

    XII_TEST_VEC3(t.m_vPosition, xiiVec3Type(0), 0);
    XII_TEST_BOOL(t.m_qRotation == xiiQuatType::MakeIdentity());
    XII_TEST_BOOL(t.m_vScale == xiiVec3Type((Type)1.0));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetAsMat4")
  {
    xiiQuatType qRot;
    qRot.SetIdentity();

    xiiTransformType t(xiiVec3Type(4, 5, 6), qRot, xiiVec3Type(2, 3, 4));
    XII_TEST_BOOL(t.GetAsMat4() == xiiMat4Type::MakeFromValues(2, 0, 0, 4, 0, 3, 0, 5, 0, 0, 4, 6, 0, 0, 0, 1));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator + / -")
  {
    xiiTransformType t0, t1;
    t0.SetIdentity();
    t1.SetIdentity();

    t1 = t0 + xiiVec3Type(2, 3, 4);
    XII_TEST_VEC3(t1.m_vPosition, xiiVec3Type(2, 3, 4), (Type)0.0001);

    t1 = t1 - xiiVec3Type(4, 2, 1);
    XII_TEST_VEC3(t1.m_vPosition, xiiVec3Type(-2, 1, 3), (Type)0.0001);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator * (quat)")
  {
    xiiQuatType qRotX, qRotY;
    qRotX = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(1, 0, 0), xiiAngleTemplate<Type>::MakeFromRadian(1.57079637f));
    qRotY = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 1, 0), xiiAngleTemplate<Type>::MakeFromRadian(1.57079637f));

    xiiTransformType t0, t1;
    t0.SetIdentity();
    t1.SetIdentity();

    t1 = qRotX * t0;
    XII_TEST_VEC3(t1.m_vPosition, xiiVec3Type(0, 0, 0), (Type)0.0001);

    xiiQuatType q;
    q = xiiQuatType::MakeFromMat3(xiiMat3Type::MakeFromValues(1, 0, 0, 0, 0, -1, 0, 1, 0));
    XII_TEST_BOOL(t1.m_qRotation.IsEqualRotation(q, (Type)0.0001));

    t1 = qRotY * t1;
    XII_TEST_VEC3(t1.m_vPosition, xiiVec3Type(0, 0, 0), (Type)0.0001);
    q = xiiQuatType::MakeFromMat3(xiiMat3Type::MakeFromValues(0, 1, 0, 0, 0, -1, -1, 0, 0));
    XII_TEST_BOOL(t1.m_qRotation.IsEqualRotation(q, (Type)0.0001));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator * (vec3)")
  {
    xiiQuatType qRotX, qRotY;
    qRotX = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(1, 0, 0), xiiAngleTemplate<Type>::MakeFromRadian(1.57079637f));
    qRotY = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 1, 0), xiiAngleTemplate<Type>::MakeFromRadian(1.57079637f));

    xiiTransformType t;
    t.SetIdentity();

    t = qRotX * t;

    XII_TEST_BOOL(t.m_qRotation.IsEqualRotation(qRotX, (Type)0.0001));
    XII_TEST_VEC3(t.m_vPosition, xiiVec3Type(0, 0, 0), (Type)0.0001);
    XII_TEST_VEC3(t.m_vScale, xiiVec3Type(1, 1, 1), (Type)0.0001);

    t = t + xiiVec3Type(1, 2, 3);

    XII_TEST_BOOL(t.m_qRotation.IsEqualRotation(qRotX, (Type)0.0001));
    XII_TEST_VEC3(t.m_vPosition, xiiVec3Type(1, 2, 3), (Type)0.0001);
    XII_TEST_VEC3(t.m_vScale, xiiVec3Type(1, 1, 1), (Type)0.0001);

    t = qRotY * t;

    XII_TEST_BOOL(t.m_qRotation.IsEqualRotation(qRotY * qRotX, (Type)0.0001));
    XII_TEST_VEC3(t.m_vPosition, xiiVec3Type(1, 2, 3), (Type)0.0001);
    XII_TEST_VEC3(t.m_vScale, xiiVec3Type(1, 1, 1), (Type)0.0001);

    xiiQuatType q;
    q = xiiQuatType::MakeFromMat3(xiiMat3Type::MakeFromValues(0, 1, 0, 0, 0, -1, -1, 0, 0));
    XII_TEST_BOOL(t.m_qRotation.IsEqualRotation(q, (Type)0.0001));

    xiiVec3Type v;
    v = t * xiiVec3Type(4, 5, 6);

    XII_TEST_VEC3(v, xiiVec3Type(5 + 1, -6 + 2, -4 + 3), (Type)0.0001);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsIdentical")
  {
    xiiTransformType t(xiiVec3Type(1, 2, 3));
    t.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 1, 0), xiiAngleTemplate<Type>::MakeFromDegree(90));

    XII_TEST_BOOL(t.IsIdentical(t));

    xiiTransformType t2(xiiVec3Type(1, 2, 4));
    t2.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 1, 0), xiiAngleTemplate<Type>::MakeFromDegree(90));

    XII_TEST_BOOL(!t.IsIdentical(t2));

    xiiTransformType t3(xiiVec3Type(1, 2, 3));
    t3.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 1, 0), xiiAngleTemplate<Type>::MakeFromDegree(91));

    XII_TEST_BOOL(!t.IsIdentical(t3));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator == / !=")
  {
    xiiTransformType t(xiiVec3Type(1, 2, 3));
    t.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 1, 0), xiiAngleTemplate<Type>::MakeFromDegree(90));

    XII_TEST_BOOL(t == t);

    xiiTransformType t2(xiiVec3Type(1, 2, 4));
    t2.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 1, 0), xiiAngleTemplate<Type>::MakeFromDegree(90));

    XII_TEST_BOOL(t != t2);

    xiiTransformType t3(xiiVec3Type(1, 2, 3));
    t3.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 1, 0), xiiAngleTemplate<Type>::MakeFromDegree(91));

    XII_TEST_BOOL(t != t3);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsEqual")
  {
    xiiTransformType t(xiiVec3Type(1, 2, 3));
    t.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 1, 0), xiiAngleTemplate<Type>::MakeFromDegree(90));

    XII_TEST_BOOL(t.IsEqual(t, xiiMath::DefaultEpsilon<Type>()));

    xiiTransformType t2(xiiVec3Type(1, 2, (Type)3 + 2 * xiiMath::DefaultEpsilon<Type>()));
    t2.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 1, 0), xiiAngleTemplate<Type>::MakeFromDegree(90));

    XII_TEST_BOOL(t.IsEqual(t2, xiiMath::LargeEpsilon<Type>()));
    XII_TEST_BOOL(!t.IsEqual(t2, xiiMath::DefaultEpsilon<Type>()));

    xiiTransformType t3(xiiVec3Type(1, 2, 3));
    t3.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 1, 0), xiiAngleTemplate<Type>::MakeFromDegree(90 + xiiMath::VeryHugeEpsilon<Type>()));

    XII_TEST_BOOL(t.IsEqual(t3, xiiMath::VeryHugeEpsilon<Type>()));
    XII_TEST_BOOL(!t.IsEqual(t3, xiiMath::DefaultEpsilon<Type>()));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator*(xiiTransformType, xiiTransformType)")
  {
    xiiTransformType tParent(xiiVec3Type(1, 2, 3));
    tParent.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 1, 0), xiiAngleTemplate<Type>::MakeFromRadian(1.57079637f));
    tParent.m_vScale.Set(2);

    xiiTransformType tToChild(xiiVec3Type(4, 5, 6));
    tToChild.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 0, 1), xiiAngleTemplate<Type>::MakeFromRadian(1.57079637f));
    tToChild.m_vScale.Set(4);

    // this is exactly the same as SetGlobalTransform
    xiiTransformType tChild;
    tChild = tParent * tToChild;

    XII_TEST_VEC3(tChild.m_vPosition, xiiVec3Type(13, 12, -5), (Type)0.003);
    XII_TEST_BOOL(tChild.m_qRotation.GetAsMat3().IsEqual(xiiMat3Type::MakeFromValues(0, 0, 1, 1, 0, 0, 0, 1, 0), (Type)0.0001));
    XII_TEST_VEC3(tChild.m_vScale, xiiVec3Type(8, 8, 8), (Type)0.0001);

    // verify that it works exactly like a 4x4 matrix
    const xiiMat4Type mParent  = tParent.GetAsMat4();
    const xiiMat4Type mToChild = tToChild.GetAsMat4();
    const xiiMat4Type mChild   = mParent * mToChild;

    XII_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), (Type)0.0001));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator*(xiiTransformType, xiiMat4)")
  {
    xiiTransformType tParent(xiiVec3Type(1, 2, 3));
    tParent.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 1, 0), xiiAngleTemplate<Type>::MakeFromDegree(90));
    tParent.m_vScale.Set(2);

    xiiTransformType tToChild(xiiVec3Type(4, 5, 6));
    tToChild.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 0, 1), xiiAngleTemplate<Type>::MakeFromDegree(90));
    tToChild.m_vScale.Set(4);

    // this is exactly the same as SetGlobalTransform
    xiiTransformType tChild;
    tChild = tParent * tToChild;

    XII_TEST_VEC3(tChild.m_vPosition, xiiVec3Type(13, 12, -5), (Type)0.0001);
    XII_TEST_BOOL(tChild.m_qRotation.GetAsMat3().IsEqual(xiiMat3Type::MakeFromValues(0, 0, 1, 1, 0, 0, 0, 1, 0), (Type)0.0001));
    XII_TEST_VEC3(tChild.m_vScale, xiiVec3Type(8, 8, 8), (Type)0.0001);

    // verify that it works exactly like a 4x4 matrix
    const xiiMat4Type mParent  = tParent.GetAsMat4();
    const xiiMat4Type mToChild = tToChild.GetAsMat4();
    const xiiMat4Type mChild   = mParent * mToChild;

    XII_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), (Type)0.0001));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator*(xiiMat4, xiiTransformType)")
  {
    xiiTransformType tParent(xiiVec3Type(1, 2, 3));
    tParent.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 1, 0), xiiAngleTemplate<Type>::MakeFromDegree(90));
    tParent.m_vScale.Set(2);

    xiiTransformType tToChild(xiiVec3Type(4, 5, 6));
    tToChild.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 0, 1), xiiAngleTemplate<Type>::MakeFromDegree(90));
    tToChild.m_vScale.Set(4);

    // this is exactly the same as SetGlobalTransform
    xiiTransformType tChild;
    tChild = tParent * tToChild;

    XII_TEST_VEC3(tChild.m_vPosition, xiiVec3Type(13, 12, -5), (Type)0.0001);
    XII_TEST_BOOL(tChild.m_qRotation.GetAsMat3().IsEqual(xiiMat3Type::MakeFromValues(0, 0, 1, 1, 0, 0, 0, 1, 0), (Type)0.0001));
    XII_TEST_VEC3(tChild.m_vScale, xiiVec3Type(8, 8, 8), (Type)0.0001);

    // verify that it works exactly like a 4x4 matrix
    const xiiMat4Type mParent  = tParent.GetAsMat4();
    const xiiMat4Type mToChild = tToChild.GetAsMat4();
    const xiiMat4Type mChild   = mParent * mToChild;

    XII_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), (Type)0.0001));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Invert / GetInverse")
  {
    xiiTransformType tParent(xiiVec3Type(1, 2, 3));
    tParent.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 1, 0), xiiAngleTemplate<Type>::MakeFromDegree(90));
    tParent.m_vScale.Set(2);

    xiiTransformType tToChild(xiiVec3Type(4, 5, 6));
    tToChild.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 0, 1), xiiAngleTemplate<Type>::MakeFromDegree(90));
    tToChild.m_vScale.Set(4);

    xiiTransformType tChild;
    tChild = xiiTransformType::MakeGlobalTransform(tParent, tToChild);

    // negate twice -> get back original
    tToChild.Invert();
    tToChild.Invert();

    xiiTransformType tInvToChild = tToChild.GetInverse();

    xiiTransformType tParentFromChild;
    tParentFromChild = xiiTransformType::MakeGlobalTransform(tChild, tInvToChild);

    XII_TEST_BOOL(tParent.IsEqual(tParentFromChild, (Type)0.0001));
  }

  //////////////////////////////////////////////////////////////////////////
  // Tests copied and ported over from xiiSimdTransform
  //////////////////////////////////////////////////////////////////////////

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
    xiiTransformType t0;

    {
      xiiQuatType qRot;
      qRot = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(1, 2, 3).GetNormalized(), xiiAngleTemplate<Type>::MakeFromDegree(42.0f));

      xiiVec3Type pos(4, 5, 6);
      xiiVec3Type scale(7, 8, 9);

      xiiTransformType t(pos);
      XII_TEST_BOOL((t.m_vPosition == pos));
      XII_TEST_BOOL(t.m_qRotation == xiiQuatType::MakeIdentity());
      XII_TEST_BOOL((t.m_vScale == xiiVec3Type(1)));

      t = xiiTransformType(pos, qRot);
      XII_TEST_BOOL((t.m_vPosition == pos));
      XII_TEST_BOOL(t.m_qRotation == qRot);
      XII_TEST_BOOL((t.m_vScale == xiiVec3Type(1)));

      t = xiiTransformType(pos, qRot, scale);
      XII_TEST_BOOL((t.m_vPosition == pos));
      XII_TEST_BOOL(t.m_qRotation == qRot);
      XII_TEST_BOOL((t.m_vScale == scale));

      t = xiiTransformType(xiiVec3Type::MakeZero(), qRot);
      XII_TEST_BOOL(t.m_vPosition.IsZero());
      XII_TEST_BOOL(t.m_qRotation == qRot);
      XII_TEST_BOOL((t.m_vScale == xiiVec3Type(1)));
    }

    {
      xiiTransformType t;
      t.SetIdentity();

      XII_TEST_BOOL(t.m_vPosition.IsZero());
      XII_TEST_BOOL(t.m_qRotation == xiiQuatType::MakeIdentity());
      XII_TEST_BOOL((t.m_vScale == xiiVec3Type(1)));

      XII_TEST_BOOL(t == xiiTransformType::MakeIdentity());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Inverse")
  {
    xiiTransformType tParent(xiiVec3Type(1, 2, 3));
    tParent.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 1, 0), xiiAngleTemplate<Type>::MakeFromDegree(90));
    tParent.m_vScale    = xiiVec3Type(2);

    xiiTransformType tToChild(xiiVec3Type(4, 5, 6));
    tToChild.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 0, 1), xiiAngleTemplate<Type>::MakeFromDegree(90));
    tToChild.m_vScale    = xiiVec3Type(4);

    xiiTransformType tChild;
    tChild = tParent * tToChild;

    // invert twice -> get back original
    xiiTransformType t2 = tToChild;
    t2.Invert();
    t2.Invert();
    XII_TEST_BOOL(t2.IsEqual(tToChild, (Type)0.0001));

    xiiTransformType tInvToChild = tToChild.GetInverse();

    xiiTransformType tParentFromChild;
    tParentFromChild = tChild * tInvToChild;

    XII_TEST_BOOL(tParent.IsEqual(tParentFromChild, (Type)0.0001));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetLocalTransform")
  {
    xiiQuatType q;
    q = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 0, 1), xiiAngleTemplate<Type>::MakeFromDegree(90));

    xiiTransformType tParent(xiiVec3Type(1, 2, 3));
    tParent.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 1, 0), xiiAngleTemplate<Type>::MakeFromDegree(90));
    tParent.m_vScale    = xiiVec3Type(2);

    xiiTransformType tChild;
    tChild.m_vPosition = xiiVec3Type(13, 12, -5);
    tChild.m_qRotation = tParent.m_qRotation * q;
    tChild.m_vScale    = xiiVec3Type(8);

    xiiTransformType tToChild;
    tToChild = xiiTransformType::MakeLocalTransform(tParent, tChild);

    XII_TEST_BOOL(tToChild.m_vPosition.IsEqual(xiiVec3Type(4, 5, 6), (Type)0.0001));
    XII_TEST_BOOL(tToChild.m_qRotation.IsEqualRotation(q, (Type)0.0001));
    XII_TEST_BOOL((tToChild.m_vScale == xiiVec3Type(4)));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetGlobalTransform")
  {
    xiiTransformType tParent(xiiVec3Type(1, 2, 3));
    tParent.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 1, 0), xiiAngleTemplate<Type>::MakeFromDegree(90));
    tParent.m_vScale    = xiiVec3Type(2);

    xiiTransformType tToChild(xiiVec3Type(4, 5, 6));
    tToChild.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 0, 1), xiiAngleTemplate<Type>::MakeFromDegree(90));
    tToChild.m_vScale    = xiiVec3Type(4);

    xiiTransformType tChild;
    tChild = xiiTransformType::MakeGlobalTransform(tParent, tToChild);

    XII_TEST_BOOL(tChild.m_vPosition.IsEqual(xiiVec3Type(13, 12, -5), (Type)0.0001));
    XII_TEST_BOOL(tChild.m_qRotation.IsEqualRotation(tParent.m_qRotation * tToChild.m_qRotation, (Type)0.0001));
    XII_TEST_BOOL((tChild.m_vScale == xiiVec3Type(8)));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetAsMat4")
  {
    xiiTransformType t(xiiVec3Type(1, 2, 3));
    t.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 1, 0), xiiAngleTemplate<Type>::MakeFromDegree(34));
    t.m_vScale    = xiiVec3Type(2, -1, 5);

    xiiMat4Type m = t.GetAsMat4();

    xiiMat4Type refM;
    refM.SetZero();
    {
      xiiQuatType q;
      q = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 1, 0), xiiAngleTemplate<Type>::MakeFromDegree(34));

      xiiTransformType referenceTransform(xiiVec3Type(1, 2, 3), q, xiiVec3Type(2, -1, 5));
      xiiMat4Type      tmp = referenceTransform.GetAsMat4();
      refM                 = xiiMat4Type::MakeFromColumnMajorArray(tmp.m_fElementsCM);
    }
    XII_TEST_BOOL(m.IsEqual(refM, xiiMath::DefaultEpsilon<Type>()));

    xiiVec3Type p[8] = {
      xiiVec3Type(-4, 0, 0),
      xiiVec3Type(5, 0, 0),
      xiiVec3Type(0, -6, 0),
      xiiVec3Type(0, 7, 0),
      xiiVec3Type(0, 0, -8),
      xiiVec3Type(0, 0, 9),
      xiiVec3Type(1, -2, 3),
      xiiVec3Type(-4, 5, 7),
    };

    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(p); ++i)
    {
      xiiVec3Type pt = t.TransformPosition(p[i]);
      xiiVec3Type pm = m.TransformPosition(p[i]);

      XII_TEST_BOOL(pt.IsEqual(pm, xiiMath::DefaultEpsilon<Type>()));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TransformPos / Dir / operator*")
  {
    xiiQuatType qRotX, qRotY;
    qRotX = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(1, 0, 0), xiiAngleTemplate<Type>::MakeFromDegree(90.0f));
    qRotY = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 1, 0), xiiAngleTemplate<Type>::MakeFromDegree(90.0f));

    xiiTransformType t(xiiVec3Type(1, 2, 3), qRotY * qRotX, xiiVec3Type(2, -2, 4));

    xiiVec3Type v;
    v = t.TransformPosition(xiiVec3Type(4, 5, 6));
    XII_TEST_BOOL(v.IsEqual(xiiVec3Type((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3), (Type)0.0001));

    v = t.TransformDirection(xiiVec3Type(4, 5, 6));
    XII_TEST_BOOL(v.IsEqual(xiiVec3Type((5 * -2), (-6 * 4), (-4 * 2)), (Type)0.0001));

    v = t * xiiVec3Type(4, 5, 6);
    XII_TEST_BOOL(v.IsEqual(xiiVec3Type((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3), (Type)0.0001));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Operators")
  {
    {
      xiiTransformType tParent(xiiVec3Type(1, 2, 3));
      tParent.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 1, 0), xiiAngleTemplate<Type>::MakeFromDegree(90));
      tParent.m_vScale    = xiiVec3Type(2);

      xiiTransformType tToChild(xiiVec3Type(4, 5, 6));
      tToChild.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 0, 1), xiiAngleTemplate<Type>::MakeFromDegree(90));
      tToChild.m_vScale    = xiiVec3Type(4);

      // this is exactly the same as SetGlobalTransform
      xiiTransformType tChild;
      tChild = tParent * tToChild;

      XII_TEST_BOOL(tChild.m_vPosition.IsEqual(xiiVec3Type(13, 12, -5), (Type)0.0001));
      XII_TEST_BOOL(tChild.m_qRotation.IsEqualRotation(tParent.m_qRotation * tToChild.m_qRotation, (Type)0.0001));
      XII_TEST_BOOL((tChild.m_vScale == xiiVec3Type(8)));

      tChild = tParent;
      tChild = tChild * tToChild;

      XII_TEST_BOOL(tChild.m_vPosition.IsEqual(xiiVec3Type(13, 12, -5), (Type)0.0001));
      XII_TEST_BOOL(tChild.m_qRotation.IsEqualRotation(tParent.m_qRotation * tToChild.m_qRotation, (Type)0.0001));
      XII_TEST_BOOL((tChild.m_vScale == xiiVec3Type(8)));

      xiiVec3Type a(7, 8, 9);
      xiiVec3Type b;
      b = tToChild.TransformPosition(a);
      b = tParent.TransformPosition(b);

      xiiVec3Type c;
      c = tChild.TransformPosition(a);

      XII_TEST_BOOL(b.IsEqual(c, (Type)0.0001));

      // verify that it works exactly like a 4x4 matrix
      const xiiMat4Type mParent  = tParent.GetAsMat4();
      const xiiMat4Type mToChild = tToChild.GetAsMat4();
      const xiiMat4Type mChild   = mParent * mToChild;

      XII_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), (Type)0.0001));
    }

    {
      xiiTransformType t(xiiVec3Type(1, 2, 3));
      t.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 1, 0), xiiAngleTemplate<Type>::MakeFromDegree(90));
      t.m_vScale    = xiiVec3Type(2);

      xiiQuatType q;
      q = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 0, 1), xiiAngleTemplate<Type>::MakeFromDegree(90));

      xiiTransformType t2 = t * q;
      xiiTransformType t4 = q * t;

      xiiTransformType t3 = t;
      t3                  = t3 * q;
      XII_TEST_BOOL(t2 == t3);
      XII_TEST_BOOL(t3 != t4);

      xiiVec3Type a(7, 8, 9);
      xiiVec3Type b;
      b = t2.TransformPosition(a);

      xiiVec3Type c = q * a;
      c             = t.TransformPosition(c);

      XII_TEST_BOOL(b.IsEqual(c, (Type)0.0001));
    }

    {
      xiiTransformType t(xiiVec3Type(1, 2, 3));
      t.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 1, 0), xiiAngleTemplate<Type>::MakeFromDegree(90));
      t.m_vScale    = xiiVec3Type(2);

      xiiVec3Type p(4, 5, 6);

      xiiTransformType t2 = t + p;
      xiiTransformType t3 = t;
      t3 += p;
      XII_TEST_BOOL(t2 == t3);

      xiiVec3Type a(7, 8, 9);
      xiiVec3Type b;
      b = t2.TransformPosition(a);

      xiiVec3Type c = t.TransformPosition(a) + p;

      XII_TEST_BOOL(b.IsEqual(c, (Type)0.0001));
    }

    {
      xiiTransformType t(xiiVec3Type(1, 2, 3));
      t.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 1, 0), xiiAngleTemplate<Type>::MakeFromDegree(90));
      t.m_vScale    = xiiVec3Type(2);

      xiiVec3Type p(4, 5, 6);

      xiiTransformType t2 = t - p;
      xiiTransformType t3 = t;
      t3 -= p;
      XII_TEST_BOOL(t2 == t3);

      xiiVec3Type a(7, 8, 9);
      xiiVec3Type b;
      b = t2.TransformPosition(a);

      xiiVec3Type c = t.TransformPosition(a) - p;

      XII_TEST_BOOL(b.IsEqual(c, (Type)0.0001));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Comparison")
  {
    xiiTransformType t(xiiVec3Type(1, 2, 3));
    t.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 1, 0), xiiAngleTemplate<Type>::MakeFromDegree(90));

    XII_TEST_BOOL(t == t);

    xiiTransformType t2(xiiVec3Type(1, 2, 4));
    t2.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 1, 0), xiiAngleTemplate<Type>::MakeFromDegree(90));

    XII_TEST_BOOL(t != t2);

    xiiTransformType t3(xiiVec3Type(1, 2, 3));
    t3.m_qRotation = xiiQuatType::MakeFromAxisAndAngle(xiiVec3Type(0, 1, 0), xiiAngleTemplate<Type>::MakeFromDegree(91));

    XII_TEST_BOOL(t != t3);
  }
}

////////////////////////////////////////////////////////////////////////////

XII_CREATE_SIMPLE_TEST(Math, Transformf)
{
  TestTransform<float>();
}

XII_CREATE_SIMPLE_TEST(Math, Transformd)
{
  TestTransform<double>();
}
