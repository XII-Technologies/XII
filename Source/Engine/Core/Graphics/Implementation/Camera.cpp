#include <Core/CorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/World/CoordinateSystem.h>
#include <Foundation/Utilities/GraphicsUtils.h>

class RemapCoordinateSystemProvider : public xiiCoordinateSystemProvider
{
public:
  RemapCoordinateSystemProvider() :
    xiiCoordinateSystemProvider(nullptr)
  {
  }

  virtual void GetCoordinateSystemFloat(const xiiVec3& vGlobalPosition, xiiCoordinateSystem& out_CoordinateSystem) const override
  {
    out_CoordinateSystem.m_vForwardDir = xiiBasisAxis::GetBasisVectorFloat(m_ForwardAxis);
    out_CoordinateSystem.m_vRightDir   = xiiBasisAxis::GetBasisVectorFloat(m_RightAxis);
    out_CoordinateSystem.m_vUpDir      = xiiBasisAxis::GetBasisVectorFloat(m_UpAxis);
  }

  virtual void GetCoordinateSystemDouble(const xiiVec3d& vGlobalPosition, xiiCoordinateSystemDouble& out_CoordinateSystem) const override
  {
    out_CoordinateSystem.m_vForwardDir = xiiBasisAxis::GetBasisVectorDouble(m_ForwardAxis);
    out_CoordinateSystem.m_vRightDir   = xiiBasisAxis::GetBasisVectorDouble(m_RightAxis);
    out_CoordinateSystem.m_vUpDir      = xiiBasisAxis::GetBasisVectorDouble(m_UpAxis);
  }

  virtual void GetCoordinateSystemReal(const xiiVec3Real& vGlobalPosition, xiiCoordinateSystemReal& out_CoordinateSystem) const override
  {
    out_CoordinateSystem.m_vForwardDir = xiiBasisAxis::GetBasisVectorReal(m_ForwardAxis);
    out_CoordinateSystem.m_vRightDir   = xiiBasisAxis::GetBasisVectorReal(m_RightAxis);
    out_CoordinateSystem.m_vUpDir      = xiiBasisAxis::GetBasisVectorReal(m_UpAxis);
  }

  xiiBasisAxis::Enum m_ForwardAxis = xiiBasisAxis::PositiveX;
  xiiBasisAxis::Enum m_RightAxis   = xiiBasisAxis::PositiveY;
  xiiBasisAxis::Enum m_UpAxis      = xiiBasisAxis::PositiveZ;
};

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiCameraMode, 1)
  XII_ENUM_CONSTANT(xiiCameraMode::PerspectiveFixedFovX),
  XII_ENUM_CONSTANT(xiiCameraMode::PerspectiveFixedFovY),
  XII_ENUM_CONSTANT(xiiCameraMode::OrthoFixedWidth),
  XII_ENUM_CONSTANT(xiiCameraMode::OrthoFixedHeight),
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

xiiCamera::xiiCamera()
{
  m_vCameraPosition[0].SetZero();
  m_vCameraPosition[1].SetZero();
  m_mViewMatrix[0].SetIdentity();
  m_mViewMatrix[1].SetIdentity();
  m_mStereoProjectionMatrix[0].SetIdentity();
  m_mStereoProjectionMatrix[1].SetIdentity();

  SetCoordinateSystem(xiiBasisAxis::PositiveX, xiiBasisAxis::PositiveY, xiiBasisAxis::PositiveZ);
}

void xiiCamera::SetCoordinateSystem(xiiBasisAxis::Enum forwardAxis, xiiBasisAxis::Enum rightAxis, xiiBasisAxis::Enum upAxis)
{
  auto provider           = XII_DEFAULT_NEW(RemapCoordinateSystemProvider);
  provider->m_ForwardAxis = forwardAxis;
  provider->m_RightAxis   = rightAxis;
  provider->m_UpAxis      = upAxis;

  m_pCoordinateSystem = provider;
}

void xiiCamera::SetCoordinateSystem(const xiiSharedPtr<xiiCoordinateSystemProvider>& provider)
{
  m_pCoordinateSystem = provider;
}

