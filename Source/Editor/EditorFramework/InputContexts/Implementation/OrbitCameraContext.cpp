#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>

xiiOrbitCameraContext::xiiOrbitCameraContext(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView)
{
  m_Volume  = xiiBoundingBox::MakeFromCenterAndHalfExtents(xiiVec3::MakeZero(), xiiVec3::MakeZero());
  m_pCamera = nullptr;

  m_LastUpdate = xiiTime::Now();

  // while the camera moves, ignore all other shortcuts
  SetShortcutsDisabled(true);

  SetOwner(pOwnerWindow, pOwnerView);
}

void xiiOrbitCameraContext::SetCamera(xiiCamera* pCamera)
{
  m_pCamera = pCamera;
}

xiiCamera* xiiOrbitCameraContext::GetCamera() const
{
  return m_pCamera;
}

void xiiOrbitCameraContext::SetDefaultCameraRelative(const xiiVec3& vDirection, float fDistanceScale)
{
  m_bFixedDefaultCamera = false;

  m_vDefaultCamera = vDirection;
  m_vDefaultCamera.NormalizeIfNotZero(xiiVec3::MakeAxisX()).IgnoreResult();
  m_vDefaultCamera *= xiiMath::Max(0.01f, fDistanceScale);
}

void xiiOrbitCameraContext::SetDefaultCameraFixed(const xiiVec3& vPosition)
{
  m_bFixedDefaultCamera = true;
  m_vDefaultCamera      = vPosition;
}

void xiiOrbitCameraContext::MoveCameraToDefaultPosition()
{
  if (!m_pCamera)
    return;

  const xiiVec3 vCenterPos = m_Volume.GetCenter();
  xiiVec3       vCamPos    = m_vDefaultCamera;

  if (!m_bFixedDefaultCamera)
  {
    const xiiVec3 ext = m_Volume.GetHalfExtents();

    vCamPos = vCenterPos + m_vDefaultCamera * xiiMath::Max(0.1f, xiiMath::Max(ext.x, ext.y, ext.z));
  }

  m_pCamera->LookAt(vCamPos, vCenterPos, xiiVec3(0, 0, 1));
}

void xiiOrbitCameraContext::SetOrbitVolume(const xiiVec3& vCenterPos, const xiiVec3& vHalfBoxSize)
{
  bool bSetCamLookAt = false;

  if (m_Volume.GetHalfExtents().IsZero() && !vHalfBoxSize.IsZero())
  {
    bSetCamLookAt = true;
  }

  m_Volume = xiiBoundingBox::MakeFromCenterAndHalfExtents(vCenterPos, vHalfBoxSize);

  if (bSetCamLookAt)
  {
    MoveCameraToDefaultPosition();
  }
}

