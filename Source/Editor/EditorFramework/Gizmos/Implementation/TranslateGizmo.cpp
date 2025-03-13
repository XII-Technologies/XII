#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Utilities/GraphicsUtils.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTranslateGizmo, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiTranslateGizmo::xiiTranslateGizmo()
{
  m_vStartPosition.SetZero();
  m_fCameraSpeed = 0.2f;

  const xiiColor colr = xiiColorScheme::LightUI(xiiColorScheme::Red);
  const xiiColor colg = xiiColorScheme::LightUI(xiiColorScheme::Green);
  const xiiColor colb = xiiColorScheme::LightUI(xiiColorScheme::Blue);

  m_hAxisX.ConfigureHandle(this, xiiEngineGizmoHandleType::FromFile, colr, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable, "Editor/Meshes/TranslateArrowX.obj");
  m_hAxisY.ConfigureHandle(this, xiiEngineGizmoHandleType::FromFile, colg, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable, "Editor/Meshes/TranslateArrowY.obj");
  m_hAxisZ.ConfigureHandle(this, xiiEngineGizmoHandleType::FromFile, colb, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable, "Editor/Meshes/TranslateArrowZ.obj");

  m_hPlaneYZ.ConfigureHandle(this, xiiEngineGizmoHandleType::FromFile, colr, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::FaceCamera, "Editor/Meshes/TranslatePlaneX.obj");
  m_hPlaneXZ.ConfigureHandle(this, xiiEngineGizmoHandleType::FromFile, colg, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::FaceCamera, "Editor/Meshes/TranslatePlaneY.obj");
  m_hPlaneXY.ConfigureHandle(this, xiiEngineGizmoHandleType::FromFile, colb, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::FaceCamera, "Editor/Meshes/TranslatePlaneZ.obj");

  SetVisible(false);
  SetTransformation(xiiTransform::MakeIdentity());

  m_Mode                  = TranslateMode::None;
  m_MovementMode          = MovementMode::ScreenProjection;
  m_LastHandleInteraction = HandleInteraction::None;
}

void xiiTranslateGizmo::OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisX);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisZ);

  pOwnerWindow->GetDocument()->AddSyncObject(&m_hPlaneXY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hPlaneXZ);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hPlaneYZ);
}

void xiiTranslateGizmo::OnVisibleChanged(bool bVisible)
{
  m_hAxisX.SetVisible(bVisible);
  m_hAxisY.SetVisible(bVisible);
  m_hAxisZ.SetVisible(bVisible);

  m_hPlaneXY.SetVisible(bVisible);
  m_hPlaneXZ.SetVisible(bVisible);
  m_hPlaneYZ.SetVisible(bVisible);
}

void xiiTranslateGizmo::OnTransformationChanged(const xiiTransform& transform)
{
  m_hAxisX.SetTransformation(transform);
  m_hAxisY.SetTransformation(transform);
  m_hAxisZ.SetTransformation(transform);
  m_hPlaneXY.SetTransformation(transform);
  m_hPlaneYZ.SetTransformation(transform);
  m_hPlaneXZ.SetTransformation(transform);

  if (!IsActiveInputContext())
  {
    // if the gizmo is currently not being dragged, copy the translation into the start position
    m_vStartPosition = GetTransformation().m_vPosition;
  }
}

void xiiTranslateGizmo::DoFocusLost(bool bCancel)
{
  xiiGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type   = bCancel ? xiiGizmoEvent::Type::CancelInteractions : xiiGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  xiiViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_hAxisX.SetVisible(true);
  m_hAxisY.SetVisible(true);
  m_hAxisZ.SetVisible(true);

  m_hPlaneXY.SetVisible(true);
  m_hPlaneXZ.SetVisible(true);
  m_hPlaneYZ.SetVisible(true);

  m_Mode                  = TranslateMode::None;
  m_LastHandleInteraction = HandleInteraction::None;
  m_MovementMode          = MovementMode::ScreenProjection;
  m_vLastMoveDiff.SetZero();

  m_vStartPosition = GetTransformation().m_vPosition;
  m_vTotalMouseDiff.SetZero();

  GetOwnerWindow()->SetPermanentStatusBarMsg("");
}

