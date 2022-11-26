#pragma once

inline xiiVec3 xiiCamera::GetCenterPosition() const
{
  if (m_Mode == xiiCameraMode::Stereo)
    return (GetPosition(xiiCameraEye::Left) + GetPosition(xiiCameraEye::Right)) * 0.5f;
  else
    return GetPosition();
}

inline xiiVec3 xiiCamera::GetCenterDirForwards() const
{
  if (m_Mode == xiiCameraMode::Stereo)
    return (GetDirForwards(xiiCameraEye::Left) + GetDirForwards(xiiCameraEye::Right)).GetNormalized();
  else
    return GetDirForwards();
}

inline xiiVec3 xiiCamera::GetCenterDirUp() const
{
  if (m_Mode == xiiCameraMode::Stereo)
    return (GetDirUp(xiiCameraEye::Left) + GetDirUp(xiiCameraEye::Right)).GetNormalized();
  else
    return GetDirUp();
}

inline xiiVec3 xiiCamera::GetCenterDirRight() const
{
  if (m_Mode == xiiCameraMode::Stereo)
    return (GetDirRight(xiiCameraEye::Left) + GetDirRight(xiiCameraEye::Right)).GetNormalized();
  else
    return GetDirRight();
}

XII_ALWAYS_INLINE float xiiCamera::GetNearPlane() const
{
  return m_fNearPlane;
}

XII_ALWAYS_INLINE float xiiCamera::GetFarPlane() const
{
  return m_fFarPlane;
}

XII_ALWAYS_INLINE float xiiCamera::GetFovOrDim() const
{
  return m_fFovOrDim;
}

XII_ALWAYS_INLINE xiiCameraMode::Enum xiiCamera::GetCameraMode() const
{
  return m_Mode;
}

XII_ALWAYS_INLINE bool xiiCamera::IsPerspective() const
{
  return m_Mode == xiiCameraMode::PerspectiveFixedFovX || m_Mode == xiiCameraMode::PerspectiveFixedFovY ||
    m_Mode == xiiCameraMode::Stereo; // All HMD stereo cameras are perspective!
}

XII_ALWAYS_INLINE bool xiiCamera::IsOrthographic() const
{
  return m_Mode == xiiCameraMode::OrthoFixedWidth || m_Mode == xiiCameraMode::OrthoFixedHeight;
}

XII_ALWAYS_INLINE bool xiiCamera::IsStereoscopic() const
{
  return m_Mode == xiiCameraMode::Stereo;
}

XII_ALWAYS_INLINE float xiiCamera::GetExposure() const
{
  return m_fExposure;
}

XII_ALWAYS_INLINE void xiiCamera::SetExposure(float fExposure)
{
  m_fExposure = fExposure;
}

XII_ALWAYS_INLINE const xiiMat4& xiiCamera::GetViewMatrix(xiiCameraEye eye) const
{
  return m_mViewMatrix[static_cast<int>(eye)];
}