xiiVec3 xiiCamera::GetPosition(xiiCameraEye eye) const
{
  return MapInternalToExternal(m_vCameraPosition[static_cast<int>(eye)]);
}

xiiVec3 xiiCamera::GetDirForwards(xiiCameraEye eye) const
{
  xiiVec3 decFwd, decRight, decUp, decPos;
  xiiGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], xiiHandedness::LeftHanded);

  return MapInternalToExternal(decFwd);
}

xiiVec3 xiiCamera::GetDirUp(xiiCameraEye eye) const
{
  xiiVec3 decFwd, decRight, decUp, decPos;
  xiiGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], xiiHandedness::LeftHanded);

  return MapInternalToExternal(decUp);
}

xiiVec3 xiiCamera::GetDirRight(xiiCameraEye eye) const
{
  xiiVec3 decFwd, decRight, decUp, decPos;
  xiiGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], xiiHandedness::LeftHanded);

  return MapInternalToExternal(decRight);
}

xiiVec3 xiiCamera::InternalGetPosition(xiiCameraEye eye) const
{
  return m_vCameraPosition[static_cast<int>(eye)];
}

xiiVec3 xiiCamera::InternalGetDirForwards(xiiCameraEye eye) const
{
  xiiVec3 decFwd, decRight, decUp, decPos;
  xiiGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], xiiHandedness::LeftHanded);

  return decFwd;
}

xiiVec3 xiiCamera::InternalGetDirUp(xiiCameraEye eye) const
{
  xiiVec3 decFwd, decRight, decUp, decPos;
  xiiGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], xiiHandedness::LeftHanded);

  return decUp;
}

xiiVec3 xiiCamera::InternalGetDirRight(xiiCameraEye eye) const
{
  xiiVec3 decFwd, decRight, decUp, decPos;
  xiiGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], xiiHandedness::LeftHanded);

  return -decRight;
}

xiiVec3 xiiCamera::MapExternalToInternal(const xiiVec3& v) const
{
  if (m_pCoordinateSystem)
  {
    xiiCoordinateSystem system;
    m_pCoordinateSystem->GetCoordinateSystemFloat(m_vCameraPosition[0], system);

    xiiMat3 m;
    m.SetRow(0, system.m_vForwardDir);
    m.SetRow(1, system.m_vRightDir);
    m.SetRow(2, system.m_vUpDir);

    return m * v;
  }

  return v;
}

xiiVec3 xiiCamera::MapInternalToExternal(const xiiVec3& v) const
{
  if (m_pCoordinateSystem)
  {
    xiiCoordinateSystem system;
    m_pCoordinateSystem->GetCoordinateSystemFloat(m_vCameraPosition[0], system);

    xiiMat3 m;
    m.SetColumn(0, system.m_vForwardDir);
    m.SetColumn(1, system.m_vRightDir);
    m.SetColumn(2, system.m_vUpDir);

    return m * v;
  }

  return v;
}

xiiAngle xiiCamera::GetFovX(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == xiiCameraMode::PerspectiveFixedFovX)
    return xiiAngle::Degree(m_fFovOrDim);

  if (m_Mode == xiiCameraMode::PerspectiveFixedFovY)
    return xiiMath::ATan(xiiMath::Tan(xiiAngle::Degree(m_fFovOrDim) * 0.5f) * fAspectRatioWidthDivHeight) * 2.0f;

  // TODO: HACK
  if (m_Mode == xiiCameraMode::Stereo)
    return xiiAngle::Degree(90);

  XII_REPORT_FAILURE("You cannot get the camera FOV when it is not a perspective camera.");
  return xiiAngle();
}

xiiAngle xiiCamera::GetFovY(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == xiiCameraMode::PerspectiveFixedFovX)
    return xiiMath::ATan(xiiMath::Tan(xiiAngle::Degree(m_fFovOrDim) * 0.5f) / fAspectRatioWidthDivHeight) * 2.0f;

  if (m_Mode == xiiCameraMode::PerspectiveFixedFovY)
    return xiiAngle::Degree(m_fFovOrDim);

  // TODO: HACK
  if (m_Mode == xiiCameraMode::Stereo)
    return xiiAngle::Degree(90);

  XII_REPORT_FAILURE("You cannot get the camera FOV when it is not a perspective camera.");
  return xiiAngle();
}


