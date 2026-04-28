/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/ConeAngleGizmo.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiConeAngleGizmo, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiConeAngleGizmo::xiiConeAngleGizmo()
{
  m_Angle       = xiiAngle::MakeFromDegree(1.0f);
  m_fAngleScale = 1.0f;
  m_fRadius     = 1.0f;

  m_ManipulateMode = ManipulateMode::None;

  m_hConeAngle.ConfigureHandle(this, xiiEngineGizmoHandleType::Cone, xiiColorLinearUB(200, 200, 0, 128), xiiGizmoFlags::Pickable);

  SetVisible(false);
  SetTransformation(xiiTransform::MakeIdentity());
}

void xiiConeAngleGizmo::OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hConeAngle);
}

void xiiConeAngleGizmo::OnVisibleChanged(bool bVisible)
{
  m_hConeAngle.SetVisible(bVisible);
}

void xiiConeAngleGizmo::OnTransformationChanged(const xiiTransform& transform)
{
  xiiTransform t = transform;

  t.m_vScale *= xiiVec3(1.0f, m_fAngleScale, m_fAngleScale) * m_fRadius;
  m_hConeAngle.SetTransformation(t);
}

void xiiConeAngleGizmo::DoFocusLost(bool bCancel)
{
  xiiGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type   = bCancel ? xiiGizmoEvent::Type::CancelInteractions : xiiGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  xiiViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_hConeAngle.SetVisible(true);

  m_ManipulateMode = ManipulateMode::None;
}

xiiEditorInput xiiConeAngleGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return xiiEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return xiiEditorInput::MayBeHandledByOthers;
  if (e->modifiers() != 0 && e->modifiers() != Qt::KeyboardModifier::ShiftModifier) // allow shift for toggling snapping
    return xiiEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_hConeAngle)
  {
    m_ManipulateMode = ManipulateMode::Angle;
  }
  else
    return xiiEditorInput::MayBeHandledByOthers;

  xiiViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_LastInteraction = xiiTime::Now();

  m_vLastMousePos = SetMouseMode(xiiEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);

  SetActiveInputContext(this);

  xiiGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type   = xiiGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return xiiEditorInput::WasExclusivelyHandled;
}

xiiEditorInput xiiConeAngleGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return xiiEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return xiiEditorInput::WasExclusivelyHandled;
}

xiiEditorInput xiiConeAngleGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;

  const xiiTime tNow = xiiTime::Now();

  if (tNow - m_LastInteraction < xiiTime::MakeFromSeconds(1.0 / 25.0))
    return xiiEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  const QPoint mousePosition = e->globalPosition().toPoint();

  const xiiVec2I32 vNewMousePos = xiiVec2I32(mousePosition.x(), mousePosition.y());
  const xiiVec2I32 vDiff        = vNewMousePos - m_vLastMousePos;

  m_vLastMousePos = UpdateMouseMode(e);

  const float    fSpeed = 0.02f;
  const xiiAngle aSpeed = xiiAngle::MakeFromDegree(1.0f);

  {
    m_Angle += (float)vDiff.x * aSpeed;
    m_Angle -= (float)vDiff.y * aSpeed;

    m_Angle = xiiMath::Clamp(m_Angle, xiiAngle(), xiiAngle::MakeFromDegree(179.0f));

    m_fAngleScale = xiiMath::Tan(m_Angle * 0.5f);
  }

  // update the scale
  OnTransformationChanged(GetTransformation());

  xiiGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type   = xiiGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return xiiEditorInput::WasExclusivelyHandled;
}

void xiiConeAngleGizmo::SetAngle(xiiAngle angle)
{
  m_Angle       = angle;
  m_fAngleScale = xiiMath::Tan(m_Angle * 0.5f);

  // update the scale
  OnTransformationChanged(GetTransformation());
}
