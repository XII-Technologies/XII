#include <Core/CorePCH.h>

#include <Core/World/CoordinateSystem.h>


xiiCoordinateSystemConversion::xiiCoordinateSystemConversion()
{
  m_mSourceToTarget.SetIdentity();
  m_mTargetToSource.SetIdentity();
}

void xiiCoordinateSystemConversion::SetConversion(const xiiCoordinateSystem& source, const xiiCoordinateSystem& target)
{
  float fSourceScale = source.m_vForwardDir.GetLengthSquared();
  XII_ASSERT_DEV(xiiMath::IsEqual(fSourceScale, source.m_vRightDir.GetLengthSquared(), xiiMath::DefaultEpsilon<float>()),
                 "Only uniformly scaled coordinate systems are supported");
  XII_ASSERT_DEV(xiiMath::IsEqual(fSourceScale, source.m_vUpDir.GetLengthSquared(), xiiMath::DefaultEpsilon<float>()),
                 "Only uniformly scaled coordinate systems are supported");
  xiiMat3 mSourceFromId;
  mSourceFromId.SetColumn(0, source.m_vRightDir);
  mSourceFromId.SetColumn(1, source.m_vUpDir);
  mSourceFromId.SetColumn(2, source.m_vForwardDir);

  float fTargetScale = target.m_vForwardDir.GetLengthSquared();
  XII_ASSERT_DEV(xiiMath::IsEqual(fTargetScale, target.m_vRightDir.GetLengthSquared(), xiiMath::DefaultEpsilon<float>()),
                 "Only uniformly scaled coordinate systems are supported");
  XII_ASSERT_DEV(xiiMath::IsEqual(fTargetScale, target.m_vUpDir.GetLengthSquared(), xiiMath::DefaultEpsilon<float>()),
                 "Only uniformly scaled coordinate systems are supported");
  xiiMat3 mTargetFromId;
  mTargetFromId.SetColumn(0, target.m_vRightDir);
  mTargetFromId.SetColumn(1, target.m_vUpDir);
  mTargetFromId.SetColumn(2, target.m_vForwardDir);

  m_mSourceToTarget = mTargetFromId * mSourceFromId.GetInverse();
  m_mSourceToTarget.SetColumn(0, m_mSourceToTarget.GetColumn(0).GetNormalized());
  m_mSourceToTarget.SetColumn(1, m_mSourceToTarget.GetColumn(1).GetNormalized());
  m_mSourceToTarget.SetColumn(2, m_mSourceToTarget.GetColumn(2).GetNormalized());

  m_fWindingSwap         = m_mSourceToTarget.GetDeterminant() < 0 ? -1.0f : 1.0f;
  m_fSourceToTargetScale = 1.0f / xiiMath::Sqrt(fSourceScale) * xiiMath::Sqrt(fTargetScale);
  m_mTargetToSource      = m_mSourceToTarget.GetInverse();
  m_fTargetToSourceScale = 1.0f / m_fSourceToTargetScale;
}

xiiVec3 xiiCoordinateSystemConversion::ConvertSourcePosition(const xiiVec3& vPos) const
{
  return m_mSourceToTarget * vPos * m_fSourceToTargetScale;
}

xiiQuat xiiCoordinateSystemConversion::ConvertSourceRotation(const xiiQuat& qOrientation) const
{
  xiiVec3 axis = m_mSourceToTarget * qOrientation.GetVectorPart();
  xiiQuat rr(axis.x, axis.y, axis.z, qOrientation.w * m_fWindingSwap);
  return rr;
}

float xiiCoordinateSystemConversion::ConvertSourceLength(float fLength) const
{
  return fLength * m_fSourceToTargetScale;
}

xiiVec3 xiiCoordinateSystemConversion::ConvertTargetPosition(const xiiVec3& vPos) const
{
  return m_mTargetToSource * vPos * m_fTargetToSourceScale;
}

xiiQuat xiiCoordinateSystemConversion::ConvertTargetRotation(const xiiQuat& qOrientation) const
{
  xiiVec3 axis = m_mTargetToSource * qOrientation.GetVectorPart();
  xiiQuat rr(axis.x, axis.y, axis.z, qOrientation.w * m_fWindingSwap);
  return rr;
}

float xiiCoordinateSystemConversion::ConvertTargetLength(float fLength) const
{
  return fLength * m_fTargetToSourceScale;
}

XII_STATICLINK_FILE(Core, Core_World_Implementation_CoordinateSystem);
