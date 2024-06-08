#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Transform.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdTransformd.h>

XII_CREATE_SIMPLE_TEST(SimdMath, SimdTransformd)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
    xiiSimdTransformd t0;

    {
      xiiSimdQuatd qRot = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(1, 2, 3).GetNormalized<3>(), xiiAngled::MakeFromDegree(42.0));

      xiiSimdVec4d pos(4, 5, 6);
      xiiSimdVec4d scale(7, 8, 9);

      xiiSimdTransformd t(pos);
      XII_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      XII_TEST_BOOL(t.m_Rotation == xiiSimdQuatd::MakeIdentity());
      XII_TEST_BOOL((t.m_Scale == xiiSimdVec4d(1)).AllSet<3>());

      t = xiiSimdTransformd(pos, qRot);
      XII_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      XII_TEST_BOOL(t.m_Rotation == qRot);
      XII_TEST_BOOL((t.m_Scale == xiiSimdVec4d(1)).AllSet<3>());

      t = xiiSimdTransformd(pos, qRot, scale);
      XII_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      XII_TEST_BOOL(t.m_Rotation == qRot);
      XII_TEST_BOOL((t.m_Scale == scale).AllSet<3>());

      t = xiiSimdTransformd(qRot);
      XII_TEST_BOOL(t.m_Position.IsZero<3>());
      XII_TEST_BOOL(t.m_Rotation == qRot);
      XII_TEST_BOOL((t.m_Scale == xiiSimdVec4d(1)).AllSet<3>());
    }

    {
      xiiSimdQuatd qRot;
      qRot = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(1, 2, 3).GetNormalized<3>(), xiiAngled::MakeFromDegree(42.0));

      xiiSimdVec4d pos(4, 5, 6);
      xiiSimdVec4d scale(7, 8, 9);

      xiiSimdTransformd t = xiiSimdTransformd::Make(pos);
      XII_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      XII_TEST_BOOL(t.m_Rotation == xiiSimdQuatd::MakeIdentity());
      XII_TEST_BOOL((t.m_Scale == xiiSimdVec4d(1)).AllSet<3>());

      t = xiiSimdTransformd::Make(pos, qRot);
      XII_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      XII_TEST_BOOL(t.m_Rotation == qRot);
      XII_TEST_BOOL((t.m_Scale == xiiSimdVec4d(1)).AllSet<3>());

      t = xiiSimdTransformd::Make(pos, qRot, scale);
      XII_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      XII_TEST_BOOL(t.m_Rotation == qRot);
      XII_TEST_BOOL((t.m_Scale == scale).AllSet<3>());
    }

    {
      xiiSimdTransformd t = xiiSimdTransformd::MakeIdentity();

      XII_TEST_BOOL(t.m_Position.IsZero<3>());
      XII_TEST_BOOL(t.m_Rotation == xiiSimdQuatd::MakeIdentity());
      XII_TEST_BOOL((t.m_Scale == xiiSimdVec4d(1)).AllSet<3>());

      XII_TEST_BOOL(t == xiiSimdTransformd::MakeIdentity());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Inverse")
  {
    xiiSimdTransformd tParent(xiiSimdVec4d(1, 2, 3));
    tParent.m_Rotation = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 1, 0), xiiAngled::MakeFromDegree(90));
    tParent.m_Scale    = xiiSimdVec4d(2);

    xiiSimdTransformd tToChild(xiiSimdVec4d(4, 5, 6));
    tToChild.m_Rotation = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 0, 1), xiiAngled::MakeFromDegree(90));
    tToChild.m_Scale    = xiiSimdVec4d(4);

    xiiSimdTransformd tChild;
    tChild = tParent * tToChild;

    // invert twice -> get back original
    xiiSimdTransformd t2 = tToChild;
    t2.Invert();
    t2.Invert();
    XII_TEST_BOOL(t2.IsEqual(tToChild, 0.0001));

    xiiSimdTransformd tInvToChild = tToChild.GetInverse();

    xiiSimdTransformd tParentFromChild;
    tParentFromChild = tChild * tInvToChild;

    XII_TEST_BOOL(tParent.IsEqual(tParentFromChild, 0.0001));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetLocalTransform")
  {
    xiiSimdQuatd q;
    q = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 0, 1), xiiAngled::MakeFromDegree(90));

    xiiSimdTransformd tParent(xiiSimdVec4d(1, 2, 3));
    tParent.m_Rotation = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 1, 0), xiiAngled::MakeFromDegree(90));
    tParent.m_Scale    = xiiSimdVec4d(2);

    xiiSimdTransformd tChild;
    tChild.m_Position = xiiSimdVec4d(13, 12, -5);
    tChild.m_Rotation = tParent.m_Rotation * q;
    tChild.m_Scale    = xiiSimdVec4d(8);

    xiiSimdTransformd tToChild = xiiSimdTransformd::MakeLocalTransform(tParent, tChild);

    XII_TEST_BOOL(tToChild.m_Position.IsEqual(xiiSimdVec4d(4, 5, 6), 0.0001).AllSet<3>());
    XII_TEST_BOOL(tToChild.m_Rotation.IsEqualRotation(q, 0.0001));
    XII_TEST_BOOL((tToChild.m_Scale == xiiSimdVec4d(4)).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetGlobalTransform")
  {
    xiiSimdTransformd tParent(xiiSimdVec4d(1, 2, 3));
    tParent.m_Rotation = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 1, 0), xiiAngled::MakeFromDegree(90));
    tParent.m_Scale    = xiiSimdVec4d(2);

    xiiSimdTransformd tToChild(xiiSimdVec4d(4, 5, 6));
    tToChild.m_Rotation = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 0, 1), xiiAngled::MakeFromDegree(90));
    tToChild.m_Scale    = xiiSimdVec4d(4);

    xiiSimdTransformd tChild = xiiSimdTransformd::MakeGlobalTransform(tParent, tToChild);

    XII_TEST_BOOL(tChild.m_Position.IsEqual(xiiSimdVec4d(13, 12, -5), 0.0001).AllSet<3>());
    XII_TEST_BOOL(tChild.m_Rotation.IsEqualRotation(tParent.m_Rotation * tToChild.m_Rotation, 0.0001));
    XII_TEST_BOOL((tChild.m_Scale == xiiSimdVec4d(8)).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetAsMat4")
  {
    xiiSimdTransformd t(xiiSimdVec4d(1, 2, 3));
    t.m_Rotation = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 1, 0), xiiAngled::MakeFromDegree(34));
    t.m_Scale    = xiiSimdVec4d(2, -1, 5);

    xiiSimdMat4d m = t.GetAsMat4();

    // reference
    xiiSimdMat4d refM;
    {
      xiiQuatd q = xiiQuatd::MakeFromAxisAndAngle(xiiVec3d(0, 1, 0), xiiAngled::MakeFromDegree(34));

      xiiTransformd referenceTransform(xiiVec3d(1, 2, 3), q, xiiVec3d(2, -1, 5));
      xiiMat4d      tmp = referenceTransform.GetAsMat4();
      refM              = xiiSimdMat4d::MakeFromColumnMajorArray(tmp.m_fElementsCM);
    }
    XII_TEST_BOOL(m.IsEqual(refM, 0.00001));

    xiiSimdVec4d p[8] = {xiiSimdVec4d(-4, 0, 0), xiiSimdVec4d(5, 0, 0), xiiSimdVec4d(0, -6, 0), xiiSimdVec4d(0, 7, 0), xiiSimdVec4d(0, 0, -8),
                         xiiSimdVec4d(0, 0, 9), xiiSimdVec4d(1, -2, 3), xiiSimdVec4d(-4, 5, 7)};

    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(p); ++i)
    {
      xiiSimdVec4d pt = t.TransformPosition(p[i]);
      xiiSimdVec4d pm = m.TransformPosition(p[i]);

      XII_TEST_BOOL(pt.IsEqual(pm, 0.00001).AllSet<3>());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TransformPos / Dir / operator*")
  {
    xiiSimdQuatd qRotX, qRotY;
    qRotX = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(1, 0, 0), xiiAngled::MakeFromDegree(90.0));
    qRotY = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 1, 0), xiiAngled::MakeFromDegree(90.0));

    xiiSimdTransformd t(xiiSimdVec4d(1, 2, 3, 10), qRotY * qRotX, xiiSimdVec4d(2, -2, 4, 11));

    xiiSimdVec4d v;
    v = t.TransformPosition(xiiSimdVec4d(4, 5, 6, 12));
    XII_TEST_BOOL(v.IsEqual(xiiSimdVec4d((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3), 0.0001).AllSet<3>());

    v = t.TransformDirection(xiiSimdVec4d(4, 5, 6, 13));
    XII_TEST_BOOL(v.IsEqual(xiiSimdVec4d((5 * -2), (-6 * 4), (-4 * 2)), 0.0001).AllSet<3>());

    v = t * xiiSimdVec4d(4, 5, 6, 12);
    XII_TEST_BOOL(v.IsEqual(xiiSimdVec4d((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3), 0.0001).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Operators")
  {
    {
      xiiSimdTransformd tParent(xiiSimdVec4d(1, 2, 3));
      tParent.m_Rotation = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 1, 0), xiiAngled::MakeFromDegree(90));
      tParent.m_Scale    = xiiSimdVec4d(2);

      xiiSimdTransformd tToChild(xiiSimdVec4d(4, 5, 6));
      tToChild.m_Rotation = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 0, 1), xiiAngled::MakeFromDegree(90));
      tToChild.m_Scale    = xiiSimdVec4d(4);

      // this is exactly the same as SetGlobalTransform
      xiiSimdTransformd tChild;
      tChild = tParent * tToChild;

      XII_TEST_BOOL(tChild.m_Position.IsEqual(xiiSimdVec4d(13, 12, -5), 0.0001).AllSet<3>());
      XII_TEST_BOOL(tChild.m_Rotation.IsEqualRotation(tParent.m_Rotation * tToChild.m_Rotation, 0.0001));
      XII_TEST_BOOL((tChild.m_Scale == xiiSimdVec4d(8)).AllSet<3>());

      tChild = tParent;
      tChild *= tToChild;

      XII_TEST_BOOL(tChild.m_Position.IsEqual(xiiSimdVec4d(13, 12, -5), 0.0001).AllSet<3>());
      XII_TEST_BOOL(tChild.m_Rotation.IsEqualRotation(tParent.m_Rotation * tToChild.m_Rotation, 0.0001));
      XII_TEST_BOOL((tChild.m_Scale == xiiSimdVec4d(8)).AllSet<3>());

      xiiSimdVec4d a(7, 8, 9);
      xiiSimdVec4d b;
      b = tToChild.TransformPosition(a);
      b = tParent.TransformPosition(b);

      xiiSimdVec4d c;
      c = tChild.TransformPosition(a);

      XII_TEST_BOOL(b.IsEqual(c, 0.0001).AllSet());

      // verify that it works exactly like a 4x4 matrix
#if 0
      const xiiMat4d mParent = tParent.GetAsMat4();
      const xiiMat4d mToChild = tToChild.GetAsMat4();
      const xiiMat4d mChild = mParent * mToChild;

      XII_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), 0.0001));
