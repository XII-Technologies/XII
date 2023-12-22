#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/ConeLengthGizmo.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiConeLengthGizmo, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiConeLengthGizmo::xiiConeLengthGizmo()
{
  m_fRadius      = 1.0f;
  m_fRadiusScale = 0.1f;

  m_ManipulateMode = ManipulateMode::None;

  m_hConeRadius.ConfigureHandle(this, xiiEngineGizmoHandleType::Cone, xiiColorLinearUB(200, 200, 200, 128), xiiGizmoFlags::Pickable | xiiGizmoFlags::OnTop); // this gizmo should be rendered very last so it is always on top

  SetVisible(false);
  SetTransformation(xiiTransform::MakeIdentity());
}

void xiiConeLengthGizmo::OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hConeRadius);
}

void xiiConeLengthGizmo::OnVisibleChanged(bool bVisible)
{
  m_hConeRadius.SetVisible(bVisible);
}

void xiiConeLengthGizmo::OnTransformationChanged(const xiiTransform& transform)
{
  xiiTransform t = transform;
  t.m_vScale *= xiiVec3(1.0f, m_fRadiusScale, m_fRadiusScale) * m_fRadius;

  m_hConeRadius.SetTransformation(t);
}

void xiiConeLengthGizmo::DoFocusLost(bool bCancel)
{
  xiiGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type   = bCancel ? xiiGizmoEvent::Type::CancelInteractions : xiiGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  xiiViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_hConeRadius.SetVisible(true);

  m_ManipulateMode = ManipulateMode::None;
}

xiiEditorInput xiiConeLengthGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return xiiEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return xiiEditorInput::MayBeHandledByOthers;
  if (e->modifiers() != 0)
    return xiiEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_hConeRadius)
  {
    m_ManipulateMode = ManipulateMode::Radius;
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

xiiEditorInput xiiConeLengthGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return xiiEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return xiiEditorInput::WasExclusivelyHandled;
}

xiiEditorInput xiiConeLengthGizmo::DoMouseMoveEvent(QMouseEvent* e)
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
  const xiiAngle aSpeed = xiiAngle::Degree(1.0f);

  if (m_ManipulateMode == ManipulateMode::Radius)
  {
    m_fRadius += vDiff.x * fSpeed;
    m_fRadius -= vDiff.y * fSpeed;

    m_fRadius = xiiMath::Max(0.0f, m_fRadius);
  }

  // update the scale
  OnTransformationChanged(GetTransformation());

  xiiGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type   = xiiGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return xiiEditorInput::WasExclusivelyHandled;
}

void xiiConeLengthGizmo::SetRadius(float fRadius)
{
  m_fRadius = fRadius;

  // update the scale
  OnTransformationChanged(GetTransformation());
}