void xiiOrbitCameraContext::DoFocusLost(bool bCancel)
{
  m_Mode = Mode::Off;

  m_bRun           = false;
  m_bMoveForwards  = false;
  m_bMoveBackwards = false;
  m_bMoveLeft      = false;
  m_bMoveRight     = false;
  m_bMoveUp        = false;
  m_bMoveDown      = false;

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
      m_Mode = Mode::Free;
      goto activate;
    }
  }

  if (m_Mode == Mode::Free)
  {
    if (e->button() == Qt::MouseButton::LeftButton)
      m_Mode = Mode::Pan;

    return xiiEditorInput::WasExclusivelyHandled;
  }

  if (m_Mode == Mode::Orbit)
  {
    if (e->button() == Qt::MouseButton::RightButton)
      m_Mode = Mode::Pan;

    return xiiEditorInput::WasExclusivelyHandled;
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

  if (m_Mode == Mode::Free)
  {
    if (e->button() == Qt::MouseButton::RightButton)
      m_Mode = Mode::Off;
  }

  if (m_Mode == Mode::Pan)
  {
    if (e->button() == Qt::MouseButton::LeftButton)
      m_Mode = Mode::Free;

    if (e->button() == Qt::MouseButton::RightButton)
      m_Mode = Mode::Off;
  }

  // just to be save
  if (e->buttons() == Qt::NoButton || m_Mode == Mode::Off)
  {
    m_Mode           = Mode::Off;
    m_bRun           = false;
    m_bMoveForwards  = false;
    m_bMoveBackwards = false;
    m_bMoveLeft      = false;
    m_bMoveRight     = false;
    m_bMoveUp        = false;
    m_bMoveDown      = false;
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

  const xiiVec2I32 CurMousePos(QCursor::pos().x(), QCursor::pos().y());
  const xiiVec2I32 diff = CurMousePos - m_vLastMousePos;
  m_vLastMousePos       = UpdateMouseMode(e);

  SetCurrentMouseMode();

  const float fMouseMoveSensitivity = 0.002f;

  const xiiVec3 vHalfExtents = m_Volume.GetHalfExtents();
  const float   fMaxExtent   = xiiMath::Max(vHalfExtents.x, vHalfExtents.y, vHalfExtents.z);
  const float   fBoost       = e->modifiers().testFlag(Qt::KeyboardModifier::ShiftModifier) ? 5.0f : 1.0f;

  if (m_Mode == Mode::Orbit)
  {
    const float fMoveRight = diff.x * fMouseMoveSensitivity;
    const float fMoveUp    = -diff.y * fMouseMoveSensitivity;

    const xiiVec3 vOrbitPoint = m_Volume.GetCenter();

    const float fDistance = (vOrbitPoint - m_pCamera->GetCenterPosition()).GetLength();

    if (fDistance > 0.01f)
    {
      // first force the camera to rotate towards the orbit point
      // this way the camera position doesn't jump around
      m_pCamera->LookAt(m_pCamera->GetCenterPosition(), vOrbitPoint, xiiVec3(0.0f, 0.0f, 1.0f));
    }

    // then rotate the camera, and adjust its position to again point at the orbit point

    m_pCamera->RotateLocally(xiiAngle::MakeFromRadian(0.0f), xiiAngle::MakeFromRadian(fMoveUp), xiiAngle::MakeFromRadian(0.0f));
    m_pCamera->RotateGlobally(xiiAngle::MakeFromRadian(0.0f), xiiAngle::MakeFromRadian(0.0f), xiiAngle::MakeFromRadian(fMoveRight));

    xiiVec3 vDir = m_pCamera->GetDirForwards();
    if (fDistance == 0.0f || vDir.SetLength(fDistance).Failed())
    {
      vDir.Set(1.0f, 0, 0);
    }

    m_pCamera->LookAt(vOrbitPoint - vDir, vOrbitPoint, xiiVec3(0.0f, 0.0f, 1.0f));
  }

  if (m_Mode == Mode::Free)
  {
    const float    fAspectRatio = (float)GetOwnerView()->size().width() / (float)GetOwnerView()->size().height();
    const xiiAngle fFovX        = m_pCamera->GetFovX(fAspectRatio);
    const xiiAngle fFovY        = m_pCamera->GetFovY(fAspectRatio);

    float fRotateBoost = 1.0f;

    const float fMouseScale              = 4.0f;
    const float fMouseRotateSensitivityX = (fFovX.GetRadian() / (float)GetOwnerView()->size().width()) * fRotateBoost * fMouseScale;
    const float fMouseRotateSensitivityY = (fFovY.GetRadian() / (float)GetOwnerView()->size().height()) * fRotateBoost * fMouseScale;

    float fRotateHorizontal = diff.x * fMouseRotateSensitivityX;
    float fRotateVertical   = -diff.y * fMouseRotateSensitivityY;

    m_pCamera->RotateLocally(xiiAngle::MakeFromRadian(0), xiiAngle::MakeFromRadian(fRotateVertical), xiiAngle::MakeFromRadian(0));
    m_pCamera->RotateGlobally(xiiAngle::MakeFromRadian(0), xiiAngle::MakeFromRadian(0), xiiAngle::MakeFromRadian(fRotateHorizontal));
  }

  if (m_Mode == Mode::Pan)
  {
    const float fSpeedFactor = GetCameraSpeed();

    const float fMoveUp    = -diff.y * fMouseMoveSensitivity * fSpeedFactor;
    const float fMoveRight = diff.x * fMouseMoveSensitivity * fSpeedFactor;

    m_pCamera->MoveLocally(0, fMoveRight, fMoveUp);
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

  const xiiVec3 vOrbitPoint = m_Volume.GetCenter();

  float fDistance = (vOrbitPoint - m_pCamera->GetCenterPosition()).GetLength();

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

  m_pCamera->LookAt(vOrbitPoint - vDir, vOrbitPoint, xiiVec3(0.0f, 0.0f, 1.0f));

  // handled, independent of whether we are the active context or not
  return xiiEditorInput::WasExclusivelyHandled;
}


xiiEditorInput xiiOrbitCameraContext::DoKeyPressEvent(QKeyEvent* e)
{
  if (e->key() == Qt::Key_F)
  {
    MoveCameraToDefaultPosition();
    return xiiEditorInput::WasExclusivelyHandled;
  }

  if (m_Mode != Mode::Free)
    return xiiEditorInput::MayBeHandledByOthers;

  m_bRun = (e->modifiers() & Qt::KeyboardModifier::ShiftModifier) != 0;

  switch (e->key())
  {
    case Qt::Key_W:
      m_bMoveForwards = true;
      return xiiEditorInput::WasExclusivelyHandled;
    case Qt::Key_S:
      m_bMoveBackwards = true;
      return xiiEditorInput::WasExclusivelyHandled;
    case Qt::Key_A:
      m_bMoveLeft = true;
      return xiiEditorInput::WasExclusivelyHandled;
    case Qt::Key_D:
      m_bMoveRight = true;
      return xiiEditorInput::WasExclusivelyHandled;
    case Qt::Key_Q:
      m_bMoveDown = true;
      return xiiEditorInput::WasExclusivelyHandled;
    case Qt::Key_E:
      m_bMoveUp = true;
      return xiiEditorInput::WasExclusivelyHandled;
  }

  return xiiEditorInput::MayBeHandledByOthers;
}

xiiEditorInput xiiOrbitCameraContext::DoKeyReleaseEvent(QKeyEvent* e)
{
  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;

  if (m_pCamera == nullptr)
    return xiiEditorInput::MayBeHandledByOthers;

  m_bRun = (e->modifiers() & Qt::KeyboardModifier::ShiftModifier) != 0;

  switch (e->key())
  {
    case Qt::Key_W:
      m_bMoveForwards = false;
      return xiiEditorInput::WasExclusivelyHandled;
    case Qt::Key_S:
      m_bMoveBackwards = false;
      return xiiEditorInput::WasExclusivelyHandled;
    case Qt::Key_A:
      m_bMoveLeft = false;
      return xiiEditorInput::WasExclusivelyHandled;
    case Qt::Key_D:
      m_bMoveRight = false;
      return xiiEditorInput::WasExclusivelyHandled;
    case Qt::Key_Q:
      m_bMoveDown = false;
      return xiiEditorInput::WasExclusivelyHandled;
    case Qt::Key_E:
      m_bMoveUp = false;
      return xiiEditorInput::WasExclusivelyHandled;
  }

  return xiiEditorInput::MayBeHandledByOthers;
}

void xiiOrbitCameraContext::UpdateContext()
{
  xiiTime diff = xiiTime::Now() - m_LastUpdate;
  m_LastUpdate = xiiTime::Now();

  const double TimeDiff = xiiMath::Min(diff.GetSeconds(), 0.1);

  float fSpeedFactor = TimeDiff;

  if (m_bRun)
    fSpeedFactor *= 5.0f;

  fSpeedFactor *= GetCameraSpeed();

  if (m_bMoveForwards)
    m_pCamera->MoveLocally(fSpeedFactor, 0, 0);
  if (m_bMoveBackwards)
    m_pCamera->MoveLocally(-fSpeedFactor, 0, 0);
  if (m_bMoveRight)
    m_pCamera->MoveLocally(0, fSpeedFactor, 0);
  if (m_bMoveLeft)
    m_pCamera->MoveLocally(0, -fSpeedFactor, 0);
  if (m_bMoveUp)
    m_pCamera->MoveGlobally(0, 0, 1 * fSpeedFactor);
  if (m_bMoveDown)
    m_pCamera->MoveGlobally(0, 0, -1 * fSpeedFactor);
}

float xiiOrbitCameraContext::GetCameraSpeed() const
{
  const xiiVec3 ext   = m_Volume.GetHalfExtents();
  float         fSize = xiiMath::Max(0.1f, ext.x, ext.y, ext.z);

  return fSize;
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