#endif
    }

    {
      xiiSimdTransformd t(xiiSimdVec4d(1, 2, 3));
      t.m_Rotation = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 1, 0), xiiAngled::MakeFromDegree(90));
      t.m_Scale    = xiiSimdVec4d(2);

      xiiSimdQuatd q = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 0, 1), xiiAngled::MakeFromDegree(90));

      xiiSimdTransformd t2 = t * q;
      xiiSimdTransformd t4 = q * t;

      xiiSimdTransformd t3 = t;
      t3 *= q;
      XII_TEST_BOOL(t2 == t3);
      XII_TEST_BOOL(t3 != t4);

      xiiSimdVec4d a(7, 8, 9);
      xiiSimdVec4d b;
      b = t2.TransformPosition(a);

      xiiSimdVec4d c = q * a;
      c              = t.TransformPosition(c);

      XII_TEST_BOOL(b.IsEqual(c, 0.0001).AllSet());
    }

    {
      xiiSimdTransformd t(xiiSimdVec4d(1, 2, 3));
      t.m_Rotation = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 1, 0), xiiAngled::MakeFromDegree(90));
      t.m_Scale    = xiiSimdVec4d(2);

      xiiSimdVec4d p(4, 5, 6);

      xiiSimdTransformd t2 = t + p;
      xiiSimdTransformd t3 = t;
      t3 += p;
      XII_TEST_BOOL(t2 == t3);

      xiiSimdVec4d a(7, 8, 9);
      xiiSimdVec4d b;
      b = t2.TransformPosition(a);

      xiiSimdVec4d c = t.TransformPosition(a) + p;

      XII_TEST_BOOL(b.IsEqual(c, 0.0001).AllSet());
    }

    {
      xiiSimdTransformd t(xiiSimdVec4d(1, 2, 3));
      t.m_Rotation = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 1, 0), xiiAngled::MakeFromDegree(90));
      t.m_Scale    = xiiSimdVec4d(2);

      xiiSimdVec4d p(4, 5, 6);

      xiiSimdTransformd t2 = t - p;
      xiiSimdTransformd t3 = t;
      t3 -= p;
      XII_TEST_BOOL(t2 == t3);

      xiiSimdVec4d a(7, 8, 9);
      xiiSimdVec4d b;
      b = t2.TransformPosition(a);

      xiiSimdVec4d c = t.TransformPosition(a) - p;

      XII_TEST_BOOL(b.IsEqual(c, 0.0001).AllSet());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Comparison")
  {
    xiiSimdTransformd t(xiiSimdVec4d(1, 2, 3));
    t.m_Rotation = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 1, 0), xiiAngled::MakeFromDegree(90));

    XII_TEST_BOOL(t == t);

    xiiSimdTransformd t2(xiiSimdVec4d(1, 2, 4));
    t2.m_Rotation = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 1, 0), xiiAngled::MakeFromDegree(90));

    XII_TEST_BOOL(t != t2);

    xiiSimdTransformd t3(xiiSimdVec4d(1, 2, 3));
    t3.m_Rotation = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 1, 0), xiiAngled::MakeFromDegree(91));

    XII_TEST_BOOL(t != t3);
  }
}