xiiEditorInput xiiTranslateGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return xiiEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return xiiEditorInput::MayBeHandledByOthers;

  m_vLastMoveDiff.SetZero();

  const xiiQuat gizmoRot = GetTransformation().m_qRotation;

  if (m_pInteractionGizmoHandle == &m_hAxisX)
  {
    m_vMoveAxis             = gizmoRot * xiiVec3(1, 0, 0);
    m_Mode                  = TranslateMode::Axis;
    m_LastHandleInteraction = HandleInteraction::AxisX;
  }
  else if (m_pInteractionGizmoHandle == &m_hAxisY)
  {
    m_vMoveAxis             = gizmoRot * xiiVec3(0, 1, 0);
    m_Mode                  = TranslateMode::Axis;
    m_LastHandleInteraction = HandleInteraction::AxisY;
  }
  else if (m_pInteractionGizmoHandle == &m_hAxisZ)
  {
    m_vMoveAxis             = gizmoRot * xiiVec3(0, 0, 1);
    m_Mode                  = TranslateMode::Axis;
    m_LastHandleInteraction = HandleInteraction::AxisZ;
  }
  else if (m_pInteractionGizmoHandle == &m_hPlaneXY)
  {
    m_vMoveAxis             = gizmoRot * xiiVec3(0, 0, 1);
    m_vPlaneAxis[0]         = gizmoRot * xiiVec3(1, 0, 0);
    m_vPlaneAxis[1]         = gizmoRot * xiiVec3(0, 1, 0);
    m_Mode                  = TranslateMode::Plane;
    m_LastHandleInteraction = HandleInteraction::PlaneZ;
  }
  else if (m_pInteractionGizmoHandle == &m_hPlaneXZ)
  {
    m_vMoveAxis             = gizmoRot * xiiVec3(0, 1, 0);
    m_vPlaneAxis[0]         = gizmoRot * xiiVec3(1, 0, 0);
    m_vPlaneAxis[1]         = gizmoRot * xiiVec3(0, 0, 1);
    m_Mode                  = TranslateMode::Plane;
    m_LastHandleInteraction = HandleInteraction::PlaneY;
  }
  else if (m_pInteractionGizmoHandle == &m_hPlaneYZ)
  {
    m_vMoveAxis             = gizmoRot * xiiVec3(1, 0, 0);
    m_vPlaneAxis[0]         = gizmoRot * xiiVec3(0, 1, 0);
    m_vPlaneAxis[1]         = gizmoRot * xiiVec3(0, 0, 1);
    m_Mode                  = TranslateMode::Plane;
    m_LastHandleInteraction = HandleInteraction::PlaneX;
  }
  else
    return xiiEditorInput::MayBeHandledByOthers;

  xiiViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_vStartPosition = GetTransformation().m_vPosition;
  m_vTotalMouseDiff.SetZero();

  xiiMat4 mView = m_pCamera->GetViewMatrix();
  xiiMat4 mProj;
  m_pCamera->GetProjectionMatrix((float)m_vViewport.x / (float)m_vViewport.y, mProj);
  xiiMat4 mViewProj = mProj * mView;
  m_mInvViewProj    = mViewProj.GetInverse();


  m_LastInteraction = xiiTime::Now();

  m_vLastMousePos = SetMouseMode(xiiEditorInputContext::MouseMode::WrapAtScreenBorders);
  SetActiveInputContext(this);

  if (m_Mode == TranslateMode::Axis)
  {
    GetPointOnAxis(e->pos().x(), e->pos().y(), m_vInteractionPivot).IgnoreResult();
  }
  else if (m_Mode == TranslateMode::Plane)
  {
    GetPointOnPlane(e->pos().x(), e->pos().y(), m_vInteractionPivot).IgnoreResult();
  }

  m_fStartScale = (m_vInteractionPivot - m_pCamera->GetPosition()).GetLength() * 0.125;

  xiiGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type   = xiiGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return xiiEditorInput::WasExclusivelyHandled;
}

