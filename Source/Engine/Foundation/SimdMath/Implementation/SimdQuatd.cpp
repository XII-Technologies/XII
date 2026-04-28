/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/SimdMath/SimdQuatd.h>

xiiSimdQuatd xiiSimdQuatd::MakeShortestRotation(const xiiSimdVec4d& vDirFrom, const xiiSimdVec4d& vDirTo)
{
  const xiiSimdVec4d v0 = vDirFrom.GetNormalized<3>();
  const xiiSimdVec4d v1 = vDirTo.GetNormalized<3>();

  const xiiSimdDouble fDot = v0.Dot<3>(v1);

  // if both vectors are identical -> no rotation needed
  if (fDot.IsEqual(1.0, 0.0001))
  {
    return xiiSimdQuatd::MakeIdentity();
  }
  else if (fDot.IsEqual(-1.0, 0.0001)) // if both vectors are opposing
  {
    return xiiSimdQuatd::MakeFromAxisAndAngle(v0.GetOrthogonalVector().GetNormalized<3>(), xiiAngled::MakeFromRadian(xiiMath::Pi<double>()));
  }

  const xiiSimdVec4d  c = v0.CrossRH(v1);
  const xiiSimdDouble s = ((fDot + xiiSimdDouble(1.0)) * xiiSimdDouble(2.0)).GetSqrt();

  xiiSimdQuatd res;
  res.m_v = c / s;
  res.m_v.SetW(s * xiiSimdDouble(0.5f));
  res.Normalize();
  return res;
}

xiiSimdQuatd xiiSimdQuatd::MakeSlerp(const xiiSimdQuatd& qFrom, const xiiSimdQuatd& qTo, const xiiSimdDouble& t)
{
  XII_ASSERT_DEBUG((t >= 0.0) && (t <= 1.0), "Invalid lerp factor.");

  const xiiSimdDouble one    = 1.0;
  const xiiSimdDouble qdelta = 1.0 - 0.001;

  const xiiSimdDouble fDot = qFrom.m_v.Dot<4>(qTo.m_v);

  xiiSimdDouble cosTheta = fDot;

  bool bFlipSign = false;
  if (cosTheta < 0.0)
  {
    bFlipSign = true;
    cosTheta  = -cosTheta;
  }

  xiiSimdDouble t0, t1;

  if (cosTheta < qdelta)
  {
    xiiAngled theta = xiiMath::ACos((double)cosTheta);

    // use sqrtInv(1+c^2) instead of 1.0/sin(theta)
    const xiiSimdDouble iSinTheta = (one - (cosTheta * cosTheta)).GetInvSqrt();
    const xiiAngled     tTheta    = (double)t * theta;

    xiiSimdDouble s0 = xiiMath::Sin(theta - tTheta);
    xiiSimdDouble s1 = xiiMath::Sin(tTheta);

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

  xiiSimdQuatd res;
  res.m_v = qFrom.m_v * t0 + qTo.m_v * t1;
  res.Normalize();
  return res;
}

bool xiiSimdQuatd::IsEqualRotation(const xiiSimdQuatd& qOther, const xiiSimdDouble& fEpsilon) const
{
  xiiSimdVec4d  vA1, vA2;
  xiiSimdDouble fA1, fA2;

  if (GetRotationAxisAndAngle(vA1, fA1) == XII_FAILURE)
    return false;
  if (qOther.GetRotationAxisAndAngle(vA2, fA2) == XII_FAILURE)
    return false;

  xiiAngled A1 = xiiAngled::MakeFromRadian(fA1);
  xiiAngled A2 = xiiAngled::MakeFromRadian(fA2);

  if ((A1.IsEqualSimple(A2, xiiAngled::MakeFromDegree(fEpsilon))) && (vA1.IsEqual(vA2, fEpsilon).AllSet<3>()))
    return true;

  if ((A1.IsEqualSimple(-A2, xiiAngled::MakeFromDegree(fEpsilon))) && (vA1.IsEqual(-vA2, fEpsilon).AllSet<3>()))
    return true;

  return false;
}

XII_STATICLINK_FILE(Foundation, Foundation_SimdMath_Implementation_SimdQuatd);
