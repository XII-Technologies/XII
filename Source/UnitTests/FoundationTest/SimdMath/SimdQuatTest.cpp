#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdQuat.h>

XII_CREATE_SIMPLE_TEST(SimdMath, SimdQuat)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    // In debug the default constructor initializes everything with NaN.
    xiiSimdQuat vDefCtor;
    XII_TEST_BOOL(vDefCtor.IsNaN());
#else

#  if XII_DISABLED(XII_COMPILER_GCC)
    // Placement new of the default constructor should not have any effect on the previous data.
    alignas(16) float testBlock[4] = {1, 2, 3, 4};
    xiiSimdQuat*      pDefCtor     = ::new ((void*)&testBlock[0]) xiiSimdQuat;
    XII_TEST_BOOL(pDefCtor->m_v.x() == 1.0f && pDefCtor->m_v.y() == 2.0f && pDefCtor->m_v.z() == 3.0f && pDefCtor->m_v.w() == 4.0f);
#  endif

#endif

    // Make sure the class didn't accidentally change in size.
#if XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE
    XII_CHECK_AT_COMPILETIME(sizeof(xiiSimdQuat) == 16);
    XII_CHECK_AT_COMPILETIME(XII_ALIGNMENT_OF(xiiSimdQuat) == 16);
#endif
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IdentityQuaternion")
  {
    xiiSimdQuat q = xiiSimdQuat::IdentityQuaternion();

    XII_TEST_BOOL(q.m_v.x() == 0.0f && q.m_v.y() == 0.0f && q.m_v.z() == 0.0f && q.m_v.w() == 1.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetIdentity")
  {
    xiiSimdQuat q(xiiSimdVec4f(1, 2, 3, 4));

    q.SetIdentity();

    XII_TEST_BOOL(q.m_v.x() == 0.0f && q.m_v.y() == 0.0f && q.m_v.z() == 0.0f && q.m_v.w() == 1.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetFromAxisAndAngle / operator* (quat, vec)")
  {
    {
      xiiSimdQuat q;
      q.SetFromAxisAndAngle(xiiSimdVec4f(1, 0, 0), xiiAngle::Degree(90));

      XII_TEST_BOOL((q * xiiSimdVec4f(0, 1, 0)).IsEqual(xiiSimdVec4f(0, 0, 1), 0.0001f).AllSet());
    }

    {
      xiiSimdQuat q;
      q.SetFromAxisAndAngle(xiiSimdVec4f(0, 1, 0), xiiAngle::Degree(90));

      XII_TEST_BOOL((q * xiiSimdVec4f(1, 0, 0)).IsEqual(xiiSimdVec4f(0, 0, -1), 0.0001f).AllSet());
    }

    {
      xiiSimdQuat q;
      q.SetFromAxisAndAngle(xiiSimdVec4f(0, 0, 1), xiiAngle::Degree(90));

      XII_TEST_BOOL((q * xiiSimdVec4f(0, 1, 0)).IsEqual(xiiSimdVec4f(-1, 0, 0), 0.0001f).AllSet());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetShortestRotation / IsEqualRotation")
  {
    xiiSimdQuat q1, q2, q3;
    q1.SetShortestRotation(xiiSimdVec4f(0, 1, 0), xiiSimdVec4f(1, 0, 0));
    q2.SetFromAxisAndAngle(xiiSimdVec4f(0, 0, -1), xiiAngle::Degree(90));
    q3.SetFromAxisAndAngle(xiiSimdVec4f(0, 0, 1), xiiAngle::Degree(-90));

    XII_TEST_BOOL(q1.IsEqualRotation(q2, xiiMath::LargeEpsilon<float>()));
    XII_TEST_BOOL(q1.IsEqualRotation(q3, xiiMath::LargeEpsilon<float>()));

    XII_TEST_BOOL(xiiSimdQuat::IdentityQuaternion().IsEqualRotation(xiiSimdQuat::IdentityQuaternion(), xiiMath::LargeEpsilon<float>()));
    XII_TEST_BOOL(xiiSimdQuat::IdentityQuaternion().IsEqualRotation(xiiSimdQuat(xiiSimdVec4f(0, 0, 0, -1)), xiiMath::LargeEpsilon<float>()));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetSlerp")
  {
    xiiSimdQuat q1, q2, q3, qr;
    q1.SetFromAxisAndAngle(xiiSimdVec4f(0, 0, 1), xiiAngle::Degree(45));
    q2.SetFromAxisAndAngle(xiiSimdVec4f(0, 0, 1), xiiAngle::Degree(0));
    q3.SetFromAxisAndAngle(xiiSimdVec4f(0, 0, 1), xiiAngle::Degree(90));

    qr.SetSlerp(q2, q3, 0.5f);

    XII_TEST_BOOL(q1.IsEqualRotation(qr, 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetRotationAxisAndAngle")
  {
    xiiSimdQuat q1, q2, q3;
    q1.SetShortestRotation(xiiSimdVec4f(0, 1, 0), xiiSimdVec4f(1, 0, 0));
    q2.SetFromAxisAndAngle(xiiSimdVec4f(0, 0, -1), xiiAngle::Degree(90));
    q3.SetFromAxisAndAngle(xiiSimdVec4f(0, 0, 1), xiiAngle::Degree(-90));

    xiiSimdVec4f axis;
    xiiSimdFloat angle;

    XII_TEST_BOOL(q1.GetRotationAxisAndAngle(axis, angle) == XII_SUCCESS);
    XII_TEST_BOOL(axis.IsEqual(xiiSimdVec4f(0, 0, -1), 0.001f).AllSet<3>());
    XII_TEST_FLOAT(xiiAngle::RadToDeg((float)angle), 90, xiiMath::LargeEpsilon<float>());

    XII_TEST_BOOL(q2.GetRotationAxisAndAngle(axis, angle) == XII_SUCCESS);
    XII_TEST_BOOL(axis.IsEqual(xiiSimdVec4f(0, 0, -1), 0.001f).AllSet<3>());
    XII_TEST_FLOAT(xiiAngle::RadToDeg((float)angle), 90, xiiMath::LargeEpsilon<float>());

    XII_TEST_BOOL(q3.GetRotationAxisAndAngle(axis, angle) == XII_SUCCESS);
    XII_TEST_BOOL(axis.IsEqual(xiiSimdVec4f(0, 0, -1), 0.001f).AllSet<3>());
    XII_TEST_FLOAT(xiiAngle::RadToDeg((float)angle), 90, xiiMath::LargeEpsilon<float>());

    XII_TEST_BOOL(xiiSimdQuat::IdentityQuaternion().GetRotationAxisAndAngle(axis, angle) == XII_SUCCESS);
    XII_TEST_BOOL(axis.IsEqual(xiiSimdVec4f(1, 0, 0), 0.001f).AllSet<3>());
    XII_TEST_FLOAT(xiiAngle::RadToDeg((float)angle), 0, xiiMath::LargeEpsilon<float>());

    xiiSimdQuat otherIdentity(xiiSimdVec4f(0, 0, 0, -1));
    XII_TEST_BOOL(otherIdentity.GetRotationAxisAndAngle(axis, angle) == XII_SUCCESS);
    XII_TEST_BOOL(axis.IsEqual(xiiSimdVec4f(1, 0, 0), 0.001f).AllSet<3>());
    XII_TEST_FLOAT(xiiAngle::RadToDeg((float)angle), 360, xiiMath::LargeEpsilon<float>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsValid / Normalize")
  {
    xiiSimdQuat q(xiiSimdVec4f(1, 2, 3, 4));
    XII_TEST_BOOL(!q.IsValid(0.001f));

    q.Normalize();
    XII_TEST_BOOL(q.IsValid(0.001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator-")
  {
    xiiSimdQuat q, q1;
    q.SetFromAxisAndAngle(xiiSimdVec4f(0, 0, 1), xiiAngle::Degree(90));
    q1.SetFromAxisAndAngle(xiiSimdVec4f(0, 0, 1), xiiAngle::Degree(-90));

    xiiSimdQuat q2 = -q;
    XII_TEST_BOOL(q1.IsEqualRotation(q2, 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator*(quat, quat)")
  {
    xiiSimdQuat q1, q2, qr, q3;
    q1.SetFromAxisAndAngle(xiiSimdVec4f(0, 0, 1), xiiAngle::Degree(60));
    q2.SetFromAxisAndAngle(xiiSimdVec4f(0, 0, 1), xiiAngle::Degree(30));
    q3.SetFromAxisAndAngle(xiiSimdVec4f(0, 0, 1), xiiAngle::Degree(90));

    qr = q1 * q2;

    XII_TEST_BOOL(qr.IsEqualRotation(q3, 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator==/!=")
  {
    xiiSimdQuat q1, q2;
    q1.SetFromAxisAndAngle(xiiSimdVec4f(0, 0, 1), xiiAngle::Degree(60));
    q2.SetFromAxisAndAngle(xiiSimdVec4f(0, 0, 1), xiiAngle::Degree(30));
    XII_TEST_BOOL(q1 != q2);

    q2.SetFromAxisAndAngle(xiiSimdVec4f(1, 0, 0), xiiAngle::Degree(60));
    XII_TEST_BOOL(q1 != q2);

    q2.SetFromAxisAndAngle(xiiSimdVec4f(0, 0, 1), xiiAngle::Degree(60));
    XII_TEST_BOOL(q1 == q2);
  }
}