float xiiCamera::GetDimensionX(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == xiiCameraMode::OrthoFixedWidth)
    return m_fFovOrDim;

  if (m_Mode == xiiCameraMode::OrthoFixedHeight)
    return m_fFovOrDim * fAspectRatioWidthDivHeight;

  XII_REPORT_FAILURE("You cannot get the camera dimensions when it is not an orthographic camera.");
  return 0;
}


float xiiCamera::GetDimensionY(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == xiiCameraMode::OrthoFixedWidth)
    return m_fFovOrDim / fAspectRatioWidthDivHeight;

  if (m_Mode == xiiCameraMode::OrthoFixedHeight)
    return m_fFovOrDim;

  XII_REPORT_FAILURE("You cannot get the camera dimensions when it is not an orthographic camera.");
  return 0;
}

void xiiCamera::SetCameraMode(xiiCameraMode::Enum Mode, float fFovOrDim, float fNearPlane, float fFarPlane)
{
  // early out if no change
  if (m_Mode == Mode && m_fFovOrDim == fFovOrDim && m_fNearPlane == fNearPlane && m_fFarPlane == fFarPlane)
  {
    return;
  }

  m_Mode       = Mode;
  m_fFovOrDim  = fFovOrDim;
  m_fNearPlane = fNearPlane;
  m_fFarPlane  = fFarPlane;

  m_fAspectOfPrecomputedStereoProjection = -1.0f;

  CameraSettingsChanged();
}

void xiiCamera::SetStereoProjection(const xiiMat4& mProjectionLeftEye, const xiiMat4& mProjectionRightEye, float fAspectRatioWidthDivHeight)
{
  m_mStereoProjectionMatrix[static_cast<int>(xiiCameraEye::Left)]  = mProjectionLeftEye;
  m_mStereoProjectionMatrix[static_cast<int>(xiiCameraEye::Right)] = mProjectionRightEye;
  m_fAspectOfPrecomputedStereoProjection                           = fAspectRatioWidthDivHeight;

  CameraSettingsChanged();
}

void xiiCamera::LookAt(const xiiVec3& vCameraPos0, const xiiVec3& vTargetPos0, const xiiVec3& vUp0)
{
  const xiiVec3 vCameraPos = MapExternalToInternal(vCameraPos0);
  const xiiVec3 vTargetPos = MapExternalToInternal(vTargetPos0);
  const xiiVec3 vUp        = MapExternalToInternal(vUp0);

  if (m_Mode == xiiCameraMode::Stereo)
  {
    XII_REPORT_FAILURE("xiiCamera::LookAt is not possible for stereo cameras.");
    return;
  }

  m_mViewMatrix[0]     = xiiGraphicsUtils::CreateLookAtViewMatrix(vCameraPos, vTargetPos, vUp, xiiHandedness::LeftHanded);
  m_mViewMatrix[1]     = m_mViewMatrix[0];
  m_vCameraPosition[1] = m_vCameraPosition[0] = vCameraPos;

  CameraOrientationChanged(true, true);
}

void xiiCamera::SetViewMatrix(const xiiMat4& mLookAtMatrix, xiiCameraEye eye)
{
  const int iEyeIdx = static_cast<int>(eye);

  m_mViewMatrix[iEyeIdx] = mLookAtMatrix;

  xiiVec3 decFwd, decRight, decUp;
  xiiGraphicsUtils::DecomposeViewMatrix(
    m_vCameraPosition[iEyeIdx], decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], xiiHandedness::LeftHanded);

  if (m_Mode != xiiCameraMode::Stereo)
  {
    m_mViewMatrix[1 - iEyeIdx]     = m_mViewMatrix[iEyeIdx];
    m_vCameraPosition[1 - iEyeIdx] = m_vCameraPosition[iEyeIdx];
  }

  CameraOrientationChanged(true, true);
}

