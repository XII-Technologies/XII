/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/Settings/EditorPreferences.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <QCursor>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTranslateGizmo, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiTranslateGizmo::xiiTranslateGizmo()
{
  m_vStartPosition.SetZero();
  m_fCameraSpeed = 0.2f;

  const xiiColor colr = xiiColorScheme::LightUI(xiiColorScheme::Red);
  const xiiColor colg = xiiColorScheme::LightUI(xiiColorScheme::Green);
  const xiiColor colb = xiiColorScheme::LightUI(xiiColorScheme::Blue);

  m_hAxisX.ConfigureHandle(this, xiiEngineGizmoHandleType::TranslationAxis, colr, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::OnTop | xiiGizmoFlags::ShowInOrtho);
  m_hAxisY.ConfigureHandle(this, xiiEngineGizmoHandleType::TranslationAxis, colg, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::OnTop | xiiGizmoFlags::ShowInOrtho);
  m_hAxisZ.ConfigureHandle(this, xiiEngineGizmoHandleType::TranslationAxis, colb, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::OnTop | xiiGizmoFlags::ShowInOrtho);

  m_hPlaneYZ.ConfigureHandle(this, xiiEngineGizmoHandleType::TranslationPlane, xiiColor(colr.r, colr.g, colr.b, 0.18f), xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::OnTop | xiiGizmoFlags::ShowInOrtho | xiiGizmoFlags::FaceCamera);
  m_hPlaneXZ.ConfigureHandle(this, xiiEngineGizmoHandleType::TranslationPlane, xiiColor(colg.r, colg.g, colg.b, 0.18f), xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::OnTop | xiiGizmoFlags::ShowInOrtho | xiiGizmoFlags::FaceCamera);
  m_hPlaneXY.ConfigureHandle(this, xiiEngineGizmoHandleType::TranslationPlane, xiiColor(colb.r, colb.g, colb.b, 0.18f), xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::OnTop | xiiGizmoFlags::ShowInOrtho | xiiGizmoFlags::FaceCamera);

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
  auto axis        = transform;
  axis.m_qRotation = transform.m_qRotation * xiiQuat::MakeShortestRotation(xiiVec3::MakeAxisX(), xiiVec3::MakeAxisY());
  m_hAxisY.SetTransformation(axis);
  axis.m_qRotation = transform.m_qRotation * xiiQuat::MakeShortestRotation(xiiVec3::MakeAxisX(), xiiVec3::MakeAxisZ());
  m_hAxisZ.SetTransformation(axis);
  m_hPlaneXY.SetTransformation(transform);
  axis.m_qRotation = transform.m_qRotation * xiiQuat::MakeShortestRotation(xiiVec3::MakeAxisZ(), xiiVec3::MakeAxisX());
  m_hPlaneYZ.SetTransformation(axis);
  axis.m_qRotation = transform.m_qRotation * xiiQuat::MakeShortestRotation(xiiVec3::MakeAxisZ(), xiiVec3::MakeAxisY());
  m_hPlaneXZ.SetTransformation(axis);

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
  m_vMovementStartTranslation.SetZero();

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
  m_vMovementStartTranslation.SetZero();
  xiiMat4 mProj;
  m_pCamera->GetProjectionMatrix((float)m_vViewport.x / (float)m_vViewport.y, mProj);
  xiiMat4 mViewProj = mProj * mView;
  m_mInvViewProj    = mViewProj.GetInverse();

  SetMouseMode(xiiEditorInputContext::MouseMode::WrapAtScreenBorders);
  const auto pressPosition = e->globalPosition().toPoint();
  m_vLastMousePos.Set(pressPosition.x(), pressPosition.y());
  SetActiveInputContext(this);

  if (m_Mode == TranslateMode::Axis)
  {
    GetPointOnAxis(e->pos().x(), e->pos().y(), m_vInteractionPivot).IgnoreResult();
  }
  else if (m_Mode == TranslateMode::Plane)
  {
    GetPointOnPlane(e->pos().x(), e->pos().y(), m_vInteractionPivot).IgnoreResult();
  }

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

  // Commit the release position even if Qt coalesced the final mouse move.
  DoMouseMoveEvent(e);

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
  if (!Plane.GetRayIntersection(vPos, vRayDir, nullptr, &vIntersection))
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

  xiiVec3 vPlaneTangent = m_vMoveAxis.CrossRH(m_pCamera->GetDirForwards());
  XII_SUCCEED_OR_RETURN(vPlaneTangent.NormalizeIfNotZero());
  const xiiVec3 vPlaneNormal = m_vMoveAxis.CrossRH(vPlaneTangent);

  xiiPlane Plane = xiiPlane::MakeFromNormalAndPoint(vPlaneNormal, m_vStartPosition);

  xiiVec3 vIntersection;
  if (!Plane.GetRayIntersection(vPos, vRayDir, nullptr, &vIntersection))
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

  const QPoint mousePosition = e->globalPosition().toPoint();

  const xiiVec2I32 CurMousePos(mousePosition.x(), mousePosition.y());

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

    vTranslate = vCurrentInteractionPoint - m_vInteractionPivot;
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
    vTranslate += m_vMovementStartTranslation;
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
    const xiiVec3    diff = GetTransformation().m_qRotation.GetInverse() * GetTranslationResult();
    xiiStringBuilder feedback;
    feedback.SetFormat("{} units   X {}  Y {}  Z {}", xiiArgF(GetTranslationResult().GetLength(), 2), xiiArgF(diff.x, 2), xiiArgF(diff.y, 2), xiiArgF(diff.z, 2));
    static_cast<xiiEngineGizmoHandle*>(m_pInteractionGizmoHandle)->SetInteractionText(feedback);
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
    m_vMovementStartTranslation = GetTranslationResult();
    m_vTotalMouseDiff.SetZero();
    m_vLastMousePos = SetMouseMode(xiiEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
  }
  else
  {
    m_vLastMousePos = SetMouseMode(xiiEditorInputContext::MouseMode::WrapAtScreenBorders);
    xiiMat4 projection;
    m_pCamera->GetProjectionMatrix(float(m_vViewport.x) / m_vViewport.y, projection);
    m_mInvViewProj   = (projection * m_pCamera->GetViewMatrix()).GetInverse();
    const auto pixel = GetOwnerView()->mapFromGlobal(QCursor::pos());
    xiiVec3    point;
    const auto result = m_Mode == TranslateMode::Axis ? GetPointOnAxis(pixel.x(), pixel.y(), point) : GetPointOnPlane(pixel.x(), pixel.y(), point);
    if (result.Succeeded())
      m_vInteractionPivot = point - GetTranslationResult();
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
