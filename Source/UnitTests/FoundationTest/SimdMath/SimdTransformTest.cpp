#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Transform.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdTransform.h>

XII_CREATE_SIMPLE_TEST(SimdMath, SimdTransform)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
    xiiSimdTransform t0;

    {
      xiiSimdQuat qRot;
      qRot.SetFromAxisAndAngle(xiiSimdVec4f(1, 2, 3).GetNormalized<3>(), xiiAngle::Degree(42.0f));

      xiiSimdVec4f pos(4, 5, 6);
      xiiSimdVec4f scale(7, 8, 9);

      xiiSimdTransform t(pos);
      XII_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      XII_TEST_BOOL(t.m_Rotation == xiiSimdQuat::IdentityQuaternion());
      XII_TEST_BOOL((t.m_Scale == xiiSimdVec4f(1)).AllSet<3>());

      t = xiiSimdTransform(pos, qRot);
      XII_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      XII_TEST_BOOL(t.m_Rotation == qRot);
      XII_TEST_BOOL((t.m_Scale == xiiSimdVec4f(1)).AllSet<3>());

      t = xiiSimdTransform(pos, qRot, scale);
      XII_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      XII_TEST_BOOL(t.m_Rotation == qRot);
      XII_TEST_BOOL((t.m_Scale == scale).AllSet<3>());

      t = xiiSimdTransform(qRot);
      XII_TEST_BOOL(t.m_Position.IsZero<3>());
      XII_TEST_BOOL(t.m_Rotation == qRot);
      XII_TEST_BOOL((t.m_Scale == xiiSimdVec4f(1)).AllSet<3>());
    }

    {
      xiiSimdTransform t;
      t.SetIdentity();

      XII_TEST_BOOL(t.m_Position.IsZero<3>());
      XII_TEST_BOOL(t.m_Rotation == xiiSimdQuat::IdentityQuaternion());
      XII_TEST_BOOL((t.m_Scale == xiiSimdVec4f(1)).AllSet<3>());

      XII_TEST_BOOL(t == xiiSimdTransform::IdentityTransform());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Inverse")
  {
    xiiSimdTransform tParent(xiiSimdVec4f(1, 2, 3));
    tParent.m_Rotation.SetFromAxisAndAngle(xiiSimdVec4f(0, 1, 0), xiiAngle::Degree(90));
    tParent.m_Scale = xiiSimdVec4f(2);

    xiiSimdTransform tToChild(xiiSimdVec4f(4, 5, 6));
    tToChild.m_Rotation.SetFromAxisAndAngle(xiiSimdVec4f(0, 0, 1), xiiAngle::Degree(90));
    tToChild.m_Scale = xiiSimdVec4f(4);

    xiiSimdTransform tChild;
    tChild = tParent * tToChild;

    // invert twice -> get back original
    xiiSimdTransform t2 = tToChild;
    t2.Invert();
    t2.Invert();
    XII_TEST_BOOL(t2.IsEqual(tToChild, 0.0001f));

    xiiSimdTransform tInvToChild = tToChild.GetInverse();

    xiiSimdTransform tParentFromChild;
    tParentFromChild = tChild * tInvToChild;

    XII_TEST_BOOL(tParent.IsEqual(tParentFromChild, 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetLocalTransform")
  {
    xiiSimdQuat q;
    q.SetFromAxisAndAngle(xiiSimdVec4f(0, 0, 1), xiiAngle::Degree(90));

    xiiSimdTransform tParent(xiiSimdVec4f(1, 2, 3));
    tParent.m_Rotation.SetFromAxisAndAngle(xiiSimdVec4f(0, 1, 0), xiiAngle::Degree(90));
    tParent.m_Scale = xiiSimdVec4f(2);

    xiiSimdTransform tChild;
    tChild.m_Position = xiiSimdVec4f(13, 12, -5);
    tChild.m_Rotation = tParent.m_Rotation * q;
    tChild.m_Scale    = xiiSimdVec4f(8);

    xiiSimdTransform tToChild;
    tToChild.SetLocalTransform(tParent, tChild);

    XII_TEST_BOOL(tToChild.m_Position.IsEqual(xiiSimdVec4f(4, 5, 6), 0.0001f).AllSet<3>());
    XII_TEST_BOOL(tToChild.m_Rotation.IsEqualRotation(q, 0.0001f));
    XII_TEST_BOOL((tToChild.m_Scale == xiiSimdVec4f(4)).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetGlobalTransform")
  {
    xiiSimdTransform tParent(xiiSimdVec4f(1, 2, 3));
    tParent.m_Rotation.SetFromAxisAndAngle(xiiSimdVec4f(0, 1, 0), xiiAngle::Degree(90));
    tParent.m_Scale = xiiSimdVec4f(2);

    xiiSimdTransform tToChild(xiiSimdVec4f(4, 5, 6));
    tToChild.m_Rotation.SetFromAxisAndAngle(xiiSimdVec4f(0, 0, 1), xiiAngle::Degree(90));
    tToChild.m_Scale = xiiSimdVec4f(4);

    xiiSimdTransform tChild;
    tChild.SetGlobalTransform(tParent, tToChild);

    XII_TEST_BOOL(tChild.m_Position.IsEqual(xiiSimdVec4f(13, 12, -5), 0.0001f).AllSet<3>());
    XII_TEST_BOOL(tChild.m_Rotation.IsEqualRotation(tParent.m_Rotation * tToChild.m_Rotation, 0.0001f));
    XII_TEST_BOOL((tChild.m_Scale == xiiSimdVec4f(8)).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetAsMat4")
  {
    xiiSimdTransform t(xiiSimdVec4f(1, 2, 3));
    t.m_Rotation.SetFromAxisAndAngle(xiiSimdVec4f(0, 1, 0), xiiAngle::Degree(34));
    t.m_Scale = xiiSimdVec4f(2, -1, 5);

    xiiSimdMat4f m = t.GetAsMat4();

    // reference
    xiiSimdMat4f refM;
    {
      xiiQuat q;
      q.SetFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::Degree(34));

      xiiTransform referenceTransform(xiiVec3(1, 2, 3), q, xiiVec3(2, -1, 5));
      xiiMat4      tmp = referenceTransform.GetAsMat4();
      refM.SetFromArray(tmp.m_fElementsCM, xiiMatrixLayout::ColumnMajor);
    }
    XII_TEST_BOOL(m.IsEqual(refM, 0.00001f));

    xiiSimdVec4f p[8] = {xiiSimdVec4f(-4, 0, 0), xiiSimdVec4f(5, 0, 0), xiiSimdVec4f(0, -6, 0), xiiSimdVec4f(0, 7, 0), xiiSimdVec4f(0, 0, -8),
                         xiiSimdVec4f(0, 0, 9), xiiSimdVec4f(1, -2, 3), xiiSimdVec4f(-4, 5, 7)};

    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(p); ++i)
    {
      xiiSimdVec4f pt = t.TransformPosition(p[i]);
      xiiSimdVec4f pm = m.TransformPosition(p[i]);

      XII_TEST_BOOL(pt.IsEqual(pm, 0.00001f).AllSet<3>());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TransformPos / Dir / operator*")
  {
    xiiSimdQuat qRotX, qRotY;
    qRotX.SetFromAxisAndAngle(xiiSimdVec4f(1, 0, 0), xiiAngle::Degree(90.0f));
    qRotY.SetFromAxisAndAngle(xiiSimdVec4f(0, 1, 0), xiiAngle::Degree(90.0f));

    xiiSimdTransform t(xiiSimdVec4f(1, 2, 3, 10), qRotY * qRotX, xiiSimdVec4f(2, -2, 4, 11));

    xiiSimdVec4f v;
    v = t.TransformPosition(xiiSimdVec4f(4, 5, 6, 12));
    XII_TEST_BOOL(v.IsEqual(xiiSimdVec4f((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3), 0.0001f).AllSet<3>());

    v = t.TransformDirection(xiiSimdVec4f(4, 5, 6, 13));
    XII_TEST_BOOL(v.IsEqual(xiiSimdVec4f((5 * -2), (-6 * 4), (-4 * 2)), 0.0001f).AllSet<3>());

    v = t * xiiSimdVec4f(4, 5, 6, 12);
    XII_TEST_BOOL(v.IsEqual(xiiSimdVec4f((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3), 0.0001f).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Operators")
  {
    {
      xiiSimdTransform tParent(xiiSimdVec4f(1, 2, 3));
      tParent.m_Rotation.SetFromAxisAndAngle(xiiSimdVec4f(0, 1, 0), xiiAngle::Degree(90));
      tParent.m_Scale = xiiSimdVec4f(2);

      xiiSimdTransform tToChild(xiiSimdVec4f(4, 5, 6));
      tToChild.m_Rotation.SetFromAxisAndAngle(xiiSimdVec4f(0, 0, 1), xiiAngle::Degree(90));
      tToChild.m_Scale = xiiSimdVec4f(4);

      // this is exactly the same as SetGlobalTransform
      xiiSimdTransform tChild;
      tChild = tParent * tToChild;

      XII_TEST_BOOL(tChild.m_Position.IsEqual(xiiSimdVec4f(13, 12, -5), 0.0001f).AllSet<3>());
      XII_TEST_BOOL(tChild.m_Rotation.IsEqualRotation(tParent.m_Rotation * tToChild.m_Rotation, 0.0001f));
      XII_TEST_BOOL((tChild.m_Scale == xiiSimdVec4f(8)).AllSet<3>());

      tChild = tParent;
      tChild *= tToChild;

      XII_TEST_BOOL(tChild.m_Position.IsEqual(xiiSimdVec4f(13, 12, -5), 0.0001f).AllSet<3>());
      XII_TEST_BOOL(tChild.m_Rotation.IsEqualRotation(tParent.m_Rotation * tToChild.m_Rotation, 0.0001f));
      XII_TEST_BOOL((tChild.m_Scale == xiiSimdVec4f(8)).AllSet<3>());

      xiiSimdVec4f a(7, 8, 9);
      xiiSimdVec4f b;
      b = tToChild.TransformPosition(a);
      b = tParent.TransformPosition(b);

      xiiSimdVec4f c;
      c = tChild.TransformPosition(a);

      XII_TEST_BOOL(b.IsEqual(c, 0.0001f).AllSet());

      // verify that it works exactly like a 4x4 matrix
      /*const xiiMat4 mParent = tParent.GetAsMat4();
      const xiiMat4 mToChild = tToChild.GetAsMat4();
      const xiiMat4 mChild = mParent * mToChild;

      XII_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), 0.0001f));*/
    }

    {
      xiiSimdTransform t(xiiSimdVec4f(1, 2, 3));
      t.m_Rotation.SetFromAxisAndAngle(xiiSimdVec4f(0, 1, 0), xiiAngle::Degree(90));
      t.m_Scale = xiiSimdVec4f(2);

      xiiSimdQuat q;
      q.SetFromAxisAndAngle(xiiSimdVec4f(0, 0, 1), xiiAngle::Degree(90));

      xiiSimdTransform t2 = t * q;
      xiiSimdTransform t4 = q * t;

      xiiSimdTransform t3 = t;
      t3 *= q;
      XII_TEST_BOOL(t2 == t3);
      XII_TEST_BOOL(t3 != t4);

      xiiSimdVec4f a(7, 8, 9);
      xiiSimdVec4f b;
      b = t2.TransformPosition(a);

      xiiSimdVec4f c = q * a;
      c              = t.TransformPosition(c);

      XII_TEST_BOOL(b.IsEqual(c, 0.0001f).AllSet());
    }

    {
      xiiSimdTransform t(xiiSimdVec4f(1, 2, 3));
      t.m_Rotation.SetFromAxisAndAngle(xiiSimdVec4f(0, 1, 0), xiiAngle::Degree(90));
      t.m_Scale = xiiSimdVec4f(2);

      xiiSimdVec4f p(4, 5, 6);

      xiiSimdTransform t2 = t + p;
      xiiSimdTransform t3 = t;
      t3 += p;
      XII_TEST_BOOL(t2 == t3);

      xiiSimdVec4f a(7, 8, 9);
      xiiSimdVec4f b;
      b = t2.TransformPosition(a);

      xiiSimdVec4f c = t.TransformPosition(a) + p;

      XII_TEST_BOOL(b.IsEqual(c, 0.0001f).AllSet());
    }

    {
      xiiSimdTransform t(xiiSimdVec4f(1, 2, 3));
      t.m_Rotation.SetFromAxisAndAngle(xiiSimdVec4f(0, 1, 0), xiiAngle::Degree(90));
      t.m_Scale = xiiSimdVec4f(2);

      xiiSimdVec4f p(4, 5, 6);

      xiiSimdTransform t2 = t - p;
      xiiSimdTransform t3 = t;
      t3 -= p;
      XII_TEST_BOOL(t2 == t3);

      xiiSimdVec4f a(7, 8, 9);
      xiiSimdVec4f b;
      b = t2.TransformPosition(a);

      xiiSimdVec4f c = t.TransformPosition(a) - p;

      XII_TEST_BOOL(b.IsEqual(c, 0.0001f).AllSet());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Comparison")
  {
    xiiSimdTransform t(xiiSimdVec4f(1, 2, 3));
    t.m_Rotation.SetFromAxisAndAngle(xiiSimdVec4f(0, 1, 0), xiiAngle::Degree(90));

    XII_TEST_BOOL(t == t);

    xiiSimdTransform t2(xiiSimdVec4f(1, 2, 4));
    t2.m_Rotation.SetFromAxisAndAngle(xiiSimdVec4f(0, 1, 0), xiiAngle::Degree(90));

    XII_TEST_BOOL(t != t2);

    xiiSimdTransform t3(xiiSimdVec4f(1, 2, 3));
    t3.m_Rotation.SetFromAxisAndAngle(xiiSimdVec4f(0, 1, 0), xiiAngle::Degree(91));

    XII_TEST_BOOL(t != t3);
  }
}
