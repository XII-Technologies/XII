#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>

xiiOrbitCameraContext::xiiOrbitCameraContext(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView)
{
  m_Volume.SetInvalid();
  m_pCamera = nullptr;

  m_Mode = Mode::Off;

  SetOrbitVolume(xiiVec3(0.0f), xiiVec3(5.0f), xiiVec3(-2, 0, 0), true);

  // while the camera moves, ignore all other shortcuts
  SetShortcutsDisabled(true);

  SetOwner(pOwnerWindow, pOwnerView);
}

void xiiOrbitCameraContext::SetCamera(xiiCamera* pCamera)
{
  if (m_pCamera == pCamera)
    return;

  m_pCamera = pCamera;
}

xiiCamera* xiiOrbitCameraContext::GetCamera() const
{
  return m_pCamera;
}

void xiiOrbitCameraContext::SetOrbitVolume(const xiiVec3& vCenterPos, const xiiVec3& vHalfBoxSize, const xiiVec3& vDefaultCameraPosition, bool bSetCamLookat)
{
  if (!vDefaultCameraPosition.IsValid())
    return;

  if (!m_Volume.GetCenter().IsEqual(vCenterPos, 0.01f) || !m_Volume.GetHalfExtents().IsEqual(vHalfBoxSize, 0.01f))
  {
    bSetCamLookat = true;
  }

  m_Volume.SetCenterAndHalfExtents(vCenterPos, vHalfBoxSize);
  m_vDefaultCameraPosition = vDefaultCameraPosition;

  if (m_pCamera && bSetCamLookat)
  {
    m_vOrbitPoint = vCenterPos;
    m_pCamera->LookAt(vDefaultCameraPosition, vCenterPos, xiiVec3(0, 0, 1));
  }
}

void xiiOrbitCameraContext::DoFocusLost(bool bCancel)
{
  m_Mode = Mode::Off;

  ResetCursor();
}

xiiEditorInput xiiOrbitCameraContext::DoMousePressEvent(QMouseEvent* e)
{
  if (m_pCamera == nullptr)
    return xiiEditorInput::MayBeHandledByOthers;

  if (!m_pCamera->IsPerspective())
    return xiiEditorInput::MayBeHandledByOthers;

  if (m_Mode == Mode::Off)
  {
    if (e->button() == Qt::MouseButton::LeftButton)
    {
      m_Mode = Mode::Orbit;
      goto activate;
    }

    if (e->button() == Qt::MouseButton::RightButton)
    {
      m_Mode = Mode::UpDown;
      goto activate;
    }

    if (e->button() == Qt::MouseButton::MiddleButton)
    {
      m_Mode = Mode::MovePlane;
      goto activate;
    }
  }

  if (m_Mode == Mode::Orbit)
  {
    if (e->button() == Qt::MouseButton::RightButton)
      m_Mode = Mode::Pan;

    goto activate;
  }

  if (m_Mode == Mode::UpDown)
  {
    if (e->button() == Qt::MouseButton::LeftButton)
      m_Mode = Mode::Pan;

    goto activate;
  }

  return xiiEditorInput::MayBeHandledByOthers;

activate:
{
  m_vLastMousePos = SetMouseMode(xiiEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
  MakeActiveInputContext();
  return xiiEditorInput::WasExclusivelyHandled;
}
}

xiiEditorInput xiiOrbitCameraContext::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;

  if (m_pCamera == nullptr)
    return xiiEditorInput::MayBeHandledByOthers;

  if (m_Mode == Mode::Off)
    return xiiEditorInput::MayBeHandledByOthers;


  if (m_Mode == Mode::Orbit)
  {
    if (e->button() == Qt::MouseButton::LeftButton)
      m_Mode = Mode::Off;
  }

  if (m_Mode == Mode::UpDown)
  {
    if (e->button() == Qt::MouseButton::RightButton)
      m_Mode = Mode::Off;
  }

  if (m_Mode == Mode::MovePlane)
  {
    if (e->button() == Qt::MouseButton::MiddleButton)
      m_Mode = Mode::Off;
  }

  if (m_Mode == Mode::Pan)
  {
    if (e->button() == Qt::MouseButton::LeftButton)
      m_Mode = Mode::UpDown;

    if (e->button() == Qt::MouseButton::RightButton)
      m_Mode = Mode::Orbit;
  }

  // just to be save
  if (e->buttons() == Qt::NoButton || m_Mode == Mode::Off)
  {
    m_Mode = Mode::Off;
    ResetCursor();
  }

  return xiiEditorInput::WasExclusivelyHandled;
}