void xiiCamera::GetProjectionMatrix(float fAspectRatioWidthDivHeight, xiiMat4& out_projectionMatrix, xiiCameraEye eye, xiiClipSpaceDepthRange::Enum depthRange) const
{
  switch (m_Mode)
  {
    case xiiCameraMode::PerspectiveFixedFovX:
      out_projectionMatrix = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(xiiAngle::Degree(m_fFovOrDim), fAspectRatioWidthDivHeight,
                                                                                         m_fNearPlane, m_fFarPlane, depthRange, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded);
      break;

    case xiiCameraMode::PerspectiveFixedFovY:
      out_projectionMatrix = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(xiiAngle::Degree(m_fFovOrDim), fAspectRatioWidthDivHeight,
                                                                                         m_fNearPlane, m_fFarPlane, depthRange, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded);
      break;

    case xiiCameraMode::OrthoFixedWidth:
      out_projectionMatrix = xiiGraphicsUtils::CreateOrthographicProjectionMatrix(m_fFovOrDim, m_fFovOrDim / fAspectRatioWidthDivHeight, m_fNearPlane,
                                                                                  m_fFarPlane, depthRange, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded);
      break;

    case xiiCameraMode::OrthoFixedHeight:
      out_projectionMatrix = xiiGraphicsUtils::CreateOrthographicProjectionMatrix(m_fFovOrDim * fAspectRatioWidthDivHeight, m_fFovOrDim, m_fNearPlane,
                                                                                  m_fFarPlane, depthRange, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded);
      break;

    case xiiCameraMode::Stereo:
      if (xiiMath::IsEqual(m_fAspectOfPrecomputedStereoProjection, fAspectRatioWidthDivHeight, xiiMath::LargeEpsilon<float>()))
        out_projectionMatrix = m_mStereoProjectionMatrix[static_cast<int>(eye)];
      else
      {
        // Evade to FixedFovY
        out_projectionMatrix = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(xiiAngle::Degree(m_fFovOrDim), fAspectRatioWidthDivHeight,
                                                                                           m_fNearPlane, m_fFarPlane, depthRange, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded);
      }
      break;

    default:
      XII_REPORT_FAILURE("Invalid Camera Mode {0}", (int)m_Mode);
  }
}

void xiiCamera::CameraSettingsChanged()
{
  XII_ASSERT_DEV(m_Mode != xiiCameraMode::None, "Invalid Camera Mode.");
  XII_ASSERT_DEV(m_fNearPlane < m_fFarPlane, "Near and Far Plane are invalid.");
  XII_ASSERT_DEV(m_fFovOrDim > 0.0f, "FOV or Camera Dimension is invalid.");

  ++m_uiSettingsModificationCounter;
}

void xiiCamera::MoveLocally(float fForward, float fRight, float fUp)
{
  m_mViewMatrix[0].SetTranslationVector(m_mViewMatrix[0].GetTranslationVector() - xiiVec3(fRight, fUp, fForward));
  m_mViewMatrix[1].SetTranslationVector(m_mViewMatrix[0].GetTranslationVector());

  xiiVec3 decFwd, decRight, decUp, decPos;
  xiiGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[0], xiiHandedness::LeftHanded);

  m_vCameraPosition[0] = m_vCameraPosition[1] = decPos;

  CameraOrientationChanged(true, false);
}

void xiiCamera::MoveGlobally(float fForward, float fRight, float fUp)
{
  xiiVec3 vMove(fForward, fRight, fUp);

  xiiVec3 decFwd, decRight, decUp, decPos;
  xiiGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[0], xiiHandedness::LeftHanded);

  m_vCameraPosition[0] += vMove;
  m_vCameraPosition[1] = m_vCameraPosition[0];

  m_mViewMatrix[0] = xiiGraphicsUtils::CreateViewMatrix(m_vCameraPosition[0], decFwd, decRight, decUp, xiiHandedness::LeftHanded);

  m_mViewMatrix[1].SetTranslationVector(m_mViewMatrix[0].GetTranslationVector());

  CameraOrientationChanged(true, false);
}

