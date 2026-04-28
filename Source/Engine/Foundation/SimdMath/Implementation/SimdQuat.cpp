/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/SimdMath/SimdQuat.h>

xiiSimdQuat xiiSimdQuat::MakeShortestRotation(const xiiSimdVec4f& vDirFrom, const xiiSimdVec4f& vDirTo)
{
  const xiiSimdVec4f v0 = vDirFrom.GetNormalized<3>();
  const xiiSimdVec4f v1 = vDirTo.GetNormalized<3>();

  const xiiSimdFloat fDot = v0.Dot<3>(v1);

  // if both vectors are identical -> no rotation needed
  if (fDot.IsEqual(1.0f, 0.0001f))
  {
    return xiiSimdQuat::MakeIdentity();
  }
  else if (fDot.IsEqual(-1.0f, 0.0001f)) // if both vectors are opposing
  {
    return xiiSimdQuat::MakeFromAxisAndAngle(v0.GetOrthogonalVector().GetNormalized<3>(), xiiAngle::MakeFromRadian(xiiMath::Pi<float>()));
  }

  const xiiSimdVec4f c = v0.CrossRH(v1);
  const xiiSimdFloat s = ((fDot + xiiSimdFloat(1.0f)) * xiiSimdFloat(2.0f)).GetSqrt();

  xiiSimdQuat res;
  res.m_v = c / s;
  res.m_v.SetW(s * xiiSimdFloat(0.5f));
  res.Normalize();
  return res;
}

xiiSimdQuat xiiSimdQuat::MakeSlerp(const xiiSimdQuat& qFrom, const xiiSimdQuat& qTo, const xiiSimdFloat& t)
{
  XII_ASSERT_DEBUG((t >= 0.0f) && (t <= 1.0f), "Invalid lerp factor.");

  const xiiSimdFloat one    = 1.0f;
  const xiiSimdFloat qdelta = 1.0f - 0.001f;

  const xiiSimdFloat fDot = qFrom.m_v.Dot<4>(qTo.m_v);

  xiiSimdFloat cosTheta = fDot;

  bool bFlipSign = false;
  if (cosTheta < 0.0f)
  {
    bFlipSign = true;
    cosTheta  = -cosTheta;
  }

  xiiSimdFloat t0, t1;

  if (cosTheta < qdelta)
  {
    xiiAngle theta = xiiMath::ACos((float)cosTheta);

    // use sqrtInv(1+c^2) instead of 1.0/sin(theta)
    const xiiSimdFloat iSinTheta = (one - (cosTheta * cosTheta)).GetInvSqrt();
    const xiiAngle     tTheta    = (float)t * theta;

    xiiSimdFloat s0 = xiiMath::Sin(theta - tTheta);
    xiiSimdFloat s1 = xiiMath::Sin(tTheta);

    t0 = s0 * iSinTheta;
    t1 = s1 * iSinTheta;
  }
  else
  {
    // If q0 is nearly the same as q1 we just linearly interpolate
    t0 = one - t;
    t1 = t;
  }

  if (bFlipSign)
    t1 = -t1;

  xiiSimdQuat res;
  res.m_v = qFrom.m_v * t0 + qTo.m_v * t1;
  res.Normalize();
  return res;
}

bool xiiSimdQuat::IsEqualRotation(const xiiSimdQuat& qOther, const xiiSimdFloat& fEpsilon) const
{
  xiiSimdVec4f vA1, vA2;
  xiiSimdFloat fA1, fA2;

  if (GetRotationAxisAndAngle(vA1, fA1) == XII_FAILURE)
    return false;
  if (qOther.GetRotationAxisAndAngle(vA2, fA2) == XII_FAILURE)
    return false;

  xiiAngle A1 = xiiAngle::MakeFromRadian(fA1);
  xiiAngle A2 = xiiAngle::MakeFromRadian(fA2);

  if ((A1.IsEqualSimple(A2, xiiAngle::MakeFromDegree(fEpsilon))) && (vA1.IsEqual(vA2, fEpsilon).AllSet<3>()))
    return true;

  if ((A1.IsEqualSimple(-A2, xiiAngle::MakeFromDegree(fEpsilon))) && (vA1.IsEqual(-vA2, fEpsilon).AllSet<3>()))
    return true;

  return false;
}

XII_STATICLINK_FILE(Foundation, Foundation_SimdMath_Implementation_SimdQuat);