xiiEditorInput xiiOrbitCameraContext::DoMouseMoveEvent(QMouseEvent* e)
{
  // do nothing, unless this is an active context
  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;

  if (m_pCamera == nullptr)
    return xiiEditorInput::MayBeHandledByOthers;

  if (!m_pCamera->IsPerspective())
    return xiiEditorInput::MayBeHandledByOthers;

  if (m_Mode == Mode::Off)
    return xiiEditorInput::MayBeHandledByOthers;

  const xiiVec2I32 CurMousePos(e->globalX(), e->globalY());
  const xiiVec2I32 diff = CurMousePos - m_vLastMousePos;
  m_vLastMousePos       = UpdateMouseMode(e);

  SetCurrentMouseMode();

  const float fMouseMoveSensitivity = 0.002f;

  const xiiVec3 vHalfExtents = m_Volume.GetHalfExtents();
  const float   fMaxExtent   = xiiMath::Max(vHalfExtents.x, vHalfExtents.y, vHalfExtents.z);
  const float   fBoost       = e->modifiers().testFlag(Qt::KeyboardModifier::ShiftModifier) ? 5.0f : 1.0f;
  const float   fSensitivity = fBoost * 0.0001f * fMaxExtent;

  if (m_Mode == Mode::Orbit)
  {
    float fMoveRight = diff.x * fMouseMoveSensitivity;
    float fMoveUp    = -diff.y * fMouseMoveSensitivity;

    float fDistance = (m_vOrbitPoint - m_pCamera->GetCenterPosition()).GetLength();

    m_pCamera->RotateLocally(xiiAngle::Radian(0.0f), xiiAngle::Radian(fMoveUp), xiiAngle::Radian(0.0f));
    m_pCamera->RotateGlobally(xiiAngle::Radian(0.0f), xiiAngle::Radian(0.0f), xiiAngle::Radian(fMoveRight));

    xiiVec3 vDir = m_pCamera->GetDirForwards();
    if (fDistance == 0.0f || vDir.SetLength(fDistance).Failed())
    {
      vDir.Set(1.0f, 0, 0);
    }

    m_pCamera->LookAt(m_vOrbitPoint - vDir, m_vOrbitPoint, xiiVec3(0.0f, 0.0f, 1.0f));
  }

  if (m_Mode == Mode::MovePlane)
  {
    const xiiVec3 vRight   = m_pCamera->GetCenterDirRight();
    xiiVec3       vForward = m_pCamera->GetCenterDirForwards();
    vForward.z             = 0;
    vForward.Normalize();

    xiiVec3 vNewPos = m_vOrbitPoint + fSensitivity * diff.x * vRight - fSensitivity * diff.y * vForward;

    //vNewPos = m_Volume.GetClampedPoint(vNewPos);
    const xiiVec3 vCamDiff = vNewPos - m_vOrbitPoint;

    m_vOrbitPoint = vNewPos;
    m_pCamera->MoveGlobally(vCamDiff.x, vCamDiff.y, vCamDiff.z);
  }

  if (m_Mode == Mode::Pan)
  {
    const xiiVec3 vRight = m_pCamera->GetCenterDirRight();
    const xiiVec3 vUp    = m_pCamera->GetCenterDirUp();

    xiiVec3 vNewPos = m_vOrbitPoint + fSensitivity * diff.x * vRight - fSensitivity * diff.y * vUp;

    //vNewPos = m_Volume.GetClampedPoint(vNewPos);
    const xiiVec3 vCamDiff = vNewPos - m_vOrbitPoint;

    m_vOrbitPoint = vNewPos;
    m_pCamera->MoveGlobally(vCamDiff.x, vCamDiff.y, vCamDiff.z);
  }

  if (m_Mode == Mode::UpDown)
  {
    const xiiVec3 vUp(0, 0, 1);

    xiiVec3 vNewPos = m_vOrbitPoint - fSensitivity * diff.y * vUp;

    //vNewPos = m_Volume.GetClampedPoint(vNewPos);
    const xiiVec3 vCamDiff = vNewPos - m_vOrbitPoint;

    m_vOrbitPoint = vNewPos;
    m_pCamera->MoveGlobally(vCamDiff.x, vCamDiff.y, vCamDiff.z);
  }

  return xiiEditorInput::WasExclusivelyHandled;
}

xiiEditorInput xiiOrbitCameraContext::DoWheelEvent(QWheelEvent* e)
{
  if (m_Mode != Mode::Off)
    return xiiEditorInput::WasExclusivelyHandled; // ignore it, but others should not handle it either

  if (!m_pCamera->IsPerspective())
    return xiiEditorInput::MayBeHandledByOthers;

  const float fScale = e->modifiers().testFlag(Qt::KeyboardModifier::ShiftModifier) ? 1.4f : 1.1f;

  float fDistance = (m_vOrbitPoint - m_pCamera->GetCenterPosition()).GetLength();
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  if (e->angleDelta().y() > 0)
#else
  if (e->delta() > 0)
#endif
  {
    fDistance /= fScale;
  }
  else
  {
    fDistance *= fScale;
  }

  xiiVec3 vDir = m_pCamera->GetDirForwards();
  if (fDistance == 0.0f || vDir.SetLength(fDistance).Failed())
  {
    vDir.Set(1.0f, 0, 0);
  }

  m_pCamera->LookAt(m_vOrbitPoint - vDir, m_vOrbitPoint, xiiVec3(0.0f, 0.0f, 1.0f));

  // handled, independent of whether we are the active context or not
  return xiiEditorInput::WasExclusivelyHandled;
}


xiiEditorInput xiiOrbitCameraContext::DoKeyPressEvent(QKeyEvent* e)
{
  if (e->key() == Qt::Key_Space)
  {
    const xiiVec3 vDiff = m_Volume.GetCenter() - m_vOrbitPoint;
    m_vOrbitPoint       = m_Volume.GetCenter();
    m_pCamera->MoveGlobally(vDiff.x, vDiff.y, vDiff.z);

    return xiiEditorInput::WasExclusivelyHandled;
  }

  if (e->key() == Qt::Key_F)
  {
    m_pCamera->LookAt(m_vDefaultCameraPosition, m_vOrbitPoint, xiiVec3(0, 0, 1));
    return xiiEditorInput::WasExclusivelyHandled;
  }

  return xiiEditorInput::MayBeHandledByOthers;
}

void xiiOrbitCameraContext::ResetCursor()
{
  if (m_Mode == Mode::Off)
  {
    SetMouseMode(xiiEditorInputContext::MouseMode::Normal);
    MakeActiveInputContext(false);
  }
}

void xiiOrbitCameraContext::SetCurrentMouseMode()
{
  if (m_Mode != Mode::Off)
  {
    SetMouseMode(xiiEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
  }
  else
  {
    SetMouseMode(xiiEditorInputContext::MouseMode::Normal);
  }
}