xiiEditorInput xiiTranslateGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return xiiEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return xiiEditorInput::WasExclusivelyHandled;
}

xiiResult xiiTranslateGizmo::GetPointOnPlane(xiiInt32 iScreenPosX, xiiInt32 iScreenPosY, xiiVec3& out_Result) const
{
  out_Result = m_vStartPosition;

  xiiVec3 vPos, vRayDir;
  if (xiiGraphicsUtils::ConvertScreenPosToWorldPos(m_mInvViewProj, 0, 0, m_vViewport.x, m_vViewport.y, xiiVec3(iScreenPosX, iScreenPosY, 0), vPos, &vRayDir).Failed())
    return XII_FAILURE;

  xiiPlane Plane = xiiPlane::MakeFromNormalAndPoint(m_vMoveAxis, m_vStartPosition);

  xiiVec3 vIntersection;
  if (!Plane.GetRayIntersection(m_pCamera->GetPosition(), vRayDir, nullptr, &vIntersection))
    return XII_FAILURE;

  out_Result = vIntersection;
  return XII_SUCCESS;
}

xiiResult xiiTranslateGizmo::GetPointOnAxis(xiiInt32 iScreenPosX, xiiInt32 iScreenPosY, xiiVec3& out_Result) const
{
  out_Result = m_vStartPosition;

  xiiVec3 vPos, vRayDir;
  if (xiiGraphicsUtils::ConvertScreenPosToWorldPos(m_mInvViewProj, 0, 0, m_vViewport.x, m_vViewport.y, xiiVec3(iScreenPosX, iScreenPosY, 0), vPos, &vRayDir).Failed())
    return XII_FAILURE;

  const xiiVec3 vPlaneTangent = m_vMoveAxis.CrossRH(m_pCamera->GetDirForwards()).GetNormalized();
  const xiiVec3 vPlaneNormal  = m_vMoveAxis.CrossRH(vPlaneTangent);

  xiiPlane Plane = xiiPlane::MakeFromNormalAndPoint(vPlaneNormal, m_vStartPosition);

  xiiVec3 vIntersection;
  if (!Plane.GetRayIntersection(m_pCamera->GetPosition(), vRayDir, nullptr, &vIntersection))
    return XII_FAILURE;

  const xiiVec3 vDirAlongRay     = vIntersection - m_vStartPosition;
  const float   fProjectedLength = vDirAlongRay.Dot(m_vMoveAxis);

  out_Result = m_vStartPosition + fProjectedLength * m_vMoveAxis;
  return XII_SUCCESS;
}