void xiiCamera::ClampRotationAngles(bool bLocalSpace, xiiAngle& forwardAxis, xiiAngle& rightAxis, xiiAngle& upAxis)
{
  if (bLocalSpace)
  {
    if (rightAxis.GetRadian() != 0.0f)
    {
      // Limit how much the camera can look up and down, to prevent it from overturning

      const float    fDot      = InternalGetDirForwards().Dot(xiiVec3(0, 0, -1));
      const xiiAngle fCurAngle = xiiMath::ACos(fDot) - xiiAngle::Degree(90.0f);
      const xiiAngle fNewAngle = fCurAngle + rightAxis;

      const xiiAngle fAllowedAngle = xiiMath::Clamp(fNewAngle, xiiAngle::Degree(-85.0f), xiiAngle::Degree(85.0f));

      rightAxis = fAllowedAngle - fCurAngle;
    }
  }
}

void xiiCamera::RotateLocally(xiiAngle forwardAxis, xiiAngle rightAxis, xiiAngle upAxis)
{
  ClampRotationAngles(true, forwardAxis, rightAxis, upAxis);

  xiiVec3 vDirForwards = InternalGetDirForwards();
  xiiVec3 vDirUp       = InternalGetDirUp();
  xiiVec3 vDirRight    = InternalGetDirRight();

  if (forwardAxis.GetRadian() != 0.0f)
  {
    xiiMat3 m;
    m.SetRotationMatrix(vDirForwards, forwardAxis);

    vDirUp    = m * vDirUp;
    vDirRight = m * vDirRight;
  }

  if (rightAxis.GetRadian() != 0.0f)
  {
    xiiMat3 m;
    m.SetRotationMatrix(vDirRight, rightAxis);

    vDirUp       = m * vDirUp;
    vDirForwards = m * vDirForwards;
  }

  if (upAxis.GetRadian() != 0.0f)
  {
    xiiMat3 m;
    m.SetRotationMatrix(vDirUp, upAxis);

    vDirRight    = m * vDirRight;
    vDirForwards = m * vDirForwards;
  }

  // Using xiiGraphicsUtils::CreateLookAtViewMatrix is not only easier, it also has the advantage that we end up always with orthonormal
  // vectors.
  auto vPos        = InternalGetPosition();
  m_mViewMatrix[0] = xiiGraphicsUtils::CreateLookAtViewMatrix(vPos, vPos + vDirForwards, vDirUp, xiiHandedness::LeftHanded);
  m_mViewMatrix[1] = m_mViewMatrix[0];

  CameraOrientationChanged(false, true);
}

void xiiCamera::RotateGlobally(xiiAngle forwardAxis, xiiAngle rightAxis, xiiAngle upAxis)
{
  ClampRotationAngles(false, forwardAxis, rightAxis, upAxis);

  xiiVec3 vDirForwards = InternalGetDirForwards();
  xiiVec3 vDirUp       = InternalGetDirUp();

  if (forwardAxis.GetRadian() != 0.0f)
  {
    xiiMat3 m;
    m.SetRotationMatrixX(forwardAxis);

    vDirUp       = m * vDirUp;
    vDirForwards = m * vDirForwards;
  }

  if (rightAxis.GetRadian() != 0.0f)
  {
    xiiMat3 m;
    m.SetRotationMatrixY(rightAxis);

    vDirUp       = m * vDirUp;
    vDirForwards = m * vDirForwards;
  }

  if (upAxis.GetRadian() != 0.0f)
  {
    xiiMat3 m;
    m.SetRotationMatrixZ(upAxis);

    vDirUp       = m * vDirUp;
    vDirForwards = m * vDirForwards;
  }

  // Using xiiGraphicsUtils::CreateLookAtViewMatrix is not only easier, it also has the advantage that we end up always with orthonormal
  // vectors.
  auto vPos        = InternalGetPosition();
  m_mViewMatrix[0] = xiiGraphicsUtils::CreateLookAtViewMatrix(vPos, vPos + vDirForwards, vDirUp, xiiHandedness::LeftHanded);
  m_mViewMatrix[1] = m_mViewMatrix[0];

  CameraOrientationChanged(false, true);
}



XII_STATICLINK_FILE(Core, Core_Graphics_Implementation_Camera);
