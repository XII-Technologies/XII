/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/SphereGizmo.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSphereGizmo, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiSphereGizmo::xiiSphereGizmo()
{
  m_bInnerEnabled = false;

  m_fRadiusInner = 1.0f;
  m_fRadiusOuter = 2.0f;

  m_ManipulateMode = ManipulateMode::None;

  m_hInnerSphere.ConfigureHandle(this, xiiEngineGizmoHandleType::Sphere, xiiColorLinearUB(200, 200, 0, 128), xiiGizmoFlags::OnTop | xiiGizmoFlags::Pickable); // this gizmo should be rendered very last so it is always on top
  m_hOuterSphere.ConfigureHandle(this, xiiEngineGizmoHandleType::Sphere, xiiColorLinearUB(200, 200, 200, 128), xiiGizmoFlags::Pickable);

  SetVisible(false);
  SetTransformation(xiiTransform::MakeIdentity());
}

void xiiSphereGizmo::OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hInnerSphere);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hOuterSphere);
}

void xiiSphereGizmo::OnVisibleChanged(bool bVisible)
{
  m_hInnerSphere.SetVisible(bVisible && m_bInnerEnabled);
  m_hOuterSphere.SetVisible(bVisible);
}

void xiiSphereGizmo::OnTransformationChanged(const xiiTransform& transform)
{
  xiiTransform mScaleInner, mScaleOuter;
  mScaleInner.SetIdentity();
  mScaleOuter.SetIdentity();
  mScaleInner.m_vScale = xiiVec3(m_fRadiusInner);
  mScaleOuter.m_vScale = xiiVec3(m_fRadiusOuter);

  m_hInnerSphere.SetTransformation(transform * mScaleInner);
  m_hOuterSphere.SetTransformation(transform * mScaleOuter);
}

void xiiSphereGizmo::DoFocusLost(bool bCancel)
{
  xiiGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type   = bCancel ? xiiGizmoEvent::Type::CancelInteractions : xiiGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  xiiViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_hInnerSphere.SetVisible(m_bInnerEnabled);
  m_hOuterSphere.SetVisible(true);

  m_ManipulateMode = ManipulateMode::None;
}

xiiEditorInput xiiSphereGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return xiiEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return xiiEditorInput::MayBeHandledByOthers;
  if (e->modifiers() != 0 && e->modifiers() != Qt::KeyboardModifier::ShiftModifier) // allow shift for toggling snapping
    return xiiEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_hInnerSphere)
  {
    m_ManipulateMode = ManipulateMode::InnerSphere;
  }
  else if (m_pInteractionGizmoHandle == &m_hOuterSphere)
  {
    m_ManipulateMode = ManipulateMode::OuterSphere;
  }
  else
    return xiiEditorInput::MayBeHandledByOthers;

  xiiViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  // m_InnerSphere.SetVisible(false);
  // m_OuterSphere.SetVisible(false);

  // m_pInteractionGizmoHandle->SetVisible(true);

  m_LastInteraction = xiiTime::Now();

  m_vLastMousePos = SetMouseMode(xiiEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);

  SetActiveInputContext(this);

  xiiGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type   = xiiGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return xiiEditorInput::WasExclusivelyHandled;
}

xiiEditorInput xiiSphereGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return xiiEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return xiiEditorInput::WasExclusivelyHandled;
}

xiiEditorInput xiiSphereGizmo::DoMouseMoveEvent(QMouseEvent* e)
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

  const float fSpeed = 0.02f;

  if (m_ManipulateMode == ManipulateMode::InnerSphere)
  {
    m_fRadiusInner += vDiff.x * fSpeed;
    m_fRadiusInner -= vDiff.y * fSpeed;

    m_fRadiusInner = xiiMath::Max(0.0f, m_fRadiusInner);

    m_fRadiusOuter = xiiMath::Max(m_fRadiusInner, m_fRadiusOuter);
  }
  else
  {
    m_fRadiusOuter += vDiff.x * fSpeed;
    m_fRadiusOuter -= vDiff.y * fSpeed;

    m_fRadiusOuter = xiiMath::Max(0.0f, m_fRadiusOuter);

    m_fRadiusInner = xiiMath::Min(m_fRadiusInner, m_fRadiusOuter);
  }

  // update the scale
  OnTransformationChanged(GetTransformation());

  xiiGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type   = xiiGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return xiiEditorInput::WasExclusivelyHandled;
}

void xiiSphereGizmo::SetInnerSphere(bool bEnabled, float fRadius)
{
  m_fRadiusInner  = fRadius;
  m_bInnerEnabled = bEnabled;

  // update the scale
  OnTransformationChanged(GetTransformation());
}

void xiiSphereGizmo::SetOuterSphere(float fRadius)
{
  m_fRadiusOuter = fRadius;

  // update the scale
  OnTransformationChanged(GetTransformation());
}
