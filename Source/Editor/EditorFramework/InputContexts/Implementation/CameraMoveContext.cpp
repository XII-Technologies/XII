#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/InputContexts/CameraMoveContext.h>
#include <EditorFramework/Preferences/ScenePreferences.h>
#include <Foundation/Utilities/GraphicsUtils.h>

static constexpr float s_fMoveSpeed[25] = {
  0.5f,
  0.75f,
  1.0f,
  1.5f,
  2.0f,

  3.0f,
  4.0f,
  6.0f,
  8.0f,
  12.0f,

  16.0f,
  24.0f,
  32.0f,
  48.0f,
  64.0f,

  96.0f,
  128.0f,
  192.0f,
  256.0f,
  384.0f,

  512.0f,
  768.0f,
  1024.0f,
  1536.0f,
  2048.0f,
};

xiiCameraMoveContext::xiiCameraMoveContext(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView)
{
  m_vOrbitPoint.SetZero();
  m_pCamera = nullptr;

  m_bRun                  = false;
  m_bSlowDown             = false;
  m_bMoveForwards         = false;
  m_bMoveBackwards        = false;
  m_bMoveRight            = false;
  m_bMoveLeft             = false;
  m_bMoveUp               = false;
  m_bMoveDown             = false;
  m_bMoveForwardsInPlane  = false;
  m_bMoveBackwardsInPlane = false;
  m_bOpenMenuOnMouseUp    = false;

  m_LastUpdate = xiiTime::Now();

  m_bRotateCamera      = false;
  m_bMoveCamera        = false;
  m_bMoveCameraInPlane = false;
  m_bOrbitCamera       = false;
  m_bSlideForwards     = false;
  m_bPanOrbitPoint     = false;

  m_bRotateLeft  = false;
  m_bRotateRight = false;
  m_bRotateUp    = false;
  m_bRotateDown  = false;

  // while the camera moves, ignore all other shortcuts
  SetShortcutsDisabled(true);

  SetOwner(pOwnerWindow, pOwnerView);
}

float xiiCameraMoveContext::ConvertCameraSpeed(xiiUInt32 uiSpeedIdx)
{
  return s_fMoveSpeed[xiiMath::Clamp<xiiUInt32>(uiSpeedIdx, 0, XII_ARRAY_SIZE(s_fMoveSpeed) - 1)];
}

void xiiCameraMoveContext::DoFocusLost(bool bCancel)
{
  m_bRotateCamera      = false;
  m_bMoveCamera        = false;
  m_bMoveCameraInPlane = false;
  m_bOrbitCamera       = false;
  m_bSlideForwards     = false;
  m_bOpenMenuOnMouseUp = false;
  m_bPanOrbitPoint     = false;

  ResetCursor();

  m_bRun                  = false;
  m_bSlowDown             = false;
  m_bMoveForwards         = false;
  m_bMoveBackwards        = false;
  m_bMoveRight            = false;
  m_bMoveLeft             = false;
  m_bMoveUp               = false;
  m_bMoveDown             = false;
  m_bMoveForwardsInPlane  = false;
  m_bMoveBackwardsInPlane = false;
  m_bRotateLeft           = false;
  m_bRotateRight          = false;
  m_bRotateUp             = false;
  m_bRotateDown           = false;
}

void xiiCameraMoveContext::LoadState()
{
  const xiiScenePreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiScenePreferencesUser>(GetOwnerWindow()->GetDocument());
  SetMoveSpeed(pPreferences->GetCameraSpeed());
}