xiiEditorInput xiiTranslateGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;

  const xiiTime tNow = xiiTime::Now();

  if (tNow - m_LastInteraction < xiiTime::MakeFromSeconds(1.0 / 25.0))
    return xiiEditorInput::WasExclusivelyHandled;

  const QPoint mousePosition = e->globalPosition().toPoint();

  const xiiVec2I32 CurMousePos(mousePosition.x(), mousePosition.y());

  m_LastInteraction = tNow;

  xiiTransform mTrans = GetTransformation();
  xiiVec3      vTranslate(0);

  if (m_MovementMode == MovementMode::ScreenProjection)
  {
    xiiVec3 vCurrentInteractionPoint;

    if (m_Mode == TranslateMode::Axis)
    {
      if (GetPointOnAxis(e->pos().x(), e->pos().y(), vCurrentInteractionPoint).Failed())
      {
        m_vLastMousePos = UpdateMouseMode(e);
        return xiiEditorInput::WasExclusivelyHandled;
      }
    }
    else if (m_Mode == TranslateMode::Plane)
    {
      if (GetPointOnPlane(e->pos().x(), e->pos().y(), vCurrentInteractionPoint).Failed())
      {
        m_vLastMousePos = UpdateMouseMode(e);
        return xiiEditorInput::WasExclusivelyHandled;
      }
    }


    const float   fPerspectiveScale = (vCurrentInteractionPoint - m_pCamera->GetPosition()).GetLength() * 0.125;
    const xiiVec3 vOffset           = (m_vInteractionPivot - m_vStartPosition);

    const xiiVec3 vNewPos = vCurrentInteractionPoint - vOffset * fPerspectiveScale / m_fStartScale;

    vTranslate = vNewPos - m_vStartPosition;
  }
  else
  {
    const float fSpeed = m_fCameraSpeed * 0.01f;

    m_vTotalMouseDiff += xiiVec2((float)(CurMousePos.x - m_vLastMousePos.x), (float)(CurMousePos.y - m_vLastMousePos.y));
    const xiiVec3 vMouseDir = m_pCamera->GetDirRight() * m_vTotalMouseDiff.x + -m_pCamera->GetDirUp() * m_vTotalMouseDiff.y;

    if (m_Mode == TranslateMode::Axis)
    {
      vTranslate = m_vMoveAxis * (m_vMoveAxis.Dot(vMouseDir)) * fSpeed;
    }
    else if (m_Mode == TranslateMode::Plane)
    {
      vTranslate = m_vPlaneAxis[0] * (m_vPlaneAxis[0].Dot(vMouseDir)) * fSpeed + m_vPlaneAxis[1] * (m_vPlaneAxis[1].Dot(vMouseDir)) * fSpeed;
    }
  }

  m_vLastMousePos = UpdateMouseMode(e);

  // disable snapping when SHIFT is pressed
  if (!e->modifiers().testFlag(Qt::ShiftModifier))
  {
    xiiSnapProvider::SnapTranslationInLocalSpace(mTrans.m_qRotation, vTranslate);
  }

  const xiiVec3 vLastPos = mTrans.m_vPosition;

  mTrans.m_vPosition = m_vStartPosition + vTranslate;

  m_vLastMoveDiff = mTrans.m_vPosition - vLastPos;

  SetTransformation(mTrans);

  // set statusbar message
  {
    const xiiVec3 diff = GetTransformation().m_qRotation.GetInverse() * GetTranslationResult();
    GetOwnerWindow()->SetPermanentStatusBarMsg(xiiFmt("Translation: {}, {}, {}", xiiArgF(diff.x, 2), xiiArgF(diff.y, 2), xiiArgF(diff.z, 2)));
  }

  if (!m_vLastMoveDiff.IsZero())
  {
    xiiGizmoEvent ev;
    ev.m_pGizmo = this;
    ev.m_Type   = xiiGizmoEvent::Type::Interaction;
    m_GizmoEvents.Broadcast(ev);
  }

  return xiiEditorInput::WasExclusivelyHandled;
}

void xiiTranslateGizmo::SetMovementMode(MovementMode mode)
{
  if (m_MovementMode == mode)
    return;

  m_MovementMode = mode;

  if (m_MovementMode == MovementMode::MouseDiff)
  {
    m_vLastMousePos = SetMouseMode(xiiEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
  }
  else
  {
    m_vLastMousePos = SetMouseMode(xiiEditorInputContext::MouseMode::WrapAtScreenBorders);
  }
}

void xiiTranslateGizmo::SetCameraSpeed(float fSpeed)
{
  m_fCameraSpeed = fSpeed;
}

void xiiTranslateGizmo::UpdateStatusBarText(xiiQtEngineDocumentWindow* pWindow)
{
  const xiiVec3 diff = xiiVec3::MakeZero();
  GetOwnerWindow()->SetPermanentStatusBarMsg(xiiFmt("Translation: {}, {}, {}", xiiArgF(diff.x, 2), xiiArgF(diff.y, 2), xiiArgF(diff.z, 2)));
}
