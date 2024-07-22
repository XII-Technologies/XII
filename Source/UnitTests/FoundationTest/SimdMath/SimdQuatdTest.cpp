#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdQuatd.h>

XII_CREATE_SIMPLE_TEST(SimdMath, SimdQuatd)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    // In debug the default constructor initializes everything with NaN.
    xiiSimdQuatd vDefCtor;
    XII_TEST_BOOL(vDefCtor.IsNaN());
#else

#  if XII_DISABLED(XII_COMPILER_GCC)
    // Placement new of the default constructor should not have any effect on the previous data.
    alignas(32) double testBlock[4] = {1, 2, 3, 4};
    xiiSimdQuatd*      pDefCtor     = ::new ((void*)&testBlock[0]) xiiSimdQuatd;
    XII_TEST_BOOL(pDefCtor->m_v.x() == 1.0 && pDefCtor->m_v.y() == 2.0 && pDefCtor->m_v.z() == 3.0 && pDefCtor->m_v.w() == 4.0);
#  endif

#endif

    // Make sure the class didn't accidentally change in size.
#if XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX
    static_assert(sizeof(xiiSimdQuatd) == 32);
    static_assert(XII_ALIGNMENT_OF(xiiSimdQuatd) == 32);
#endif
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IdentityQuaternion")
  {
    xiiSimdQuatd q = xiiSimdQuatd::MakeIdentity();

    XII_TEST_BOOL(q.m_v.x() == 0.0 && q.m_v.y() == 0.0 && q.m_v.z() == 0.0 && q.m_v.w() == 1.0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetIdentity")
  {
    xiiSimdQuatd q(xiiSimdVec4d(1, 2, 3, 4));

    q = xiiSimdQuatd::MakeIdentity();

    XII_TEST_BOOL(q.m_v.x() == 0.0 && q.m_v.y() == 0.0 && q.m_v.z() == 0.0 && q.m_v.w() == 1.0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetFromAxisAndAngle / operator* (quat, vec)")
  {
    {
      xiiSimdQuatd q = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(1, 0, 0), xiiAngled::MakeFromDegree(90));

      XII_TEST_BOOL((q * xiiSimdVec4d(0, 1, 0)).IsEqual(xiiSimdVec4d(0, 0, 1), 0.0001).AllSet());
    }

    {
      xiiSimdQuatd q = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 1, 0), xiiAngled::MakeFromDegree(90));

      XII_TEST_BOOL((q * xiiSimdVec4d(1, 0, 0)).IsEqual(xiiSimdVec4d(0, 0, -1), 0.0001).AllSet());
    }

    {
      xiiSimdQuatd q = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 0, 1), xiiAngled::MakeFromDegree(90));

      XII_TEST_BOOL((q * xiiSimdVec4d(0, 1, 0)).IsEqual(xiiSimdVec4d(-1, 0, 0), 0.0001).AllSet());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetShortestRotation / IsEqualRotation")
  {
    xiiSimdQuatd q1, q2, q3;
    q1 = xiiSimdQuatd::MakeShortestRotation(xiiSimdVec4d(0, 1, 0), xiiSimdVec4d(1, 0, 0));
    q2 = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 0, -1), xiiAngled::MakeFromDegree(90));
    q3 = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 0, 1), xiiAngled::MakeFromDegree(-90));

    XII_TEST_BOOL(q1.IsEqualRotation(q2, xiiMath::LargeEpsilon<float>()));
    XII_TEST_BOOL(q1.IsEqualRotation(q3, xiiMath::LargeEpsilon<float>()));

    XII_TEST_BOOL(xiiSimdQuatd::MakeIdentity().IsEqualRotation(xiiSimdQuatd::MakeIdentity(), xiiMath::LargeEpsilon<float>()));
    XII_TEST_BOOL(xiiSimdQuatd::MakeIdentity().IsEqualRotation(xiiSimdQuatd(xiiSimdVec4d(0, 0, 0, -1)), xiiMath::LargeEpsilon<float>()));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetSlerp")
  {
    xiiSimdQuatd q1, q2, q3, qr;
    q1 = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 0, 1), xiiAngled::MakeFromDegree(45));
    q2 = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 0, 1), xiiAngled::MakeFromDegree(0));
    q3 = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 0, 1), xiiAngled::MakeFromDegree(90));

    qr = xiiSimdQuatd::MakeSlerp(q2, q3, 0.5);

    XII_TEST_BOOL(q1.IsEqualRotation(qr, 0.0001));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetRotationAxisAndAngle")
  {
    xiiSimdQuatd q1, q2, q3;
    q1 = xiiSimdQuatd::MakeShortestRotation(xiiSimdVec4d(0, 1, 0), xiiSimdVec4d(1, 0, 0));
    q2 = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 0, -1), xiiAngled::MakeFromDegree(90));
    q3 = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 0, 1), xiiAngled::MakeFromDegree(-90));

    xiiSimdVec4d  axis;
    xiiSimdDouble angle;

    XII_TEST_BOOL(q1.GetRotationAxisAndAngle(axis, angle) == XII_SUCCESS);
    XII_TEST_BOOL(axis.IsEqual(xiiSimdVec4d(0, 0, -1), 0.001).AllSet<3>());
    XII_TEST_FLOAT(xiiAngled::RadToDeg((float)angle), 90, xiiMath::LargeEpsilon<float>());

    XII_TEST_BOOL(q2.GetRotationAxisAndAngle(axis, angle) == XII_SUCCESS);
    XII_TEST_BOOL(axis.IsEqual(xiiSimdVec4d(0, 0, -1), 0.001).AllSet<3>());
    XII_TEST_FLOAT(xiiAngled::RadToDeg((float)angle), 90, xiiMath::LargeEpsilon<float>());

    XII_TEST_BOOL(q3.GetRotationAxisAndAngle(axis, angle) == XII_SUCCESS);
    XII_TEST_BOOL(axis.IsEqual(xiiSimdVec4d(0, 0, -1), 0.001).AllSet<3>());
    XII_TEST_FLOAT(xiiAngled::RadToDeg((float)angle), 90, xiiMath::LargeEpsilon<float>());

    XII_TEST_BOOL(xiiSimdQuatd::MakeIdentity().GetRotationAxisAndAngle(axis, angle) == XII_SUCCESS);
    XII_TEST_BOOL(axis.IsEqual(xiiSimdVec4d(1, 0, 0), 0.001).AllSet<3>());
    XII_TEST_FLOAT(xiiAngled::RadToDeg((float)angle), 0, xiiMath::LargeEpsilon<float>());

    xiiSimdQuatd otherIdentity(xiiSimdVec4d(0, 0, 0, -1));
    XII_TEST_BOOL(otherIdentity.GetRotationAxisAndAngle(axis, angle) == XII_SUCCESS);
    XII_TEST_BOOL(axis.IsEqual(xiiSimdVec4d(1, 0, 0), 0.001).AllSet<3>());
    XII_TEST_FLOAT(xiiAngled::RadToDeg((float)angle), 360, xiiMath::LargeEpsilon<float>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsValid / Normalize")
  {
    xiiSimdQuatd q(xiiSimdVec4d(1, 2, 3, 4));
    XII_TEST_BOOL(!q.IsValid(0.001));

    q.Normalize();
    XII_TEST_BOOL(q.IsValid(0.001));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator-")
  {
    xiiSimdQuatd q, q1;
    q  = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 0, 1), xiiAngled::MakeFromDegree(90));
    q1 = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 0, 1), xiiAngled::MakeFromDegree(-90));

    xiiSimdQuatd q2 = -q;
    XII_TEST_BOOL(q1.IsEqualRotation(q2, 0.0001));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator*(quat, quat)")
  {
    xiiSimdQuatd q1, q2, qr, q3;
    q1 = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 0, 1), xiiAngled::MakeFromDegree(60));
    q2 = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 0, 1), xiiAngled::MakeFromDegree(30));
    q3 = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 0, 1), xiiAngled::MakeFromDegree(90));

    qr = q1 * q2;

    XII_TEST_BOOL(qr.IsEqualRotation(q3, 0.0001));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator==/!=")
  {
    xiiSimdQuatd q1, q2;
    q1 = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 0, 1), xiiAngled::MakeFromDegree(60));
    q2 = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 0, 1), xiiAngled::MakeFromDegree(30));
    XII_TEST_BOOL(q1 != q2);

    q2 = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(1, 0, 0), xiiAngled::MakeFromDegree(60));
    XII_TEST_BOOL(q1 != q2);

    q2 = xiiSimdQuatd::MakeFromAxisAndAngle(xiiSimdVec4d(0, 0, 1), xiiAngled::MakeFromDegree(60));
    XII_TEST_BOOL(q1 == q2);
  }
}