void xiiCameraMoveContext::UpdateContext()
{
  xiiTime diff = xiiTime::Now() - m_LastUpdate;
  m_LastUpdate = xiiTime::Now();

  const double TimeDiff = xiiMath::Min(diff.GetSeconds(), 0.1);

  xiiScenePreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiScenePreferencesUser>(GetOwnerWindow()->GetDocument());
  float                    fSpeedFactor = TimeDiff;

  if (m_bRun)
    fSpeedFactor *= 5.0f;
  if (m_bSlowDown)
    fSpeedFactor *= 0.2f;

  const float fRotateHorizontal = 45 * fSpeedFactor;
  const float fRotateVertical   = 45 * fSpeedFactor;

  fSpeedFactor *= ConvertCameraSpeed(pPreferences->GetCameraSpeed());

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
  if (m_bRotateLeft)
    m_pCamera->RotateGlobally(xiiAngle::MakeFromRadian(0), xiiAngle::MakeFromRadian(0), xiiAngle::MakeFromDegree(-fRotateHorizontal));
  if (m_bRotateRight)
    m_pCamera->RotateGlobally(xiiAngle::MakeFromRadian(0), xiiAngle::MakeFromRadian(0), xiiAngle::MakeFromDegree(fRotateHorizontal));
  if (m_bRotateUp)
    m_pCamera->RotateLocally(xiiAngle::MakeFromRadian(0), xiiAngle::MakeFromDegree(fRotateVertical), xiiAngle::MakeFromRadian(0));
  if (m_bRotateDown)
    m_pCamera->RotateLocally(xiiAngle::MakeFromRadian(0), xiiAngle::MakeFromDegree(-fRotateVertical), xiiAngle::MakeFromRadian(0));

  if (m_bMoveForwardsInPlane)
  {
    if (m_pCamera->IsPerspective())
    {
      xiiVec3 vDir = m_pCamera->GetCenterDirForwards();
      vDir.z       = 0.0f;
      vDir.NormalizeIfNotZero(xiiVec3::MakeZero()).IgnoreResult();
      m_pCamera->MoveGlobally(vDir.x * fSpeedFactor, vDir.y * fSpeedFactor, vDir.z * fSpeedFactor);
    }
    else
    {
      m_pCamera->MoveLocally(0, 0, fSpeedFactor);
    }
  }

  if (m_bMoveBackwardsInPlane)
  {
    if (m_pCamera->IsPerspective())
    {
      xiiVec3 vDir = m_pCamera->GetCenterDirForwards();
      vDir.z       = 0.0f;
      vDir.NormalizeIfNotZero(xiiVec3::MakeZero()).IgnoreResult();
      m_pCamera->MoveGlobally(vDir.x * -fSpeedFactor, vDir.y * -fSpeedFactor, vDir.z * -fSpeedFactor);
    }
    else
    {
      m_pCamera->MoveLocally(0, 0, -fSpeedFactor);
    }
  }
}

void xiiCameraMoveContext::DeactivateIfLast()
{
  if (m_bRotateCamera || m_bMoveCamera || m_bMoveCameraInPlane || m_bOrbitCamera || m_bSlideForwards || m_bPanOrbitPoint || m_bMoveForwards || m_bMoveBackwards || m_bMoveRight || m_bMoveLeft || m_bMoveUp || m_bMoveDown || m_bMoveForwardsInPlane || m_bMoveBackwardsInPlane || m_bRotateLeft || m_bRotateRight || m_bRotateUp || m_bRotateDown)
    return;

  FocusLost(false);
}

xiiEditorInput xiiCameraMoveContext::DoKeyReleaseEvent(QKeyEvent* e)
{
  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;

  if (m_pCamera == nullptr)
    return xiiEditorInput::MayBeHandledByOthers;

  m_bRun      = (e->modifiers() & Qt::KeyboardModifier::ShiftModifier) != 0;
  m_bSlowDown = false;

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
    case Qt::Key_Left:
      m_bMoveLeft   = false;
      m_bRotateLeft = false;
      DeactivateIfLast();
      return xiiEditorInput::WasExclusivelyHandled;
    case Qt::Key_Right:
      m_bMoveRight   = false;
      m_bRotateRight = false;
      DeactivateIfLast();
      return xiiEditorInput::WasExclusivelyHandled;
    case Qt::Key_Up:
      m_bMoveForwards = false;
      m_bRotateUp     = false;
      DeactivateIfLast();
      return xiiEditorInput::WasExclusivelyHandled;
    case Qt::Key_Down:
      m_bMoveBackwards = false;
      m_bRotateDown    = false;
      DeactivateIfLast();
      return xiiEditorInput::WasExclusivelyHandled;
  }

  return xiiEditorInput::MayBeHandledByOthers;
}

xiiEditorInput xiiCameraMoveContext::DoKeyPressEvent(QKeyEvent* e)
{
  if (m_pCamera == nullptr)
    return xiiEditorInput::MayBeHandledByOthers;

  //if (e->modifiers() == Qt::KeyboardModifier::ControlModifier)
  //  return xiiEditorInput::MayBeHandledByOthers;

  m_bRun = (e->modifiers() & Qt::KeyboardModifier::ShiftModifier) != 0;

  switch (e->key())
  {
    case Qt::Key_Left:
      if (e->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier))
        m_bRotateLeft = true;
      else
        m_bMoveLeft = true;
      SetActiveInputContext(this);
      return xiiEditorInput::WasExclusivelyHandled;
    case Qt::Key_Right:
      if (e->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier))
        m_bRotateRight = true;
      else
        m_bMoveRight = true;
      SetActiveInputContext(this);
      return xiiEditorInput::WasExclusivelyHandled;
    case Qt::Key_Up:
      if (e->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier))
        m_bRotateUp = true;
      else
        m_bMoveForwards = true;
      SetActiveInputContext(this);
      return xiiEditorInput::WasExclusivelyHandled;
    case Qt::Key_Down:
      if (e->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier))
        m_bRotateDown = true;
      else
        m_bMoveBackwards = true;
      SetActiveInputContext(this);
      return xiiEditorInput::WasExclusivelyHandled;
  }

  if (!m_bRotateCamera)
    return xiiEditorInput::MayBeHandledByOthers;

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

xiiEditorInput xiiCameraMoveContext::DoMousePressEvent(QMouseEvent* e)
{
  if (m_pCamera == nullptr)
    return xiiEditorInput::MayBeHandledByOthers;

  if (m_pCamera->IsOrthographic())
  {
    if (e->button() == Qt::MouseButton::RightButton)
    {
      m_bOpenMenuOnMouseUp = (e->buttons() == Qt::MouseButton::RightButton);
      m_bMoveCamera        = true;
      m_vLastMousePos      = SetMouseMode(xiiEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
      m_bDidMoveMouse[1]   = false;
      MakeActiveInputContext();
      return xiiEditorInput::WasExclusivelyHandled;
    }
    else
    {
      m_bOpenMenuOnMouseUp = false;
    }
  }
  else
  {
    if (e->button() == Qt::MouseButton::RightButton)
    {
      m_bSlideForwards     = false;
      m_bRotateCamera      = false;
      m_bOpenMenuOnMouseUp = (e->buttons() == Qt::MouseButton::RightButton);

      m_fSlideForwardsDistance = (m_vOrbitPoint - m_pCamera->GetPosition()).GetLength();

      if ((e->modifiers() & Qt::KeyboardModifier::AltModifier) != 0)
        m_bSlideForwards = true;
      else
        m_bRotateCamera = true;

      m_vLastMousePos    = SetMouseMode(xiiEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
      m_bDidMoveMouse[1] = false;
      MakeActiveInputContext();
      return xiiEditorInput::WasExclusivelyHandled;
    }
    else
    {
      m_bOpenMenuOnMouseUp = false;
    }

    if (e->button() == Qt::MouseButton::LeftButton)
    {
      m_bOrbitCamera = false;
      m_bMoveCamera  = false;

      if ((e->modifiers() & Qt::KeyboardModifier::AltModifier) != 0)
        m_bOrbitCamera = true;
      else
        m_bMoveCamera = true;

      m_vLastMousePos    = SetMouseMode(xiiEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
      m_bDidMoveMouse[0] = false;
      MakeActiveInputContext();
      return xiiEditorInput::WasExclusivelyHandled;
    }

    if (e->button() == Qt::MouseButton::MiddleButton)
    {
      m_bRotateCamera      = false;
      m_bMoveCamera        = false;
      m_bMoveCameraInPlane = false;
      m_bPanOrbitPoint     = false;

      if ((e->modifiers() & Qt::KeyboardModifier::AltModifier) != 0)
      {
        m_bPanOrbitPoint = true;
      }
      else
        m_bMoveCameraInPlane = true;

      m_vLastMousePos    = SetMouseMode(xiiEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
      m_bDidMoveMouse[2] = false;
      MakeActiveInputContext();
      return xiiEditorInput::WasExclusivelyHandled;
    }
  }

  return xiiEditorInput::MayBeHandledByOthers;
}

void xiiCameraMoveContext::ResetCursor()
{
  if (!m_bRotateCamera && !m_bMoveCamera && !m_bMoveCameraInPlane && !m_bOrbitCamera && !m_bSlideForwards)
  {
    SetMouseMode(xiiEditorInputContext::MouseMode::Normal);

    MakeActiveInputContext(false);
  }
}

void xiiCameraMoveContext::SetCurrentMouseMode()
{
  if (m_bRotateCamera || m_bMoveCamera || m_bMoveCameraInPlane || m_bOrbitCamera || m_bSlideForwards)
  {
    SetMouseMode(xiiEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
  }
  else
  {
    SetMouseMode(xiiEditorInputContext::MouseMode::Normal);
  }
}

xiiEditorInput xiiCameraMoveContext::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;

  if (m_pCamera == nullptr)
    return xiiEditorInput::MayBeHandledByOthers;

  if (m_pCamera->IsOrthographic())
  {
    if (e->button() == Qt::MouseButton::RightButton)
    {
      m_bMoveCamera = false;

      ResetCursor();

      if (!m_bDidMoveMouse[1] && m_bOpenMenuOnMouseUp)
      {
        GetOwnerView()->OpenContextMenu(e->globalPosition().toPoint());
      }
      return xiiEditorInput::WasExclusivelyHandled;
    }
  }
  else
  {
    if (e->button() == Qt::MouseButton::RightButton)
    {
      m_bRotateCamera  = false;
      m_bSlideForwards = false;

      m_bMoveForwards  = false;
      m_bMoveBackwards = false;
      m_bMoveLeft      = false;
      m_bMoveRight     = false;
      m_bMoveUp        = false;
      m_bMoveDown      = false;
      m_bRotateLeft    = false;
      m_bRotateRight   = false;
      m_bRotateUp      = false;
      m_bRotateDown    = false;

      ResetCursor();

      if (!m_bDidMoveMouse[1] && m_bOpenMenuOnMouseUp)
      {
        GetOwnerView()->OpenContextMenu(e->globalPosition().toPoint());
      }

      return xiiEditorInput::WasExclusivelyHandled;
    }

    if (e->button() == Qt::MouseButton::LeftButton)
    {
      m_bMoveCamera  = false;
      m_bOrbitCamera = false;
      ResetCursor();

      if (!m_bDidMoveMouse[0])
      {
        // not really handled, so make this context inactive and tell the surrounding code that it may pass
        // the event to the next handler
        return xiiEditorInput::MayBeHandledByOthers;
      }

      return xiiEditorInput::WasExclusivelyHandled;
    }

    if (e->button() == Qt::MouseButton::MiddleButton)
    {
      m_bRotateCamera      = false;
      m_bMoveCamera        = false;
      m_bMoveCameraInPlane = false;
      m_bPanOrbitPoint     = false;

      ResetCursor();

      if (!m_bDidMoveMouse[2])
      {
        // not really handled, so make this context inactive and tell the surrounding code that it may pass
        // the event to the next handler
        return xiiEditorInput::MayBeHandledByOthers;
      }

      return xiiEditorInput::WasExclusivelyHandled;
    }
  }

  return xiiEditorInput::MayBeHandledByOthers;
}

const xiiVec3& xiiCameraMoveContext::GetOrbitPoint() const
{
  return m_vOrbitPoint;
}

void xiiCameraMoveContext::SetOrbitPoint(const xiiVec3& vPos)
{
  m_vOrbitPoint = vPos;
}

xiiEditorInput xiiCameraMoveContext::DoMouseMoveEvent(QMouseEvent* e)
{
  // do nothing, unless this is an active context
  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;

  // store that the mouse has been moved since the last click
  for (xiiInt32 i = 0; i < XII_ARRAY_SIZE(m_bDidMoveMouse); ++i)
    m_bDidMoveMouse[i] = true;

  // send a message to clear any highlight
  xiiViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  if (m_pCamera == nullptr)
    return xiiEditorInput::MayBeHandledByOthers;

  const xiiScenePreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiScenePreferencesUser>(GetOwnerWindow()->GetDocument());

  float fBoost       = 1.0f;
  float fRotateBoost = 1.0f;

  if (m_bRun)
    fBoost = 5.0f;
  if (m_bSlowDown)
  {
    fBoost       = 0.1f;
    fRotateBoost = 0.2f;
  }

  const xiiVec2I32 CurMousePos(QCursor::pos().x(), QCursor::pos().y());
  const xiiVec2I32 diff = CurMousePos - m_vLastMousePos;

  if (m_pCamera->IsOrthographic())
  {
    float fDistPerPixel = 0;

    if (m_pCamera->GetCameraMode() == xiiCameraMode::OrthoFixedHeight)
      fDistPerPixel = m_pCamera->GetFovOrDim() / (float)GetOwnerView()->size().height();

    if (m_pCamera->GetCameraMode() == xiiCameraMode::OrthoFixedWidth)
      fDistPerPixel = m_pCamera->GetFovOrDim() / (float)GetOwnerView()->size().width();

    if (m_bMoveCamera)
    {
      m_vLastMousePos = UpdateMouseMode(e);

      float fMoveUp    = diff.y * fDistPerPixel;
      float fMoveRight = -diff.x * fDistPerPixel;

      m_pCamera->MoveLocally(0, fMoveRight, fMoveUp);

      return xiiEditorInput::WasExclusivelyHandled;
    }
  }
  else
  {
    SetCurrentMouseMode();

    // correct the up vector, if it got messed up
    m_pCamera->LookAt(m_pCamera->GetCenterPosition(), m_pCamera->GetCenterPosition() + m_pCamera->GetCenterDirForwards(), xiiVec3(0, 0, 1));

    const float    fAspectRatio = (float)GetOwnerView()->size().width() / (float)GetOwnerView()->size().height();
    const xiiAngle fFovX        = m_pCamera->GetFovX(fAspectRatio);
    const xiiAngle fFovY        = m_pCamera->GetFovY(fAspectRatio);

    const float fMouseScale = 4.0f;

    const float fMouseMoveSensitivity    = 0.002f * ConvertCameraSpeed(pPreferences->GetCameraSpeed()) * fBoost;
    const float fMouseRotateSensitivityX = (fFovX.GetRadian() / (float)GetOwnerView()->size().width()) * fRotateBoost * fMouseScale;
    const float fMouseRotateSensitivityY = (fFovY.GetRadian() / (float)GetOwnerView()->size().height()) * fRotateBoost * fMouseScale;

    if (m_bRotateCamera && m_bMoveCamera) // left & right mouse button -> pan
    {
      float fMoveUp    = -diff.y * fMouseMoveSensitivity;
      float fMoveRight = diff.x * fMouseMoveSensitivity;

      m_pCamera->MoveLocally(0, fMoveRight, fMoveUp);

      m_vLastMousePos = UpdateMouseMode(e);
      return xiiEditorInput::WasExclusivelyHandled;
    }

    if (m_bRotateCamera || m_bOrbitCamera)
    {
      float fDistToOrbit = 0.0f;

      if (m_bOrbitCamera)
      {
        fDistToOrbit = xiiMath::Max(0.01f, (m_vOrbitPoint - m_pCamera->GetCenterPosition()).GetLength());
      }

      float fRotateHorizontal = diff.x * fMouseRotateSensitivityX;
      float fRotateVertical   = -diff.y * fMouseRotateSensitivityY;

      m_pCamera->RotateLocally(xiiAngle::MakeFromRadian(0), xiiAngle::MakeFromRadian(fRotateVertical), xiiAngle::MakeFromRadian(0));
      m_pCamera->RotateGlobally(xiiAngle::MakeFromRadian(0), xiiAngle::MakeFromRadian(0), xiiAngle::MakeFromRadian(fRotateHorizontal));

      if (m_bOrbitCamera)
      {
        const xiiVec3 vDirection = m_pCamera->GetDirForwards();
        const xiiVec3 vNewCamPos = m_vOrbitPoint - vDirection * fDistToOrbit;

        m_pCamera->LookAt(vNewCamPos, m_vOrbitPoint, m_pCamera->GetDirUp());
      }

      m_vLastMousePos = UpdateMouseMode(e);
      return xiiEditorInput::WasExclusivelyHandled;
    }

    if (m_bMoveCamera)
    {
      float fMoveRight   = diff.x * fMouseMoveSensitivity;
      float fMoveForward = -diff.y * fMouseMoveSensitivity;

      m_pCamera->MoveLocally(fMoveForward, fMoveRight, 0);

      m_vLastMousePos = UpdateMouseMode(e);

      return xiiEditorInput::WasExclusivelyHandled;
    }

    if (m_bMoveCameraInPlane)
    {
      float fMoveRight   = diff.x * fMouseMoveSensitivity;
      float fMoveForward = -diff.y * fMouseMoveSensitivity;

      m_pCamera->MoveLocally(0, fMoveRight, 0);

      xiiVec3 vDir = m_pCamera->GetCenterDirForwards();
      vDir.z       = 0.0f;
      vDir.NormalizeIfNotZero(xiiVec3::MakeZero()).IgnoreResult();

      m_vOrbitPoint += vDir * fMoveForward;
      m_pCamera->MoveGlobally(vDir.x * fMoveForward, vDir.y * fMoveForward, vDir.z * fMoveForward);

      m_vLastMousePos = UpdateMouseMode(e);

      return xiiEditorInput::WasExclusivelyHandled;
    }

    if (m_bSlideForwards)
    {
      float fMove = diff.y * fMouseMoveSensitivity * m_fSlideForwardsDistance * 0.1f;

      m_pCamera->MoveLocally(fMove, 0, 0);

      m_vLastMousePos = UpdateMouseMode(e);

      return xiiEditorInput::WasExclusivelyHandled;
    }

    if (m_bPanOrbitPoint)
    {
      xiiMat4 viewMatrix, projectionMatrix;
      GetOwnerView()->GetCameraMatrices(viewMatrix, projectionMatrix);

      xiiMat4 mvp = projectionMatrix * viewMatrix;

      xiiVec3 vScreenPos(0);
      if (xiiGraphicsUtils::ConvertWorldPosToScreenPos(mvp, 0, 0, GetOwnerView()->width(), GetOwnerView()->height(), m_vOrbitPoint, vScreenPos).Succeeded())
      {
        xiiMat4 invMvp = mvp.GetInverse();

        vScreenPos.x -= diff.x;
        vScreenPos.y += diff.y;

        xiiVec3 vNewPoint(0);
        if (xiiGraphicsUtils::ConvertScreenPosToWorldPos(invMvp, 0, 0, GetOwnerView()->width(), GetOwnerView()->height(), vScreenPos, vNewPoint).Succeeded())
        {
          const xiiVec3 vDiff = vNewPoint - m_vOrbitPoint;

          m_vOrbitPoint = vNewPoint;
          m_pCamera->MoveGlobally(vDiff.x, vDiff.y, vDiff.z);
        }
      }

      m_vLastMousePos = UpdateMouseMode(e);
      return xiiEditorInput::WasExclusivelyHandled;
    }
  }

  return xiiEditorInput::MayBeHandledByOthers;
}

void xiiCameraMoveContext::SetMoveSpeed(xiiInt32 iSpeed)
{
  if (GetOwnerWindow()->GetDocument() != nullptr)
  {
    xiiScenePreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiScenePreferencesUser>(GetOwnerWindow()->GetDocument());
    pPreferences->SetCameraSpeed(iSpeed);
  }
}

xiiEditorInput xiiCameraMoveContext::DoWheelEvent(QWheelEvent* e)
{
  if (m_bMoveCamera || m_bMoveCameraInPlane || m_bOrbitCamera || m_bRotateCamera)
    return xiiEditorInput::WasExclusivelyHandled; // ignore it, but others should not handle it either

  const xiiScenePreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiScenePreferencesUser>(GetOwnerWindow()->GetDocument());

  if (m_pCamera->IsOrthographic())
  {
    float       fBoost = 1.0f;
    const float fTick  = 1.4f;

    float fNewDim = 20.0f;

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    if (e->angleDelta().y() > 0)
#else
    if (e->delta() > 0)
#endif
      fNewDim = m_pCamera->GetFovOrDim() * xiiMath::Pow(1.0f / fTick, fBoost);
    else
      fNewDim = m_pCamera->GetFovOrDim() * xiiMath::Pow(fTick, fBoost);

    fNewDim = xiiMath::Clamp(fNewDim, 1.0f, 2000.0f);

    m_pCamera->SetCameraMode(m_pCamera->GetCameraMode(), fNewDim, m_pCamera->GetNearPlane(), m_pCamera->GetFarPlane());

    // handled, independent of whether we are the active context or not
    return xiiEditorInput::WasExclusivelyHandled;
  }
  else
  {
    if (e->modifiers() == Qt::KeyboardModifier::ControlModifier)
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
      if (e->angleDelta().y() > 0)
#else
      if (e->delta() > 0)
#endif
      {
        SetMoveSpeed(pPreferences->GetCameraSpeed() + 1);
      }
      else
      {
        SetMoveSpeed(pPreferences->GetCameraSpeed() - 1);
      }

      // handled, independent of whether we are the active context or not
      return xiiEditorInput::WasExclusivelyHandled;
    }

    {
      float fBoost = 0.25f;

      if (e->modifiers() == Qt::KeyboardModifier::ShiftModifier)
        fBoost *= 5.0f;

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
      if (e->angleDelta().y() > 0)
#else
      if (e->delta() > 0)
#endif
      {
        m_pCamera->MoveLocally(ConvertCameraSpeed(pPreferences->GetCameraSpeed()) * fBoost, 0, 0);
      }
      else
      {
        m_pCamera->MoveLocally(-ConvertCameraSpeed(pPreferences->GetCameraSpeed()) * fBoost, 0, 0);
      }

      // handled, independent of whether we are the active context or not
      return xiiEditorInput::WasExclusivelyHandled;
    }
  }
}

void xiiCameraMoveContext::SetCamera(xiiCamera* pCamera)
{
  if (m_pCamera == pCamera)
    return;

  m_pCamera = pCamera;
}
